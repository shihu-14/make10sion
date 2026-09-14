#include "../src/BattleCardRules.hpp"
#include "../src/BattleDamageRules.hpp"
#include "../src/BattleLayoutRules.hpp"
#include "../src/BoardCalculationRules.hpp"
#include "../src/CardSymbolRules.hpp"
#include "../src/DebugScenarioRules.hpp"
#include "../src/EnemyIntentRules.hpp"
#include "../src/GameStateRules.hpp"
#include "../src/ShopRules.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

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
		== DropResult::BoardSwap, "one overlapping board card becomes a swap candidate");

	auto partial_outside_with_overlap = Request(DragOrigin::Board, { 6, 2 }, { { 0, 0 }, { 1, 0 } });
	At(board, { 6, 2 }).occupant = 8;
	Expect(ResolveDrop(partial_outside_with_overlap, board).result == DropResult::BoardSwap,
		"one board-card overlap is classified before reciprocal placement validation");
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
	Expect(ShouldRotateDraggedCard(true, true),
		"R rotates a card while a drag is active");
	Expect(!ShouldRotateDraggedCard(false, true) && !ShouldRotateDraggedCard(true, false),
		"rotation input is ignored outside an active drag or without an R press");
	Expect(!CanResetCardRotation(true),
		"an occupied board card cannot reset its rotation before detach");
	Expect(CanResetCardRotation(false),
		"a board card can reset its rotation after its occupancy is cleared");
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

void TestBoardCardSwapRules() {
	auto board = MakeBoard();
	At(board, { 1, 1 }).occupant = 4;
	At(board, { 2, 1 }).occupant = 4;
	At(board, { 4, 2 }).occupant = 8;
	At(board, { 4, 3 }).occupant = 8;
	At(board, { 4, 4 }).occupant = 8;

	const auto overlap = Request(DragOrigin::Board, { 4, 2 }, { { 0, 0 }, { 1, 0 } });
	const DropDecision decision = ResolveDrop(overlap, board);
	Expect((decision.result == DropResult::BoardSwap) && (decision.target_card_id == 8),
		"a board card overlapping one board card becomes a board swap candidate");

	BoardSwapRequest swap;
	swap.first_card_id = 4;
	swap.first_destination_anchor = { 4, 2 };
	swap.first_footprint = { { 0, 0 }, { 1, 0 } };
	swap.second_card_id = 8;
	swap.second_destination_anchor = { 1, 1 };
	swap.second_footprint = { { 0, 0 }, { 0, 1 }, { 0, 2 } };
	Expect(CanSwapBoardCards(swap, board),
		"different board card shapes can swap when both destinations are legal");

	At(board, { 5, 2 }).occupant = 12;
	Expect(!CanSwapBoardCards(swap, board),
		"a board swap fails when either destination collides with a third card");
	At(board, { 5, 2 }).occupant = EmptyCardId;
	At(board, { 1, 2 }).usable = false;
	Expect(!CanSwapBoardCards(swap, board),
		"a board swap fails when either destination uses a locked cell");

	Expect(ShouldUseAutoRotatedPlacement(DropResult::ReturnToHand, DropResult::Place),
		"an invalid placement may use one legal automatic quarter-turn");
	Expect(!ShouldUseAutoRotatedPlacement(DropResult::BoardSwap, DropResult::Place),
		"a board swap candidate is never silently replaced by automatic rotation");
}

void TestDamageHitRules() {
	using namespace BattleDamageRules;
	Expect(HitCountForDamage(15, 100) == 1, "fifteen percent damage uses one hit");
	Expect(HitCountForDamage(16, 100) == 2, "damage above fifteen percent uses two hits");
	Expect(HitCountForDamage(30, 100) == 2, "thirty percent damage uses two hits");
	Expect(HitCountForDamage(31, 100) == 3, "damage above thirty percent uses three hits");
	Expect(HitCountForDamage(45, 100) == 3, "forty-five percent damage uses three hits");
	Expect(HitCountForDamage(46, 100) == 4, "damage above forty-five percent uses four hits");
	Expect(HitCountForDamage(60, 100) == 4, "sixty percent damage uses four hits");
	Expect(HitCountForDamage(61, 100) == 5, "damage above sixty percent uses five hits");

	const auto hits = SplitDamage(17, 100);
	Expect((hits.size() == 2) && (hits[0] == 9) && (hits[1] == 8),
		"actual damage is split as evenly as possible");
	int32_t total = 0;
	for (const auto hit : hits) total += hit;
	Expect(total == 17, "split hit damage sums exactly to the actual damage");
	const auto tiny_hits = SplitDamage(2, 2);
	Expect((tiny_hits.size() == 2) && (tiny_hits[0] == 1) && (tiny_hits[1] == 1),
		"hit count never creates zero-damage effects");

	int32_t hp = 5;
	int32_t applied_hits = 0;
	for (const int32_t hit : std::array<int32_t, 3>{ 4, 4, 4 }) {
		hp = ApplyHit(hp, hit);
		++applied_hits;
		if (!ShouldContinueHits(hp)) break;
	}
	Expect((hp == 0) && (applied_hits == 2),
		"remaining hit effects stop immediately when HP reaches zero");
	Expect(!ShouldDrawHitEffect(0, 120, 180),
		"zero damage does not redraw a stale hit effect position");
	Expect(!ShouldDrawHitEffect(1, -1, 180) && !ShouldDrawHitEffect(1, 120, -1),
		"a hit effect requires a valid screen position");
	Expect(ShouldDrawHitEffect(1, 120, 180),
		"a real damage hit with a valid position draws its effect");
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
	Expect(CanProcessBoardInput(true, false, PointerInputOwner::Card),
		"card owner can continue an active board drag");
	Expect(CanProcessBoardInput(true, false, PointerInputOwner::None),
		"unowned input can reach an empty board cell");
	Expect(!CanProcessBoardInput(true, false, PointerInputOwner::Deck),
		"deck-owned input does not reach the board");
	Expect(!CanProcessBoardInput(true, true, PointerInputOwner::Card),
		"failed hand capture does not also start a board drag");
	bool transition_started = false;
	Expect(BeginOneShotTransition(transition_started), "victory transition starts once");
	Expect(!BeginOneShotTransition(transition_started), "victory transition cannot start twice");
}

