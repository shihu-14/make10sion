#include "../src/BattleCardRules.hpp"

#include <cstdlib>
#include <iostream>
#include <string_view>

namespace {

using namespace BattleCardRules;

int failures = 0;

void Expect(const bool condition, const std::string_view name) {
	if (condition) return;
	std::cerr << "FAILED: " << name << '\n';
	++failures;
}

BoardSnapshot MakeBoard() {
	BoardSnapshot board;
	board.width = 7;
	board.height = 6;
	board.cells.resize(static_cast<std::size_t>(board.width * board.height), BoardCell{ true, EmptyCardId });
	return board;
}

BoardCell& At(BoardSnapshot& board, const Cell cell) {
	return board.cells[static_cast<std::size_t>(cell.y * board.width + cell.x)];
}

DropRequest Request(const DragOrigin origin, const Cell anchor, std::vector<Cell> footprint = { { 0, 0 } }) {
	DropRequest request;
	request.card_id = 4;
	request.origin = origin;
	request.candidate_anchor = anchor;
	request.original_anchor = { 2, 2 };
	request.footprint = std::move(footprint);
	request.pointer_on_board = true;
	return request;
}

void TestHandDrops() {
	auto board = MakeBoard();
	Expect(ResolveDrop(Request(DragOrigin::Hand, { 1, 1 }), board).result == DropResult::Place,
		"hand places on an empty cell");

	At(board, { 1, 1 }).occupant = 8;
	const DropDecision swap = ResolveDrop(Request(DragOrigin::Hand, { 1, 1 }), board);
	Expect((swap.result == DropResult::Swap) && (swap.target_card_id == 8),
		"hand swaps with one board card");

	At(board, { 2, 1 }).occupant = 9;
	Expect(ResolveDrop(Request(DragOrigin::Hand, { 1, 1 }, { { 0, 0 }, { 1, 0 } }), board).result
		== DropResult::ReturnToHand, "hand does not swap with multiple board cards");

	auto outside = Request(DragOrigin::Hand, { -1, 1 }, { { 0, 0 }, { 1, 0 } });
	Expect(ResolveDrop(outside, board).result == DropResult::ReturnToHand,
		"hand returns when the card extends past the left edge");
	outside.candidate_anchor = { 6, 1 };
	Expect(ResolveDrop(outside, board).result == DropResult::ReturnToHand,
		"hand returns when the card extends past the right edge");
	outside.candidate_anchor = { 1, -1 };
	outside.footprint = { { 0, 0 }, { 0, 1 } };
	Expect(ResolveDrop(outside, board).result == DropResult::ReturnToHand,
		"hand returns when the card extends past the top edge");
	outside.candidate_anchor = { 1, 5 };
	Expect(ResolveDrop(outside, board).result == DropResult::ReturnToHand,
		"hand returns when the card extends past the bottom edge");

	auto off_board_pointer = Request(DragOrigin::Hand, { 1, 1 });
	off_board_pointer.pointer_on_board = false;
	Expect(ResolveDrop(off_board_pointer, board).result == DropResult::ReturnToHand,
		"hand returns over non-board UI");

	At(board, { 3, 3 }).usable = false;
	Expect(ResolveDrop(Request(DragOrigin::Hand, { 3, 3 }), board).result == DropResult::ReturnToHand,
		"hand returns over an unusable cell");
}

void TestBoardDrops() {
	auto board = MakeBoard();
	At(board, { 2, 2 }).occupant = 4;
	At(board, { 3, 2 }).occupant = 4;

	Expect(ResolveDrop(Request(DragOrigin::Board, { 3, 2 }, { { 0, 0 }, { 1, 0 } }), board).result
		== DropResult::Place, "horizontal card moves by one cell while overlapping its old footprint");
	Expect(ResolveDrop(Request(DragOrigin::Board, { 2, 3 }, { { 0, 0 }, { 0, 1 } }), board).result
		== DropResult::Place, "vertical card moves by one cell");

	auto same = Request(DragOrigin::Board, { 2, 2 }, { { 0, 0 }, { 1, 0 } });
	Expect(ResolveDrop(same, board).result == DropResult::RestoreToBoard,
		"board card restores at its original anchor");
	same.candidate_anchor = { 3, 2 };
	same.near_start = true;
	Expect(ResolveDrop(same, board).result == DropResult::RestoreToBoard,
		"board card restores inside the cancellation distance");

	At(board, { 4, 2 }).occupant = 8;
	Expect(ResolveDrop(Request(DragOrigin::Board, { 3, 2 }, { { 0, 0 }, { 1, 0 } }), board).result
		== DropResult::RestoreToBoard, "board cards never swap on partial overlap");

	auto partial_outside_with_overlap = Request(DragOrigin::Board, { 6, 2 }, { { 0, 0 }, { 1, 0 } });
	At(board, { 6, 2 }).occupant = 8;
	Expect(ResolveDrop(partial_outside_with_overlap, board).result == DropResult::RestoreToBoard,
		"other-card overlap takes priority over a partial board exit");
	At(board, { 6, 2 }).occupant = EmptyCardId;
	Expect(ResolveDrop(partial_outside_with_overlap, board).result == DropResult::ReturnToHand,
		"board card returns to hand when partially outside");

	At(board, { 3, 3 }).usable = false;
	Expect(ResolveDrop(Request(DragOrigin::Board, { 3, 3 }), board).result == DropResult::ReturnToHand,
		"board card returns to hand over an unusable cell");

	auto cancelled = Request(DragOrigin::Board, { 4, 4 });
	cancelled.cancelled = true;
	Expect(ResolveDrop(cancelled, board).result == DropResult::RestoreToBoard,
		"focus loss restores a board drag");
	cancelled.origin = DragOrigin::Hand;
	Expect(ResolveDrop(cancelled, board).result == DropResult::ReturnToHand,
		"focus loss restores a hand drag");
}

void TestRotationAndFastRelease() {
	auto board = MakeBoard();
	auto horizontal = Request(DragOrigin::Hand, { 5, 1 }, { { 0, 0 }, { 1, 0 } });
	Expect(ResolveDrop(horizontal, board).result == DropResult::Place,
		"horizontal rotated footprint fits");
	auto vertical = Request(DragOrigin::Hand, { 6, 5 }, { { 0, 0 }, { 0, 1 } });
	Expect(ResolveDrop(vertical, board).result == DropResult::ReturnToHand,
		"vertical rotated footprint does not fit");

	auto release_frame = Request(DragOrigin::Hand, { -1, 2 });
	release_frame.pointer_on_board = false;
	Expect(ResolveDrop(release_frame, board).result == DropResult::ReturnToHand,
		"release-frame coordinates determine a fast invalid drop");
}

void TestInputOwnership() {
	Expect(CapturePointerOwner(true, false, true, true, true, true, true) == PointerInputOwner::Deck,
		"modal deck owns input");
	Expect(CapturePointerOwner(false, true, true, true, true, true, true) == PointerInputOwner::Card,
		"active drag owns input");
	Expect(CapturePointerOwner(false, false, true, true, true, true, true) == PointerInputOwner::Deck,
		"deck button wins initial hit priority");
	Expect(CapturePointerOwner(false, false, true, false, true, true, true) == PointerInputOwner::Attack,
		"attack button wins over cards");
	Expect(CapturePointerOwner(false, false, true, false, false, true, true) == PointerInputOwner::Card,
		"hand card wins over board card");
	Expect(CapturePointerOwner(false, false, false, true, true, true, true) == PointerInputOwner::None,
		"locked combat does not capture a new gesture");
	Expect(CanAcceptBattleInput(true, false, false), "idle battle accepts input");
	Expect(!CanAcceptBattleInput(false, false, false), "combat animation rejects input");
	Expect(!CanAcceptBattleInput(true, true, false), "board lock rejects input");
	Expect(!CanAcceptBattleInput(true, false, true), "scene transition rejects input");
	bool transition_started = false;
	Expect(BeginOneShotTransition(transition_started), "victory transition starts once");
	Expect(!BeginOneShotTransition(transition_started), "victory transition cannot start twice");
}

void TestStableIdentityAndReservations() {
	auto board = MakeBoard();
	At(board, { 2, 2 }).occupant = 42;
	Expect(OccupantsReferenceKnownCards(board, { 4, 42 }), "occupancy stores a known stable card id");
	Expect(!OccupantsReferenceKnownCards(board, { 4, 8 }), "orphan occupancy is rejected");

	std::vector<ReservedHandSlot> reservations{ { 4, 0 }, { 8, 1 } };
	Expect(HasUniqueReservedHandSlots(reservations), "reserved hand slots are unique");
	reservations[0].slot = 1;
	Expect(!HasUniqueReservedHandSlots(reservations), "duplicate reserved hand slots are rejected");

	board = MakeBoard();
	const BoardSnapshot before = board;
	(void)ResolveDrop(Request(DragOrigin::Hand, { 2, 2 }), board);
	Expect(board.cells == before.cells, "drop classification does not mutate unrelated board state");
}

void TestConcurrentReturnMotions() {
	CardLifecycle card_a = CardLifecycle::ReturningToHand;
	CardLifecycle card_b = CardLifecycle::InHand;
	VisualMotion motion_a;
	VisualMotion motion_b;
	StartVisualMotion(motion_a, { 800, 500 }, { 350, 900 });

	Expect(!CanStartCardDrag(card_a), "returning card ignores input");
	Expect(CanStartCardDrag(card_b), "another hand card remains interactive");
	Expect(!AdvanceVisualMotion(motion_a, 1.0 / 60.0), "return continues across multiple frames");
	const ScreenPoint a_after_first_frame = motion_a.current;
	Expect((a_after_first_frame != motion_a.start) && (a_after_first_frame != motion_a.end),
		"returning card has a visual-only intermediate position");

	card_b = CardLifecycle::DraggingFromHand;
	Expect(IsLogicallyInHand(card_a) && IsLogicallyInHand(card_b),
		"returning and dragging cards are both logically in hand");
	StartVisualMotion(motion_b, { 900, 450 }, { 425, 900 });
	card_b = CardLifecycle::ReturningToHand;

	for (int frame = 0; frame < 9; frame++) {
		if (AdvanceVisualMotion(motion_a, 1.0 / 60.0)) SettleReturnLifecycle(card_a);
		if (AdvanceVisualMotion(motion_b, 1.0 / 60.0)) SettleReturnLifecycle(card_b);
	}
	Expect((card_a == CardLifecycle::InHand) && (motion_a.current == ScreenPoint{ 350, 900 }),
		"first card snaps exactly to its reserved hand position");
	Expect((card_b == CardLifecycle::InHand) && (motion_b.current == ScreenPoint{ 425, 900 }),
		"second card returns independently to a different hand position");
}

void TestForcedMotionCompletion() {
	CardLifecycle hand_card = CardLifecycle::ReturningToHand;
	CardLifecycle board_card = CardLifecycle::ReturningToBoard;
	VisualMotion hand_motion;
	VisualMotion board_motion;
	StartVisualMotion(hand_motion, { 700, 400 }, { 350, 900 });
	StartVisualMotion(board_motion, { 900, 600 }, { 735, 305 });
	AdvanceVisualMotion(hand_motion, 1.0 / 60.0);
	AdvanceVisualMotion(board_motion, 1.0 / 60.0);

	CompleteVisualMotion(hand_motion);
	CompleteVisualMotion(board_motion);
	SettleReturnLifecycle(hand_card);
	SettleReturnLifecycle(board_card);
	Expect((hand_card == CardLifecycle::InHand) && (hand_motion.current == hand_motion.end),
		"focus loss or scene transition completes a hand return");
	Expect((board_card == CardLifecycle::OnBoard) && (board_motion.current == board_motion.end),
		"focus loss or scene transition completes a board return");
}

} // namespace

int main() {
	TestHandDrops();
	TestBoardDrops();
	TestRotationAndFastRelease();
	TestInputOwnership();
	TestStableIdentityAndReservations();
	TestConcurrentReturnMotions();
	TestForcedMotionCompletion();
	if (failures != 0) return EXIT_FAILURE;
	std::cout << "All battle card interaction tests passed\n";
	return EXIT_SUCCESS;
}
