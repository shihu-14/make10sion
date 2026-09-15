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
	same.near_start = true;
	Expect(ResolveDrop(same, board).result == DropResult::RestoreToBoard,
		"unrotated board card restores near its drag start");
	same.near_start = false;
	Expect(ResolveDrop(same, board).result == DropResult::Place,
		"rotated board card commits its new orientation at the original anchor");
	same.candidate_anchor = { 3, 2 };
	same.near_start = true;
	Expect(ResolveDrop(same, board).result == DropResult::RestoreToBoard,
		"board card restores inside the cancellation distance");

	auto corner_board = MakeBoard();
	At(corner_board, { 2, 2 }).occupant = 4;
	At(corner_board, { 3, 2 }).occupant = 4;
	At(corner_board, { 2, 3 }).occupant = 4;
	auto rotated_corner = Request(DragOrigin::Board, { 2, 2 },
		{ { 0, 0 }, { 1, 0 }, { 1, 1 } });
	rotated_corner.near_start = false;
	Expect(ResolveDrop(rotated_corner, corner_board).result == DropResult::Place,
		"rotated corner card commits its changed footprint at the original anchor");

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
		"an unrotated invalid placement may use one legal automatic quarter-turn");
	Expect(!ShouldUseAutoRotatedPlacement(
		DropResult::ReturnToHand, DropResult::Place, true),
		"a drop never overrides the rotation selected during the drag");
	Expect(!ShouldUseAutoRotatedPlacement(DropResult::BoardSwap, DropResult::Place),
		"a board swap candidate is never silently replaced by automatic rotation");
}