void TestInteractionDrawLayers() {
	Expect(GetCardDrawLayer(CardLifecycle::InDeck) == CardDrawLayer::Hidden,
		"deck cards are hidden from battle card layers");
	Expect(GetCardDrawLayer(CardLifecycle::InHand) == CardDrawLayer::StaticHand,
		"hand cards use the static hand layer");
	Expect(GetCardDrawLayer(CardLifecycle::OnBoard) == CardDrawLayer::StaticBoard,
		"board cards use the static board layer");
	Expect(GetCardDrawLayer(CardLifecycle::ReturningToHand) == CardDrawLayer::ReturningOverlay,
		"hand returns use the interaction overlay");
	Expect(GetCardDrawLayer(CardLifecycle::ReturningToBoard) == CardDrawLayer::ReturningOverlay,
		"board returns use the interaction overlay");
	Expect(GetCardDrawLayer(CardLifecycle::DraggingFromHand) == CardDrawLayer::DraggingOverlay,
		"hand drags use the top interaction layer");
	Expect(GetCardDrawLayer(CardLifecycle::DraggingFromBoard) == CardDrawLayer::DraggingOverlay,
		"board drags use the top interaction layer");
	Expect(!CanStartCardDrag(CardLifecycle::ReturningToBoard),
		"returning board visuals are excluded from hit testing");
	Expect(CanBeHandSwapTarget(CardLifecycle::OnBoard),
		"a settled board card remains a valid hand swap target");
	Expect(!CanBeHandSwapTarget(CardLifecycle::ReturningToBoard),
		"another card cannot change a returning card's destination");
	Expect(CapturePointerOwner(false, false, true, false, false, false, true)
		== PointerInputOwner::Card,
		"a board card under a returning visual can own input");
	Expect(CapturePointerOwner(false, false, true, true, false, false, true)
		== PointerInputOwner::Deck,
		"UI under a returning visual keeps its normal priority");
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

void TestRepeatedFailedBoardSwapReturn() {
	for (int iteration = 0; iteration < 500; iteration++) {
		auto board = MakeBoard();
		At(board, { 2, 2 }).occupant = 4;
		At(board, { 3, 2 }).occupant = 4;
		At(board, { 4, 2 }).occupant = 8;
		At(board, { 5, 2 }).occupant = 12;
		const BoardSnapshot before = board;

		const DropDecision decision = ResolveDrop(
			Request(DragOrigin::Board, { 3, 2 }, { { 0, 0 }, { 1, 0 } }), board);
		Expect(decision.result == DropResult::BoardSwap,
			"board overlap resolves to a board swap candidate");
		BoardSwapRequest swap;
		swap.first_card_id = 4;
		swap.first_destination_anchor = { 4, 2 };
		swap.first_footprint = { { 0, 0 }, { 1, 0 } };
		swap.second_card_id = 8;
		swap.second_destination_anchor = { 2, 2 };
		swap.second_footprint = { { 0, 0 } };
		Expect(!CanSwapBoardCards(swap, board),
			"an illegal reciprocal placement restores the dragged board card");

		CardLifecycle returning_card = CardLifecycle::ReturningToBoard;
		CardLifecycle other_card = CardLifecycle::OnBoard;
		VisualMotion return_motion;
		StartVisualMotion(return_motion, { 900, 520 }, { 825, 395 });
		const Cell other_anchor{ 4, 2 };
		const int other_rotation = 1;
		const int other_hand_slot = 3;

		for (int frame = 0; frame < 10; frame++) {
			if (frame == 1) {
				Expect(CanStartCardDrag(other_card),
					"another board card remains draggable during a return");
				other_card = CardLifecycle::DraggingFromBoard;
			}
			Expect(CanProcessBoardInput(true, false, PointerInputOwner::Card),
				"returning visual does not roll back another active drag");
			if (AdvanceVisualMotion(return_motion, 1.0 / 60.0)) {
				SettleReturnLifecycle(returning_card);
			}
			Expect(board.cells == before.cells,
				"board restore preserves all logical occupancy while animating");
			Expect((other_anchor == Cell{ 4, 2 }) && (other_rotation == 1) && (other_hand_slot == 3),
				"board restore does not mutate the other card");
		}
		Expect((returning_card == CardLifecycle::OnBoard)
			&& (return_motion.current == ScreenPoint{ 825, 395 }),
			"board return ends exactly at its original screen position");
		Expect(GetCardDrawLayer(other_card) == CardDrawLayer::DraggingOverlay,
			"active drag remains above the returning overlay");
	}
}

void TestBoardProgress() {
	using namespace GameStateRules;
	BoardProgress progress;
	Expect(progress.UnlockedCount() == 6, "board progress starts with six cells");
	Expect(CalculateHandLimit(progress) == 5, "six cells allow five hand cards");
	Expect(progress.IsUnlockable({ 2, 1 }), "cell above the initial board is unlockable");
	Expect(!progress.IsUnlockable({ 0, 0 }), "detached cell is not unlockable");
	Expect(progress.Unlock({ 2, 1 }), "first event cell unlock succeeds");
	Expect(!progress.Unlock({ 2, 1 }), "same event cell cannot unlock twice");
	Expect(progress.Unlock({ 3, 1 }), "second event cell unlock succeeds");
	Expect((progress.UnlockedCount() == 8) && (CalculateHandLimit(progress) == 6),
		"two event cells increase the next hand limit");
	Expect(ActIndex(0) == 0, "first act index starts at zero");
	Expect(ActIndex(10) == 1, "second act index starts at layer ten");
	Expect((ActIndex(29) == 2) && (FloorInAct(29) == 9),
		"final boss uses act two floor nine");
	const auto second_act = ResolveVictory(2, 9);
	Expect((second_act.destination == VictoryDestination::NextActBattle)
		&& (second_act.next_layer == 10), "first boss advances directly to layer ten");
	const auto third_act = ResolveVictory(2, 19);
	Expect((third_act.destination == VictoryDestination::NextActBattle)
		&& (third_act.next_layer == 20), "second boss advances directly to layer twenty");
	Expect(ResolveVictory(2, 29).destination == VictoryDestination::Result,
		"final boss produces the clear result");
	Expect(ResolveVictory(0, 29).destination == VictoryDestination::Map,
		"non-boss victory does not finish the run");
	BoardProgress fully_unlocked;
	while (true) {
		const auto candidates = fully_unlocked.UnlockableCells();
		if (candidates.empty()) break;
		Expect(fully_unlocked.Unlock(candidates.front()), "each reachable board cell unlocks once");
	}
	Expect(fully_unlocked.UnlockedCount() == BoardProgress::CellCount,
		"board progress can unlock every cell without stale counts");
	Expect(CalculateHandLimit(fully_unlocked) == 18, "hand limit is capped at eighteen");
	Expect(ResolveBattleHandLimit(fully_unlocked, 0) == 18,
		"normal battles use the eighteen-card maximum");
	Expect(ResolveBattleHandLimit(fully_unlocked, 18) == 18,
		"an explicit debug override can expose eighteen hand cards");
	BoardProgress debug_progress;
	debug_progress.UnlockAll();
	Expect((debug_progress.UnlockedCount() == BoardProgress::CellCount)
		&& (CalculateHandLimit(debug_progress) == 18),
		"debug board progress unlocks all forty-two cells directly");
	Expect(ElapsedMillis(150, 100) == 50, "elapsed milliseconds preserve unsigned precision");
	Expect(ElapsedMillis(50, 100) == 0, "clock rollback cannot underflow elapsed milliseconds");
}

void TestBattleDeckState() {
	using namespace GameStateRules;
	BattleDeckState deck;
	deck.Initialize(4, { 2, 0, 3, 1 });
	Expect(deck.Validate(), "battle deck starts with one zone per card");
	Expect(deck.CanMove(1, CardZone::DrawPile, CardZone::Hand),
		"zone transition can be validated without mutating the deck");
	Expect(deck.ZoneOf(1) == CardZone::DrawPile,
		"zone transition preflight leaves the card in its original zone");
	Expect(!deck.CanMove(1, CardZone::Hand, CardZone::Board),
		"zone transition preflight rejects an incorrect source zone");
	Expect(deck.Move(1, CardZone::DrawPile, CardZone::Hand), "draw moves one card to hand");
	Expect(deck.Move(1, CardZone::Hand, CardZone::Board), "placement moves hand card to board");
	Expect(deck.Move(1, CardZone::Board, CardZone::Discard), "turn end discards board card");
	Expect(deck.Move(3, CardZone::DrawPile, CardZone::Discard), "second card moves to discard");
	Expect(deck.Move(0, CardZone::DrawPile, CardZone::Discard), "third card moves to discard");
	Expect(deck.Move(2, CardZone::DrawPile, CardZone::Discard), "fourth card moves to discard");
	Expect(deck.RecycleDiscard(), "discard recycles only when all other zones are empty");
	Expect(deck.Validate() && (deck.Cards(CardZone::DrawPile).size() == 4),
		"recycled deck keeps every card exactly once");
	Expect(NeedsBoardDetach(CardZone::Hand, CardZone::Discard),
		"discarding a hand card detaches its Board registration first");
	Expect(NeedsBoardDetach(CardZone::Board, CardZone::Discard),
		"discarding a board card detaches its Board registration first");
	Expect(!NeedsBoardDetach(CardZone::Hand, CardZone::Board),
		"placing a hand card keeps the same Board registration");
	Expect(!NeedsBoardDetach(CardZone::Board, CardZone::Hand),
		"returning a board card keeps its reserved hand registration");
}

void TestTurnDrawRefreshRules() {
	using namespace GameStateRules;
	Expect(RequiredBoardFillCells(6) == 3 && RequiredBoardFillCells(7) == 4
		&& RequiredBoardFillCells(BoardProgress::CellCount) == 21,
		"turn refresh requires half the unlocked board rounded up");
	Expect(CountOccupiedCells("12\n3$") == 3,
		"occupied-cell counting excludes holes and line separators");
	Expect(ShouldRefreshDrawPile(2, 8, 6),
		"a draw pile below the playable-cell threshold refreshes when the full deck can satisfy it");
	Expect(!ShouldRefreshDrawPile(3, 8, 6),
		"a draw pile at the threshold is kept");
	Expect(!ShouldRefreshDrawPile(1, 2, 6),
		"an undersized full deck does not trigger an endless refresh");

	BattleDeckState deck;
	deck.Initialize(4, { 3, 2, 1, 0 });
	Expect(deck.Move(0, CardZone::DrawPile, CardZone::Hand)
		&& !deck.RefreshDrawPile({ 3, 2, 1, 0 }),
		"a refresh is rejected while a card is still in hand");
	Expect(deck.Move(0, CardZone::Hand, CardZone::Discard)
		&& deck.Move(1, CardZone::DrawPile, CardZone::Discard),
		"turn cards can enter discard before refresh");
	Expect(deck.RefreshDrawPile({ 3, 2, 1, 0 }),
		"a turn-boundary refresh rebuilds the full draw pile in the requested order");
	Expect(deck.Validate() && deck.Cards(CardZone::Discard).empty()
		&& deck.Cards(CardZone::DrawPile) == std::vector<int32_t>{ 3, 2, 1, 0 },
		"refresh preserves every card exactly once and supports fixed debug order");
}

void TestAudioSettingsRules() {
	using namespace GameStateRules;
	AudioSettings settings;
	Expect(settings.bgm_volume == 1.0 && settings.se_volume == 1.0,
		"shared BGM and SE settings start independently at one hundred percent");
	settings.bgm_volume = 0.25;
	Expect(settings.se_volume == 1.0,
		"changing BGM does not change the independent SE volume");
	Expect(ClampVolume(-0.5) == 0.0 && ClampVolume(1.5) == 1.0,
		"shared audio volumes stay inside zero and one");
	Expect(VolumePercent(0.0) == 0 && VolumePercent(0.555) == 56
		&& VolumePercent(1.0) == 100,
		"shared audio volumes produce rounded percentage labels");
	Expect(SliderVolumeAt(50.0, 100.0, 400.0) == 0.0
		&& SliderVolumeAt(300.0, 100.0, 400.0) == 0.5
		&& SliderVolumeAt(600.0, 100.0, 400.0) == 1.0,
		"volume sliders clamp pointer positions to their tracks");
}

void TestCardSymbolRules() {
	using namespace CardSymbolRules;
	for (char symbol = '0'; symbol <= '7'; ++symbol) {
		const auto definition = Decode(symbol);
		Expect(definition.kind == Kind::Number
			&& definition.current_value == (symbol - '0')
			&& definition.visual_number == (symbol - '0'),
			"every supported digit has matching logical and visual values");
	}
	for (char symbol = 'A'; symbol <= 'H'; ++symbol) {
		const auto definition = Decode(symbol);
		Expect(definition.kind == Kind::OccupiedOnly
			&& definition.visual_number == (symbol - 'A'),
			"every uppercase symbol is occupied-only with its faded visual value");
	}
	for (char symbol = 'a'; symbol <= 'q'; ++symbol) {
		Expect(Decode(symbol).IsKnown(), "every special symbol from a through q is defined");
	}
	Expect(Decode('+').operation == Operator::Add
		&& Decode('-').operation == Operator::Subtract
		&& Decode('*').operation == Operator::Multiply
		&& Decode('/').operation == Operator::Divide,
		"all four arithmetic operators have explicit definitions");
	const auto zero = Decode('0');
	Expect(zero.kind == Kind::Number && zero.current_value == 0,
		"digit zero remains a numeric value");
	const auto q = Decode('q');
	Expect(q.kind == Kind::Number && q.current_value == 1 && q.next_turn_bonus == 1,
		"q is one now and adds one to the same cell next turn");
	const auto faded_four = Decode('E');
	Expect(faded_four.kind == Kind::OccupiedOnly && faded_four.visual_number == 4,
		"uppercase E is a faded occupied-only four");
	Expect(Decode('$').kind == Kind::Hole, "dollar is a non-occupying hole");
	Expect(Decode('a').row_multiplier == 1.0
		&& Decode('b').row_multiplier == 1.5
		&& Decode('c').row_multiplier == 2.0,
		"row multiplier symbols retain their existing values");
	Expect(Decode('f').row_mode == RowMode::Defense
		&& Decode('i').row_mode == RowMode::Attack,
		"row mode symbols retain their existing meanings");
	Expect(Decode('j').current_value == 1 && Decode('j').next_turn_bonus == 2
		&& Decode('k').current_value == 2 && Decode('k').next_turn_bonus == 2
		&& Decode('l').current_value == 2 && Decode('l').next_turn_bonus == 4
		&& Decode('o').current_value == 2 && Decode('o').next_turn_bonus == 1
		&& Decode('p').current_value == 3 && Decode('p').next_turn_bonus == 1,
		"all next-turn symbols retain their current and delayed values");
	Expect(Decode('?').kind == Kind::Unknown, "unknown symbols are never converted through ASCII");
	Expect(IsValidCardDefinition("q\nE"), "known rectangular card definition is accepted");
	Expect(!IsValidCardDefinition("8"), "unsupported numeric symbol is rejected");
	Expect(!IsValidCardDefinition("12\n3"), "ragged card definition is rejected");
}

void TestBoardCalculationRules() {
	using namespace BoardCalculationRules;
	using Usage = ExpressionCellUsage;
	Board board{ 7, 6 };
	board.Set(0, 0, '7');
	board.Set(1, 0, '*');
	board.Set(2, 0, '0');
	Expect(Evaluate(board).row_values[0] == 0, "seven multiplied by zero is zero");

	board = Board{ 7, 6 };
	board.Set(0, 0, '2');
	board.Set(1, 0, '+');
	board.Set(2, 0, '3');
	board.Set(3, 0, '*');
	board.Set(4, 0, '4');
	Expect(Evaluate(board).row_values[0] == 14,
		"typed evaluation preserves multiplication precedence");

	board = Board{ 7, 6 };
	board.Set(0, 0, '7');
	board.Set(1, 0, '/');
	board.Set(2, 0, '0');
	const auto divided_by_zero = Evaluate(board);
	Expect((divided_by_zero.row_values[0] == 0) && !divided_by_zero.row_valid[0],
		"division by zero makes only that row zero and records invalidity");
	board.Set(0, 1, '3');
	board.Set(1, 1, '+');
	board.Set(2, 1, '4');
	Expect(Evaluate(board).row_values[1] == 7, "another valid row survives an invalid row");

	board = Board{ 7, 6 };
	board.Set(0, 0, '2');
	board.Set(1, 0, '3');
	const auto adjacent = Evaluate(board);
	Expect((adjacent.row_values[0] == 2) && adjacent.row_valid[0],
		"an adjacent second number is ignored as in the historical evaluator");

	board = Board{ 7, 6 };
	board.Set(0, 0, '+');
	board.Set(1, 0, '2');
	board.Set(2, 0, '+');
	board.Set(3, 0, '*');
	board.Set(4, 0, '3');
	board.Set(5, 0, '+');
	const auto tolerant = Evaluate(board);
	Expect((tolerant.row_values[0] == 5) && tolerant.row_valid[0],
		"leading repeated and trailing operators are ignored as in the historical evaluator");
	Expect(tolerant.ExpressionUsageAt(0, 0) == Usage::Ignored
		&& tolerant.ExpressionUsageAt(1, 0) == Usage::Used
		&& tolerant.ExpressionUsageAt(2, 0) == Usage::Used
		&& tolerant.ExpressionUsageAt(3, 0) == Usage::Ignored
		&& tolerant.ExpressionUsageAt(4, 0) == Usage::Used
		&& tolerant.ExpressionUsageAt(5, 0) == Usage::Ignored,
		"tolerant evaluation reports which operators and numbers actually form the expression");

	board = Board{ 7, 6 };
	board.Set(0, 0, '2');
	board.Set(1, 0, '3');
	board.Set(2, 0, '4');
	board.Set(3, 0, '+');
	board.Set(4, 0, '5');
	const auto repeated_numbers = Evaluate(board);
	Expect(repeated_numbers.row_values[0] == 7
		&& repeated_numbers.ExpressionUsageAt(0, 0) == Usage::Used
		&& repeated_numbers.ExpressionUsageAt(1, 0) == Usage::Ignored
		&& repeated_numbers.ExpressionUsageAt(2, 0) == Usage::Ignored
		&& repeated_numbers.ExpressionUsageAt(3, 0) == Usage::Used
		&& repeated_numbers.ExpressionUsageAt(4, 0) == Usage::Used,
		"adjacent numbers expose their historical used and ignored classification");

	board = Board{ 7, 6 };
	board.Set(0, 0, '2');
	board.Set(1, 0, '+');
	board.Set(2, 0, 'g');
	board.Set(3, 0, '+');
	board.Set(4, 0, '3');
	Expect(Evaluate(board).row_values[0] == 5,
		"tokens after an aggregate operand follow the historical tolerant evaluator");

	board = Board{ 7, 6 };
	board.Set(0, 0, '0');
	board.Set(0, 1, '4');
	board.Set(0, 2, 'g');
	board.Set(0, 3, 'h');
	board.Set(0, 4, 'e');
	const auto aggregate = Evaluate(board);
	Expect(aggregate.row_values[2] == 4, "Max uses numeric cells from the whole board");
	Expect(aggregate.row_values[3] == 0, "Min includes numeric zero");
	Expect(aggregate.row_values[4] == 2, "Ave uses the whole-board population and truncates toward zero");
	Expect(aggregate.ExpressionUsageAt(0, 2) == Usage::Used
		&& aggregate.ExpressionUsageAt(0, 3) == Usage::Used
		&& aggregate.ExpressionUsageAt(0, 4) == Usage::Used,
		"accepted aggregate cards are reported as used expression cells");
	for (const char aggregate_symbol : { 'g', 'h', 'e' }) {
		Board ignored_aggregate{ 2, 1 };
		ignored_aggregate.Set(0, 0, '2');
		ignored_aggregate.Set(1, 0, aggregate_symbol);
		Expect(Evaluate(ignored_aggregate).ExpressionUsageAt(1, 0) == Usage::Ignored,
			"an aggregate skipped by expression construction is reported as ignored");
	}

	board = Board{ 7, 6 };
	for (int32_t x = 0; x < 7; ++x) board.Set(x, 0, "abcfiA$"[x]);
	for (int32_t x = 0; x < 7; ++x) board.Set(x, 1, "BCDEFGH"[x]);
	const auto non_expression = Evaluate(board);
	bool all_non_expression = true;
	for (int32_t y = 0; y < 2; ++y) {
		for (int32_t x = 0; x < 7; ++x) {
			all_non_expression = all_non_expression
				&& (non_expression.ExpressionUsageAt(x, y) == Usage::NonExpression);
		}
	}
	Expect(all_non_expression
		&& non_expression.ExpressionUsageAt(-1, 0) == Usage::NonExpression
		&& non_expression.ExpressionUsageAt(7, 0) == Usage::NonExpression,
		"multipliers row modes occupied-only cells holes and out-of-range lookups are not expression failures");

	board = Board{ 7, 6 };
	for (int32_t x = 0; x < 6; ++x) {
		board.Set(x, 0, "jklopq"[x]);
	}
	const auto delayed_usage = Evaluate(board);
	Expect(delayed_usage.ExpressionUsageAt(0, 0) == Usage::Used
		&& delayed_usage.ExpressionUsageAt(1, 0) == Usage::Ignored
		&& delayed_usage.ExpressionUsageAt(2, 0) == Usage::Ignored
		&& delayed_usage.ExpressionUsageAt(3, 0) == Usage::Ignored
		&& delayed_usage.ExpressionUsageAt(4, 0) == Usage::Ignored
		&& delayed_usage.ExpressionUsageAt(5, 0) == Usage::Ignored,
		"next-turn cards retain ordinary number participation in expression construction");

	board = Board{ 7, 6 };
	board.Set(0, 0, 'a');
	board.Set(1, 0, 'c');
	board.Set(2, 0, 'b');
	Expect(Evaluate(board).row_multiplier_effects[0] == 1.5,
		"the rightmost row multiplier keeps the historical scan-order priority");
	board.Set(3, 0, 'a');
	Expect(Evaluate(board).row_multiplier_effects[0] == 1.0,
		"a later smaller multiplier still overwrites earlier multipliers");

	board = Board{ 7, 6 };
	board.Set(0, 0, 'i');
	board.Set(1, 0, 'f');
	Expect(Evaluate(board).row_modes[0] == 0,
		"row mode conflict retains the current rightmost-symbol behavior");
	board.Set(2, 0, 'i');
	Expect(Evaluate(board).row_modes[0] == 1,
		"a later attack symbol remains the current winner");

	board = Board{ 7, 6 };
	board.Set(0, 0, 'E');
	board.Set(1, 0, '+');
	board.Set(2, 0, '6');
	Expect(Evaluate(board).row_values[0] == 6,
		"an occupied-only uppercase cell is skipped instead of becoming an ASCII number");

	Expect(!CheckedRowContribution(std::numeric_limits<int32_t>::max(), 2.0),
		"out-of-range row contribution is rejected");
	board = Board{ 7, 6 };
	board.Set(0, 0, '0', std::numeric_limits<int32_t>::max());
	board.Set(1, 0, '*');
	board.Set(2, 0, '2');
	board.Set(3, 0, '/');
	board.Set(4, 0, '2');
	const auto intermediate_overflow = Evaluate(board);
	Expect((intermediate_overflow.row_values[0] == 0)
		&& !intermediate_overflow.row_valid[0],
		"an out-of-range intermediate result invalidates the row even if it later returns to range");
	board = Board{ 7, 6 };
	board.Set(0, 0, '0');
	board.Set(1, 0, '*');
	board.Set(2, 0, '7', std::numeric_limits<int32_t>::max());
	const auto invalid_operand = Evaluate(board);
	Expect((invalid_operand.row_values[0] == 0) && !invalid_operand.row_valid[0],
		"an out-of-range operand is rejected before multiplication can hide it");
	Expect(invalid_operand.ExpressionUsageAt(0, 0) == Usage::Used
		&& invalid_operand.ExpressionUsageAt(1, 0) == Usage::Used
		&& invalid_operand.ExpressionUsageAt(2, 0) == Usage::Used,
		"evaluation failure does not relabel expression tokens as ignored");
	Expect(FinalRowMultiplier(2.0, 1.5) == 3.5,
		"UI and confirmation share the final row multiplier rule");
	const auto fresh_evaluation = Evaluate(Board{ 7, 6 });
	Expect(fresh_evaluation.ExpressionUsageAt(0, 0) == Usage::NonExpression,
		"a fresh evaluation never retains cell usage from an earlier board");
	int32_t total = std::numeric_limits<int32_t>::max();
	Expect(!CheckedAdd(total, 1) && total == std::numeric_limits<int32_t>::max(),
		"overflowing total is rejected without changing the prior total");
}

void TestDelayedEffectLifecycle() {
	using namespace BoardCalculationRules;
	struct DelayedCase {
		char symbol;
		int32_t current_value;
		int32_t next_turn_bonus;
	};
	constexpr std::array cases{
		DelayedCase{ 'q', 1, 1 },
		DelayedCase{ 'j', 1, 2 },
		DelayedCase{ 'k', 2, 2 },
		DelayedCase{ 'l', 2, 4 },
		DelayedCase{ 'o', 2, 1 },
		DelayedCase{ 'p', 3, 1 },
	};
	for (const auto& delayed : cases) {
		std::array<int32_t, 2> active{};
		std::array<int32_t, 2> current{};
		std::array<int32_t, 2> committed{};
		const auto definition = CardSymbolRules::Decode(delayed.symbol);
		Board current_turn{ 2, 1 };
		current_turn.Set(0, 0, delayed.symbol);
		const auto current_result = Evaluate(current_turn);
		Expect(current_result.row_values[0] == delayed.current_value
			&& current_result.ExpressionUsageAt(0, 0) == ExpressionCellUsage::Used,
			"a delayed card contributes its documented current-turn value");
		current[0] = definition.next_turn_bonus;
		CommitDelayedEffects(current, committed);
		current[0] = 0;
		AdvanceDelayedEffects(active, current, committed);

		Board next_turn{ 2, 1 };
		next_turn.Set(0, 0, '5', active[0]);
		const auto next_result = Evaluate(next_turn);
		Expect(definition.current_value == delayed.current_value
			&& active[0] == delayed.next_turn_bonus
			&& next_result.row_values[0] == (5 + delayed.next_turn_bonus),
			"a committed delayed card affects the same cell on exactly the next turn");

		CommitDelayedEffects(current, committed);
		AdvanceDelayedEffects(active, current, committed);
		Board following_turn{ 2, 1 };
		following_turn.Set(0, 0, '5', active[0]);
		Expect(Evaluate(following_turn).row_values[0] == 5,
			"a delayed bonus expires after one turn unless generated again");
	}

	std::array<int32_t, 2> active{};
	std::array<int32_t, 2> current{ 1, 0 };
	std::array<int32_t, 2> committed{};
	current[0] = 0;
	CommitDelayedEffects(current, committed);
	AdvanceDelayedEffects(active, current, committed);
	Expect(active[0] == 0, "a delayed card returned to hand before confirmation leaves no bonus");

	current[0] = 1;
	CommitDelayedEffects(current, committed);
	current[0] = 0;
	AdvanceDelayedEffects(active, current, committed);
	Board other_cell{ 2, 1 };
	other_cell.Set(1, 0, '5', active[1]);
	Expect(Evaluate(other_cell).row_values[0] == 5,
		"a delayed bonus never moves to another board cell");

	for (const char aggregate : { 'g', 'h', 'e' }) {
		Board aggregate_board{ 2, 1 };
		aggregate_board.Set(0, 0, aggregate, active[0]);
		aggregate_board.Set(1, 0, '5');
		Expect(Evaluate(aggregate_board).row_values[0] == 5,
			"Max Min and Ave ignore a positional delayed bonus on a non-number cell");
	}
	Board operator_board{ 2, 1 };
	operator_board.Set(0, 0, '+', active[0]);
	operator_board.Set(1, 0, '5');
	Expect(Evaluate(operator_board).row_values[0] == 5,
		"an operator cell ignores its positional delayed bonus");

	GameStateRules::BattleDeckState deck;
	deck.Initialize(1);
	Expect(deck.Move(0, GameStateRules::CardZone::DrawPile, GameStateRules::CardZone::Hand)
		&& deck.Move(0, GameStateRules::CardZone::Hand, GameStateRules::CardZone::Board)
		&& deck.Move(0, GameStateRules::CardZone::Board, GameStateRules::CardZone::Discard)
		&& deck.Validate(),
		"committing delayed effects does not weaken Board to Discard zone invariants");
}

void TestEnemyIntentRules() {
	using namespace EnemyIntentRules;
	TurnState turn_state;
	PrepareAction(turn_state, -16, 4);
	PrepareAction(turn_state, -18, 9);
	Expect(turn_state.raw_attack == -16 && turn_state.raw_defense == 4,
		"the selected enemy action remains fixed for the entire turn");
	bool prepared = false;
	const EnemyIntentRules::Request steal{ -13, 5, 15, 10 };
	const auto first = ResolveFrame(prepared, steal);
	const auto second = ResolveFrame(prepared, steal);
	Expect(first.attack == 40 && first.defense == 5 && first.money_delta == -30,
		"money-steal intent resolves to one bounded state change");
	Expect(second.attack == 40 && second.defense == 5 && second.money_delta == 0,
		"the same enemy intent keeps its displayed values but cannot apply side effects twice");

	prepared = false;
	const auto guard = ResolveFrame(prepared, { -17, 0, 15, 15 });
	Expect(guard.attack == 80 && guard.cancels_player_damage,
		"damage-cancelling intent is scoped to its prepared turn");
	prepared = false;
	const auto hand_scaled_before = ResolveFrame(prepared, { -10, 0, 15, 15 });
	const auto hand_scaled_after = ResolveFrame(prepared, { -10, 0, 15, 12 });
	Expect(hand_scaled_before.attack == 3 && hand_scaled_after.attack == 9,
		"hand-scaled intent recomputes its displayed attack as cards are played");
}

void TestDebugScenarioRules() {
	const auto& scenario = DebugScenarioRules::Midgame();
	constexpr std::array<std::string_view, 18> expected_deck{
		"b5\n7$", "*$\n2+", "$+\n*3", "4/\n$6", "f\n3\n3", "q\n+",
		"*\n5", "o\n-", "i\ng\n-", "+\n2", "4\na", "e\nc",
		"+\nm", "6\n-", "/\n4", "3\n*", "i\n2", "h\n+",
	};
	struct DebugPlacement {
		int32_t rotations;
		int32_t anchor_x;
		int32_t anchor_y;
	};
	constexpr std::array<DebugPlacement, 18> placements{
		DebugPlacement{ 0, 0, 0 }, { 1, 1, 0 }, { 2, 3, 0 }, { 3, 4, 0 },
		{ 0, 6, 0 }, { 1, 0, 2 }, { 3, 2, 2 }, { 1, 4, 2 },
		{ 1, 0, 3 }, { 3, 3, 3 }, { 1, 5, 3 }, { 3, 0, 4 },
		{ 0, 2, 4 }, { 0, 3, 4 }, { 0, 4, 4 }, { 0, 5, 4 },
		{ 0, 6, 4 }, { 1, 0, 5 },
	};
	Expect((scenario.layer == 22) && (scenario.enemy_type == 0),
		"midgame debug starts in a late non-boss battle");
	Expect((scenario.hp == 100) && (scenario.max_hp == 100) && (scenario.money == 300),
		"midgame debug resources are deterministic");
	Expect((scenario.seed == 0x4D313053ULL) && (scenario.deck.size() == 18),
		"midgame debug uses a fixed seed and exactly eighteen cards");
	Expect((scenario.hand_limit_override == 18)
		&& (scenario.enemy_texture_path == "../../image/boss_1.png")
		&& scenario.preserve_deck_order,
		"midgame debug explicitly overrides the hand limit and enemy visual");
	bool has_uppercase_symbol = false;
	bool all_cards_are_valid = true;
	bool all_cards_use_minimum_width = true;
	bool has_single_cell_card = false;
	int32_t occupied_cell_count = 0;
	bool deck_matches_expected_order = (scenario.deck.size() == expected_deck.size());
	for (std::size_t card_index = 0;
		card_index < std::min(scenario.deck.size(), expected_deck.size()); ++card_index) {
		deck_matches_expected_order = deck_matches_expected_order
			&& (scenario.deck[card_index] == expected_deck[card_index]);
	}
	for (const auto definition : scenario.deck) {
		all_cards_are_valid = all_cards_are_valid
			&& CardSymbolRules::IsValidCardDefinition(definition);
		const std::size_t first_newline = definition.find('\n');
		const int32_t card_width = static_cast<int32_t>(
			(first_newline == std::string_view::npos) ? definition.size() : first_newline);
		const int32_t card_height = 1 + static_cast<int32_t>(
			std::count(definition.begin(), definition.end(), '\n'));
		all_cards_use_minimum_width = all_cards_use_minimum_width
			&& (card_width <= card_height);
		int32_t card_cell_count = 0;
		for (const char symbol : definition) {
			if (('A' <= symbol) && (symbol <= 'H')) has_uppercase_symbol = true;
			if ((symbol != '\n') && (symbol != '$')) ++card_cell_count;
		}
		has_single_cell_card = has_single_cell_card || (card_cell_count == 1);
		occupied_cell_count += card_cell_count;
	}
	Expect(deck_matches_expected_order,
		"midgame debug exposes the requested cards in the requested order");
	Expect(!has_uppercase_symbol && !has_single_cell_card,
		"midgame debug avoids uppercase and single-cell cards");
	Expect(all_cards_are_valid && (occupied_cell_count == 42),
		"midgame debug uses only known symbols and exactly fills forty-two cells");
	Expect(all_cards_use_minimum_width,
		"midgame debug starts every card in its minimum-width orientation");
	int32_t cards_with_holes = 0;
	for (const auto definition : scenario.deck) {
		cards_with_holes += (definition.find('$') != std::string_view::npos) ? 1 : 0;
	}
	Expect(cards_with_holes == 4,
		"midgame debug includes four concave L or key-shaped cards");

	const auto preserved_order = GameStateRules::CreateInitialDrawOrder(
		static_cast<int32_t>(scenario.deck.size()), scenario.preserve_deck_order);
	Expect(!GameStateRules::ShouldShuffleInitialDrawOrder(scenario.preserve_deck_order)
		&& GameStateRules::ShouldShuffleInitialDrawOrder(false),
		"only the explicit debug override disables the normal deck shuffle");
	GameStateRules::BattleDeckState deck;
	deck.Initialize(static_cast<int32_t>(scenario.deck.size()), preserved_order);
	bool hand_matches_scenario_order = true;
	for (int32_t expected_card_id = 0;
		expected_card_id < scenario.hand_limit_override; ++expected_card_id) {
		const int32_t card_id = deck.Cards(GameStateRules::CardZone::DrawPile).back();
		hand_matches_scenario_order = hand_matches_scenario_order
			&& (card_id == expected_card_id);
		Expect(deck.Move(card_id, GameStateRules::CardZone::DrawPile,
			GameStateRules::CardZone::Hand), "midgame debug can draw every visible test card");
	}
	Expect(hand_matches_scenario_order && deck.Validate()
		&& (deck.Cards(GameStateRules::CardZone::Hand).size() == 18),
		"midgame debug starts with eighteen uniquely owned hand cards");

	constexpr std::array<std::string_view, 6> rows{
		"b5+3*4f", "7*2+6/3", "q+5*o-3",
		"ig-2+4a", "ce+6/3i", "h+m-4*2",
	};
	BoardCalculationRules::Board completed_board{ 7, 6 };
	bool legal_completed_board = true;
	for (std::size_t card_index = 0; card_index < scenario.deck.size(); ++card_index) {
		std::vector<std::string> card_rows(1);
		for (const char symbol : scenario.deck[card_index]) {
			if (symbol == '\n') card_rows.emplace_back();
			else card_rows.back().push_back(symbol);
		}
		for (int32_t rotation = 0; rotation < placements[card_index].rotations; ++rotation) {
			const int32_t old_height = static_cast<int32_t>(card_rows.size());
			const int32_t old_width = static_cast<int32_t>(card_rows.front().size());
			std::vector<std::string> rotated(static_cast<std::size_t>(old_width),
				std::string(static_cast<std::size_t>(old_height), '$'));
			for (int32_t y = 0; y < old_height; ++y) {
				for (int32_t x = 0; x < old_width; ++x) {
					rotated[static_cast<std::size_t>(old_width - 1 - x)]
						[static_cast<std::size_t>(y)] = card_rows[static_cast<std::size_t>(y)]
							[static_cast<std::size_t>(x)];
				}
			}
			card_rows = std::move(rotated);
		}
		for (int32_t y = 0; y < static_cast<int32_t>(card_rows.size()); ++y) {
			for (int32_t x = 0; x < static_cast<int32_t>(card_rows[y].size()); ++x) {
				const char symbol = card_rows[y][x];
				if (symbol == '$') continue;
				const int32_t board_x = placements[card_index].anchor_x + x;
				const int32_t board_y = placements[card_index].anchor_y + y;
				legal_completed_board = legal_completed_board
					&& (0 <= board_x) && (board_x < 7) && (0 <= board_y) && (board_y < 6)
					&& !completed_board.At(board_x, board_y).occupied;
				if (legal_completed_board) completed_board.Set(board_x, board_y, symbol);
			}
		}
	}
	for (int32_t y = 0; y < static_cast<int32_t>(rows.size()); ++y) {
		for (int32_t x = 0; x < static_cast<int32_t>(rows[y].size()); ++x) {
			legal_completed_board = legal_completed_board
				&& (completed_board.At(x, y).symbol == rows[y][x]);
		}
	}
	Expect(legal_completed_board,
		"the documented rotations and anchors legally tile the entire debug board");
	const auto result = BoardCalculationRules::Evaluate(completed_board);
	Expect(result.row_values == std::vector<int32_t>{ 17, 16, 8, 14, 6, 5 },
		"the completed debug board produces the documented row values including Ave");
	Expect(result.row_multiplier_effects[0] == 1.5 && result.row_modes[0] == 0,
		"the first debug row applies b and switches to defense");
	Expect(result.row_multiplier_effects[3] == 1.0 && result.row_modes[3] == 1,
		"the fourth debug row applies a and switches to attack");
	Expect(result.row_multiplier_effects[4] == 2.0 && result.row_modes[4] == 1,
		"the fifth debug row applies c and switches to attack");
	bool expression_classification_is_expected = true;
	for (int32_t y = 0; y < static_cast<int32_t>(rows.size()); ++y) {
		for (int32_t x = 0; x < static_cast<int32_t>(rows[y].size()); ++x) {
			const auto kind = CardSymbolRules::Decode(rows[y][x]).kind;
			const auto usage = result.ExpressionUsageAt(x, y);
			const bool is_expression = (kind == CardSymbolRules::Kind::Number)
				|| (kind == CardSymbolRules::Kind::Operator)
				|| (kind == CardSymbolRules::Kind::Aggregate);
			expression_classification_is_expected = expression_classification_is_expected
				&& (usage == (is_expression
					? BoardCalculationRules::ExpressionCellUsage::Used
					: BoardCalculationRules::ExpressionCellUsage::NonExpression));
		}
	}
	Expect(expression_classification_is_expected,
		"the completed debug board uses every expression symbol without fading effects or row modes");
}

void TestBattleLayoutRules() {
	using namespace BattleLayoutRules;
	Expect(std::abs(PlayerDisplayScale - 0.85) < 0.0001,
		"all battle player draws use the shared 0.85 scale");
	Expect((EnemyHitTarget().x - EnemyPosition().x == EnemyHitOffsetX)
		&& (EnemyHitTarget().y - EnemyPosition().y == EnemyHitOffsetY),
		"enemy attack impact follows the shared enemy position");
	const auto enemy_effect_bounds = EnemyDamageEffectBounds();
	Expect((enemy_effect_bounds.x - EnemyPosition().x == EnemyDamageEffectMinOffsetX)
		&& (enemy_effect_bounds.y - EnemyPosition().y == EnemyDamageEffectMinOffsetY),
		"enemy damage effects follow the shared enemy position");
	const double normal_enemy_scale = EnemyDisplayScale(400, 1.0);
	const double boss_enemy_scale = EnemyDisplayScale(700, 1.0);
	Expect(std::abs(normal_enemy_scale * 400.0 - EnemyDisplayHeight) < 0.0001
		&& std::abs(boss_enemy_scale * 700.0 - EnemyDisplayHeight) < 0.0001,
		"normal and boss textures share one normalized display height");
	Expect(std::abs((800.0 * EnemyBaseScale(400)) / EnemyDisplayHeight - 2.0) < 0.0001,
		"enemy normalization preserves the source aspect ratio");
	Expect(std::abs(EnemyDisplayScale(400, EnemyHitScaleMultiplier) - 0.7) < 0.0001
		&& std::abs(EnemyDisplayScale(700, EnemyHitScaleMultiplier) * 700.0 - 280.0) < 0.0001,
		"the hit animation is relative to each texture's normalized base scale");
	Expect(EnemyBaseScale(0) == 1.0,
		"an unavailable enemy texture has a safe neutral scale");
	std::array<ScreenRect, 18> hand_bounds{};
	for (int32_t slot = 0; slot < 18; ++slot) {
		hand_bounds[static_cast<std::size_t>(slot)] = HandCardBounds(slot);
		Expect(SceneBounds().Contains(hand_bounds[static_cast<std::size_t>(slot)]),
			"every debug hand card remains inside the logical scene");
		if (slot != 0) {
			Expect(HandPosition(slot).x != HandPosition(slot - 1).x,
				"adjacent debug hand slots have distinct horizontal positions");
		}
	}
	Expect(!hand_bounds.back().Intersects(EqualButtonBounds()),
		"the eighteenth hand card does not overlap the attack button");
	Expect(!hand_bounds.back().Intersects(DiscardPileBounds()),
		"the eighteenth hand card does not overlap the discard pile");
	Expect(SceneBounds().Contains(EqualButtonBounds()),
		"the attack button stays fully inside the logical scene");
	Expect((PlayerPosition().y == 230) && (PlayerHpPosition().y == 700)
		&& (EnemyPosition().y == 500) && (EnemyHpPosition().y == 700)
		&& (BoardOffset().y == 190) && (HandPosition(0).y == 900)
		&& (EqualButtonBounds().y == 750),
		"battle layout preserves the selected vertical positions");
	for (int32_t y = 0; y < BoardHeight; ++y) {
		for (int32_t x = 0; x < BoardWidth; ++x) {
			const BattleLayoutRules::BoardCell cell{ x, y };
			Expect(BoardCellAt(BoardCellCenter(cell)) == cell,
				"board cell centers round-trip through the shared transform");
		}
	}
	Expect(BoardCellAt({ BoardOffset().x - 1, BoardOffset().y })
		== BattleLayoutRules::BoardCell{ -1, -1 },
		"screen-to-board conversion does not clamp outside coordinates");
}

void TestShopRules() {
	using namespace ShopRules;
	Expect(IsOneTimeRelic(3) && IsOneTimeRelic(10) && !IsOneTimeRelic(5),
		"one-time relic classification matches the shop pool");
	Expect(!CanOfferRelic(3, 1, false), "owned one-time relic is not offered");
	Expect(!CanOfferRelic(3, 0, true), "one-time relic is not offered twice in one shop");
	Expect(CanOfferRelic(5, 2, true), "stackable relic may be offered repeatedly");
}

} // namespace

int main() {
	TestHandDrops();
	TestBoardDrops();
	TestRotationAndFastRelease();
	TestBoardCardSwapRules();
	TestDamageHitRules();
	TestInputOwnership();
	TestInteractionDrawLayers();
	TestStableIdentityAndReservations();
	TestConcurrentReturnMotions();
	TestForcedMotionCompletion();
	TestRepeatedFailedBoardSwapReturn();
	TestBoardProgress();
	TestBattleDeckState();
	TestTurnDrawRefreshRules();
	TestAudioSettingsRules();
	TestCardSymbolRules();
	TestBoardCalculationRules();
	TestDelayedEffectLifecycle();
	TestEnemyIntentRules();
	TestDebugScenarioRules();
	TestBattleLayoutRules();
	TestShopRules();
	if (failures != 0) return EXIT_FAILURE;
	std::cout << "All battle card interaction tests passed\n";
	return EXIT_SUCCESS;
}