void TestDamageHitRules() {
	using namespace BattleDamageRules;
	const auto no_defense = ResolveDefenseExchange(12, 0, 12);
	Expect(!no_defense.has_contact && (no_defense.attack_after == 12)
		&& (no_defense.defense_after == 0),
		"zero defense skips the defense-contact stage");
	const auto absorbed = ResolveDefenseExchange(8, 10, 0);
	Expect(absorbed.has_contact && (absorbed.attack_after == 0)
		&& (absorbed.defense_after == 2),
		"defense contact can absorb the complete attack");
	const auto penetrated = ResolveDefenseExchange(14, 9, 5);
	Expect(penetrated.has_contact && (penetrated.attack_after == 5)
		&& (penetrated.defense_after == 0),
		"defense contact leaves the calculated remaining attack");
	const auto zero_attack = ResolveDefenseExchange(0, 10, 0);
	Expect(!zero_attack.has_contact && (zero_attack.attack_after == 0)
		&& (zero_attack.defense_after == 10),
		"zero attack skips defense contact without reducing defense");
	Expect(HitCountForDamage(6, 100) == 1,
		"damage up to one eighth of half maximum HP uses one hit");
	Expect(HitCountForDamage(7, 100) == 2,
		"damage above one eighth of half maximum HP uses two hits");
	Expect(HitCountForDamage(25, 100) == 4,
		"damage equal to one quarter maximum HP uses half the configured hits");
	Expect(HitCountForDamage(44, 100) == MaximumDamageHitCount,
		"damage in the final half-HP step uses the configured maximum hit count");
	Expect(HitCountForDamage(50, 100) == MaximumDamageHitCount
		&& HitCountForDamage(100, 100) == MaximumDamageHitCount,
		"damage at or above half maximum HP stays at the configured maximum hit count");
	Expect(HitCountForDamage(9, 20) == MaximumDamageHitCount,
		"an early enemy can reach the configured maximum hit count");
	const auto maximum_hits = SplitDamage(50, 100);
	Expect(maximum_hits.size() == static_cast<std::size_t>(MaximumDamageHitCount),
		"large damage uses the configured maximum hit count");
	Expect(std::abs(ResolveDefenseExchangeDuration(25, 100) - 0.25) < 0.000001
		&& std::abs(ResolveDefenseExchangeDuration(50, 100) - 0.5) < 0.000001
		&& std::abs(ResolveDefenseExchangeDuration(100, 100) - 1.0) < 0.000001,
		"defense exchange duration follows the proportion of starting defense removed");
	Expect((ResolveDefenseExchangeDuration(0, 100) == 0.0)
		&& std::abs(ResolveDefenseExchangeDuration(100, 10) - 1.0) < 0.000001,
		"removing all starting defense always uses the maximum exchange duration");

	const auto hits = SplitDamage(17, 100);
	Expect((hits.size() == 3) && (hits[0] == 6) && (hits[1] == 6) && (hits[2] == 5),
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
	Expect(ShouldRefreshPlayerCombatValues(true, false, false, false, false),
		"entering idle refreshes the player combat values");
	Expect(ShouldRefreshPlayerCombatValues(true, true, true, false, false),
		"finishing a board drag refreshes the player combat values");
	Expect(ShouldRefreshPlayerCombatValues(true, true, false, false, true),
		"a hand drag completed within one frame refreshes the player combat values");
	Expect(!ShouldRefreshPlayerCombatValues(true, true, false, false, false),
		"an unchanged idle board does not recalculate combat values every frame");
	Expect(!ShouldRefreshPlayerCombatValues(false, true, true, false, false),
		"combat animation never replaces its mutable combat values with a preview");
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
	Expect(!AdvanceVisualMotion(motion_a, 1.0 / 60.0,
		ResolveReturnMotionEasing(card_a)), "return continues across multiple frames");
	const ScreenPoint a_after_first_frame = motion_a.current;
	Expect((a_after_first_frame != motion_a.start) && (a_after_first_frame != motion_a.end),
		"returning card has a visual-only intermediate position");

	card_b = CardLifecycle::DraggingFromHand;
	Expect(IsLogicallyInHand(card_a) && IsLogicallyInHand(card_b),
		"returning and dragging cards are both logically in hand");
	StartVisualMotion(motion_b, { 900, 450 }, { 425, 900 });
	card_b = CardLifecycle::ReturningToHand;

	for (int frame = 0; frame < 16; frame++) {
		if (AdvanceVisualMotion(motion_a, 1.0 / 60.0,
			ResolveReturnMotionEasing(card_a))) SettleReturnLifecycle(card_a);
		if (AdvanceVisualMotion(motion_b, 1.0 / 60.0,
			ResolveReturnMotionEasing(card_b))) SettleReturnLifecycle(card_b);
	}
	Expect((card_a == CardLifecycle::InHand) && (motion_a.current == ScreenPoint{ 350, 900 }),
		"first card snaps exactly to its reserved hand position");
	Expect((card_b == CardLifecycle::InHand) && (motion_b.current == ScreenPoint{ 425, 900 }),
		"second card returns independently to a different hand position");
}

void TestHandReturnMotionTiming() {
	Expect(ResolveReturnMotionEasing(CardLifecycle::ReturningToHand)
		== VisualMotionEasing::CubicEaseOut
		&& ResolveReturnMotionEasing(CardLifecycle::ReturningToBoard)
		== VisualMotionEasing::CubicEaseOut,
		"hand and board returns both select cubic ease-out timing");

	VisualMotion hand_motion;
	VisualMotion board_motion;
	StartVisualMotion(hand_motion, { 0, 0 }, { 100, 100 }, 1.0);
	StartVisualMotion(board_motion, { 0, 0 }, { 100, 100 }, 1.0);
	Expect(!AdvanceVisualMotion(hand_motion, 0.5, VisualMotionEasing::CubicEaseOut)
		&& !AdvanceVisualMotion(board_motion, 0.5, VisualMotionEasing::CubicEaseOut)
		&& (hand_motion.current == ScreenPoint{ 87, 87 })
		&& (board_motion.current == ScreenPoint{ 87, 87 })
		&& (hand_motion.elapsed_seconds == board_motion.elapsed_seconds)
		&& (hand_motion.duration_seconds == board_motion.duration_seconds)
		&& (hand_motion.end == board_motion.end),
		"both return paths start fast while preserving duration and endpoint");

	Expect(AdvanceVisualMotion(hand_motion, 0.5, VisualMotionEasing::CubicEaseOut)
		&& AdvanceVisualMotion(board_motion, 0.5, VisualMotionEasing::CubicEaseOut)
		&& (hand_motion.current == hand_motion.end)
		&& (board_motion.current == board_motion.end),
		"cubic returns still complete at the original duration and endpoint");
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

void TestHandDealAnimationRules() {
	Expect(ResolveHandDealStage(true, false, false, false) == HandDealStage::RotatePile
		&& ResolveHandDealStage(true, false, true, false) == HandDealStage::DealCards,
		"hand dealing rotates the pile once before launching cards");
	Expect(ResolveHandDealStage(true, true, false, false) == HandDealStage::ReturnPile
		&& ResolveHandDealStage(true, true, false, true) == HandDealStage::Complete,
		"completed hand dealing returns the pile without restarting card launches");

	Expect(!ShouldDrawHandCardDuringDeal(0, 0, true)
		&& ShouldDrawHandCardDuringDeal(0, 1, true)
		&& !ShouldDrawHandCardDuringDeal(1, 1, true)
		&& ShouldDrawHandCardDuringDeal(1, 0, false),
		"deal rendering reveals only cards whose launch has started");
	Expect(ShouldPlayHandDealSound(0) && ShouldPlayHandDealSound(7)
		&& !ShouldPlayHandDealSound(8) && !ShouldPlayHandDealSound(14),
		"hand deal sound plays for only the first eight launched cards");

	const auto second_before_start = ResolveHandDealProgress(1, 0.099);
	const auto second_at_start = ResolveHandDealProgress(1, 0.10);
	Expect(!second_before_start.started && (second_before_start.linear_progress == 0.0)
		&& second_at_start.started && (second_at_start.linear_progress == 0.0),
		"hand cards preserve the configured one hundred millisecond stagger interval");

	const auto first_in_flight = ResolveHandDealProgress(0, 0.15);
	const auto second_in_flight = ResolveHandDealProgress(1, 0.15);
	Expect(first_in_flight.started && second_in_flight.started
		&& !first_in_flight.complete && !second_in_flight.complete,
		"successive hand cards overlap in flight instead of waiting for each other");

	const auto quarter = ResolveHandDealProgress(0, 0.075);
	const auto midpoint = ResolveHandDealProgress(0, 0.15);
	const auto three_quarters = ResolveHandDealProgress(0, 0.225);
	Expect(std::abs(quarter.eased_progress - 0.578125) < 0.000001
		&& std::abs(midpoint.eased_progress - 0.875) < 0.000001
		&& std::abs(three_quarters.eased_progress - 0.984375) < 0.000001,
		"hand card travel starts fast and decelerates with cubic ease-out");

	const auto last_complete = ResolveHandDealProgress(14, 1.7);
	Expect(last_complete.complete && (last_complete.eased_progress == 1.0)
		&& (std::abs(HandDealTotalDuration(1) - 0.30) < 0.000001)
		&& (std::abs(HandDealTotalDuration(15) - 1.7) < 0.000001)
		&& (HandDealTotalDuration(0) == 0.0),
		"deal duration preserves the current user-adjusted timing parameters");
}

void TestDiscardCollectionTiming() {
	Expect(std::abs(DiscardCardStartIntervalSeconds(0, 15) - 0.30) < 0.000001
		&& std::abs(DiscardCardStartIntervalSeconds(13, 15) - 0.04) < 0.000001,
		"discard launch intervals run from three hundred to forty milliseconds");
	double previous_interval = DiscardCardStartIntervalSeconds(0, 15);
	for (int32_t interval_index = 1; interval_index < 14; ++interval_index) {
		const double interval = DiscardCardStartIntervalSeconds(interval_index, 15);
		Expect(interval < previous_interval,
			"each later discard card starts sooner than the previous one");
		previous_interval = interval;
	}

	const auto first_halfway = ResolveDiscardCardProgress(0, 15, 0.075);
	const auto second_before_start = ResolveDiscardCardProgress(1, 15, 0.299);
	const auto second_at_start = ResolveDiscardCardProgress(1, 15, 0.30);
	const auto second_halfway = ResolveDiscardCardProgress(1, 15, 0.375);
	Expect(first_halfway.started && !first_halfway.complete
		&& std::abs(first_halfway.linear_progress - 0.5) < 0.000001
		&& !second_before_start.started
		&& second_at_start.started && (second_at_start.linear_progress == 0.0)
		&& second_halfway.started && !second_halfway.complete
		&& std::abs(second_halfway.linear_progress - 0.5) < 0.000001,
		"discard cards keep the same linear travel while only launch timing changes");
	Expect(std::abs(DiscardCollectionTotalDuration(15) - 2.53) < 0.000001
		&& std::abs(DiscardCollectionTotalDuration(1) - 0.15) < 0.000001
		&& (DiscardCollectionTotalDuration(0) == 0.0),
		"discard collection duration follows card count and accelerating intervals");
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

		for (int frame = 0; frame < 16; frame++) {
			if (frame == 1) {
				Expect(CanStartCardDrag(other_card),
					"another board card remains draggable during a return");
				other_card = CardLifecycle::DraggingFromBoard;
			}
			Expect(CanProcessBoardInput(true, false, PointerInputOwner::Card),
				"returning visual does not roll back another active drag");
			if (AdvanceVisualMotion(return_motion, 1.0 / 60.0,
				ResolveReturnMotionEasing(returning_card))) {
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
	Expect(progress.UnlockedCount() == 9, "board progress starts with nine cells");
	Expect(CalculateHandLimit(progress) == 7, "nine cells allow seven hand cards");
	Expect(progress.IsUnlockable({ 2, 0 }), "cell above the initial board is unlockable");
	Expect(!progress.IsUnlockable({ 0, 0 }), "detached cell is not unlockable");
	Expect(progress.Unlock({ 2, 0 }), "first event cell unlock succeeds");
	Expect(!progress.Unlock({ 2, 0 }), "same event cell cannot unlock twice");
	Expect(progress.Unlock({ 3, 0 }), "second event cell unlock succeeds");
	Expect((progress.UnlockedCount() == 11) && (CalculateHandLimit(progress) == 8),
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
	Expect(CalculateHandLimit(fully_unlocked) == 15, "hand limit is capped at fifteen");
	Expect(ResolveBattleHandLimit(fully_unlocked, 0) == 15,
		"normal battles use the fifteen-card maximum");
	Expect(ResolveBattleHandLimit(fully_unlocked, 18) == 15,
		"an explicit override cannot exceed the fifteen-card maximum");
	BoardProgress debug_progress;
	debug_progress.UnlockAll();
	Expect((debug_progress.UnlockedCount() == BoardProgress::CellCount)
		&& (CalculateHandLimit(debug_progress) == 15),
		"debug board progress unlocks all forty-two cells while keeping the shared hand maximum");
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
	Expect(ResolveExpressionCellAlpha(Usage::Ignored) == 0.15
		&& ResolveExpressionCellAlpha(Usage::Used) == 1.0
		&& ResolveExpressionCellAlpha(Usage::NonExpression) == 1.0,
		"only ignored expression cells use the disabled cell alpha");
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
	Expect((scenario.hand_limit_override == 15)
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
		&& (deck.Cards(GameStateRules::CardZone::Hand).size() == 15)
		&& (deck.Cards(GameStateRules::CardZone::DrawPile).size() == 3),
		"midgame debug starts with fifteen hand cards and keeps three in the draw pile");

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
	Expect((ResolveBodyAttackAlpha(0.0) == 1.0)
		&& (ResolveBodyAttackAlpha(0.25) == 0.5)
		&& (ResolveBodyAttackAlpha(0.5) == 0.0)
		&& (ResolveBodyAttackAlpha(1.0) == 0.0),
		"body attack fades completely over half a second");
	Expect((DamageEffectTriggerTime(0) == 0.0)
		&& (std::abs(DamageEffectTriggerTime(BattleDamageRules::MaximumDamageHitCount - 1)
			- DamageEffectInterval * (BattleDamageRules::MaximumDamageHitCount - 1)) < 0.000001),
		"damage effect timing follows the configured count and interval");
	Expect(!ShouldTriggerDamageEffect(DamageEffectInterval - 0.001, 1)
		&& ShouldTriggerDamageEffect(DamageEffectInterval, 1),
		"damage effects trigger exactly at the configured interval");
	Expect((EnemyAttackArcTarget() == BattleLayoutRules::ScreenPoint{ 1400, 250 })
		&& (PlayerAttackArcTarget() == BattleLayoutRules::ScreenPoint{ 250, 250 }),
		"attack arcs retain their original targets before defense contact");
	const auto player_arc_start = ResolveAttackArcMotion(
		PlayerAttackStart(), EnemyAttackArcTarget(), 0.0);
	const auto player_arc_midpoint = ResolveAttackArcMotion(
		PlayerAttackStart(), EnemyAttackArcTarget(), AttackArcTravelDuration / 2.0);
	const auto player_arc_end = ResolveAttackArcMotion(
		PlayerAttackStart(), EnemyAttackArcTarget(), AttackArcTravelDuration);
	const double player_linear_midpoint_y =
		(PlayerAttackStart().y + EnemyAttackArcTarget().y) / 2.0;
	Expect((player_arc_start.x == PlayerAttackStart().x)
		&& (player_arc_start.y == PlayerAttackStart().y)
		&& (player_arc_end.x == EnemyAttackArcTarget().x)
		&& (player_arc_end.y == EnemyAttackArcTarget().y)
		&& (player_arc_midpoint.y < player_linear_midpoint_y),
		"player attack follows an upward arc to the enemy defense icon");
	Expect((player_arc_start.scale == 1.0)
		&& (player_arc_midpoint.scale > player_arc_start.scale)
		&& (player_arc_midpoint.scale < player_arc_end.scale)
		&& (player_arc_end.scale == 1.3),
		"attack icon and value grow gradually along the arc");
	const auto enemy_arc_midpoint = ResolveAttackArcMotion(
		EnemyAttackStart(), PlayerAttackArcTarget(), AttackArcTravelDuration / 2.0);
	const double enemy_linear_midpoint_y =
		(EnemyAttackStart().y + PlayerAttackArcTarget().y) / 2.0;
	Expect(enemy_arc_midpoint.y < enemy_linear_midpoint_y,
		"enemy attack follows an upward arc toward the player defense icon");
	Expect((ResolveCombatValueChange(100, 0, 0.5, 1.0) == 50)
		&& (ResolveCombatValueChange(10, 0, 0.5, 1.0) == 5)
		&& (ResolveCombatValueChange(100, 0, 1.0, 1.0) == 0)
		&& (ResolveCombatValueChange(10, 0, 1.0, 1.0) == 0),
		"combat values share the selected animation duration");
	Expect((ResolveCombatValueChange(100, 0, 0.25, 0.5) == 50)
		&& (ResolveCombatValueChange(100, 0, 0.5, 0.5) == 0),
		"combat values use the selected damage-based exchange duration");
	const auto idle_player_values = ResolvePlayerCombatValueVisibility(
		true, false, false, false);
	Expect(idle_player_values.attack && idle_player_values.defense,
		"idle battle shows the board-derived player attack and defense");
	const auto enemy_effect_values = ResolvePlayerCombatValueVisibility(
		false, true, false, false);
	const auto player_effect_values = ResolvePlayerCombatValueVisibility(
		false, false, true, false);
	const auto discard_values = ResolvePlayerCombatValueVisibility(
		false, false, false, true);
	const auto hidden_values = ResolvePlayerCombatValueVisibility(
		false, false, false, false);
	Expect(!enemy_effect_values.attack && enemy_effect_values.defense
		&& !player_effect_values.attack && player_effect_values.defense
		&& !discard_values.attack && discard_values.defense
		&& !hidden_values.attack && !hidden_values.defense,
		"combat and discard phases preserve the existing player value visibility");
	Expect((EnemyHitTarget().x - EnemyPosition().x == EnemyHitOffsetX)
		&& (EnemyHitTarget().y - EnemyPosition().y == EnemyHitOffsetY),
		"enemy attack impact follows the shared enemy position");
	const auto enemy_effect_bounds = EnemyDamageEffectBounds();
	Expect((enemy_effect_bounds.x
			== EnemyPosition().x - EnemyDamageEffectRandomOffset.x)
		&& (enemy_effect_bounds.y
			== EnemyPosition().y - EnemyDamageEffectRandomOffset.y)
		&& (enemy_effect_bounds.width == EnemyDamageEffectRandomOffset.x * 2)
		&& (enemy_effect_bounds.height == EnemyDamageEffectRandomOffset.y * 2),
		"enemy damage effects use one random offset around the enemy center");
	const auto player_effect_bounds = PlayerDamageEffectBounds();
	Expect((player_effect_bounds.x
			== PlayerDamageEffectCenter.x - PlayerDamageEffectRandomOffset.x)
		&& (player_effect_bounds.y
			== PlayerDamageEffectCenter.y - PlayerDamageEffectRandomOffset.y)
		&& (player_effect_bounds.width == PlayerDamageEffectRandomOffset.x * 2)
		&& (player_effect_bounds.height == PlayerDamageEffectRandomOffset.y * 2),
		"player damage effects use a center and random offset instead of hardcoded bounds");
	const double normal_enemy_scale = EnemyDisplayScale(400, 1.0);
	const double boss_enemy_scale = EnemyDisplayScale(700, 1.0);
	Expect(std::abs(normal_enemy_scale * 400.0 - boss_enemy_scale * 700.0) < 0.0001,
		"normal and boss textures share one normalized display height");
	const double normal_hit_scale = EnemyDisplayScale(400, EnemyHitScaleMultiplier);
	const double boss_hit_scale = EnemyDisplayScale(700, EnemyHitScaleMultiplier);
	Expect(std::abs((normal_hit_scale / normal_enemy_scale)
		- (boss_hit_scale / boss_enemy_scale)) < 0.0001,
		"hit animation applies the same normalized multiplier to each texture");
	Expect(EnemyBaseScale(0) == 1.0,
		"an unavailable enemy texture has a safe neutral scale");
	const auto unchanged_changes = ResolvePlayerCombatStatPulseChanges(12, 7, 12, 7);
	const auto attack_only_change = ResolvePlayerCombatStatPulseChanges(12, 7, 18, 7);
	const auto defense_only_change = ResolvePlayerCombatStatPulseChanges(12, 7, 12, 3);
	const auto mixed_changes = ResolvePlayerCombatStatPulseChanges(12, 7, 18, 3);
	Expect(!unchanged_changes.attack && !unchanged_changes.defense,
		"unchanged player combat values do not animate");
	Expect(attack_only_change.attack && !attack_only_change.defense
		&& !defense_only_change.attack && defense_only_change.defense
		&& mixed_changes.attack && mixed_changes.defense,
		"player attack and defense detect value changes independently");
	const auto increase_peak = ResolveCombatStatPulse(
		CombatStatPulseDuration / 2.0, mixed_changes.attack);
	const auto decrease_peak = ResolveCombatStatPulse(
		CombatStatPulseDuration / 2.0, mixed_changes.defense);
	const auto unchanged_peak = ResolveCombatStatPulse(
		CombatStatPulseDuration / 2.0, unchanged_changes.attack);
	Expect(increase_peak.scale == 1.1
		&& increase_peak.scale > 1.0 && increase_peak.lift == CombatStatPulseLift
		&& decrease_peak.scale == 1.1
		&& decrease_peak.scale > 1.0 && decrease_peak.lift == CombatStatPulseLift
		&& unchanged_peak.scale == 1.0 && unchanged_peak.lift == 0.0,
		"changed values grow to the shared peak and unchanged values remain still");
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
	TestHandReturnMotionTiming();
	TestForcedMotionCompletion();
	TestHandDealAnimationRules();
	TestDiscardCollectionTiming();
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
