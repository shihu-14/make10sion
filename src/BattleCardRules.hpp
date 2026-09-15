#ifndef BattleCardRules_HPP
#define BattleCardRules_HPP

#include <cstdint>
#include <vector>

namespace BattleCardRules {

using CardId = std::int32_t;
inline constexpr CardId EmptyCardId = -1; // カード未配置を表すIDを定義する．

struct Cell {
	std::int32_t x = -1;
	std::int32_t y = -1;

	friend constexpr bool operator==(const Cell&, const Cell&) = default;
};

struct BoardCell {
	bool usable = false;
	CardId occupant = EmptyCardId;

	friend constexpr bool operator==(const BoardCell&, const BoardCell&) = default;
};

struct BoardSnapshot {
	std::int32_t width = 0;
	std::int32_t height = 0;
	std::vector<BoardCell> cells;

	[[nodiscard]] bool contains(const Cell cell) const noexcept {
		return (0 <= cell.x) && (cell.x < width) && (0 <= cell.y) && (cell.y < height);
	}

	[[nodiscard]] const BoardCell* at(const Cell cell) const noexcept {
		if (!contains(cell)) return nullptr;
		const auto index = static_cast<std::size_t>(cell.y * width + cell.x);
		if (cells.size() <= index) return nullptr;
		return &cells[index];
	}
};

enum class DragOrigin {
	Hand,
	Board,
};

enum class DropResult {
	ReturnToHand,
	RestoreToBoard,
	Place,
	Swap,
	BoardSwap,
};

struct DropRequest {
	CardId card_id = EmptyCardId;
	DragOrigin origin = DragOrigin::Hand;
	Cell candidate_anchor;
	Cell original_anchor;
	std::vector<Cell> footprint;
	bool pointer_on_board = false;
	bool cancelled = false;
	bool near_start = false;
};

struct DropDecision {
	DropResult result = DropResult::ReturnToHand;
	Cell anchor;
	CardId target_card_id = EmptyCardId;
};

[[nodiscard]] inline DropDecision ResolveDrop(const DropRequest& request, const BoardSnapshot& board) {
	// ドロップ位置と重なり方からカードの処理結果を決定する．
	DropDecision decision;
	decision.anchor = request.candidate_anchor;
	if (request.cancelled) {
		decision.result = (request.origin == DragOrigin::Board)
			? DropResult::RestoreToBoard
			: DropResult::ReturnToHand;
		return decision;
	}

	bool invalid = !request.pointer_on_board || request.footprint.empty();
	std::vector<CardId> overlapping_cards;
	for (const Cell local_cell : request.footprint) {
		const Cell board_cell{
			request.candidate_anchor.x + local_cell.x,
			request.candidate_anchor.y + local_cell.y,
		};
		const BoardCell* cell = board.at(board_cell);
		if ((cell == nullptr) || !cell->usable) {
			invalid = true;
			continue;
		}
		if ((cell->occupant == EmptyCardId) || (cell->occupant == request.card_id)) continue;
		bool already_found = false;
		for (const CardId card_id : overlapping_cards) {
			if (card_id == cell->occupant) {
				already_found = true;
				break;
			}
		}
		if (!already_found) overlapping_cards.push_back(cell->occupant);
	}

	if (request.origin == DragOrigin::Board) {
		if (1 < overlapping_cards.size()) {
			decision.result = DropResult::RestoreToBoard;
			return decision;
		}
		if (overlapping_cards.size() == 1) {
			decision.result = DropResult::BoardSwap;
			decision.target_card_id = overlapping_cards.front();
			return decision;
		}
		if (invalid) {
			decision.result = DropResult::ReturnToHand;
			return decision;
		}
		if (request.near_start) {
			decision.result = DropResult::RestoreToBoard;
			return decision;
		}
		decision.result = DropResult::Place;
		return decision;
	}

	if (invalid || (1 < overlapping_cards.size())) return decision;
	if (overlapping_cards.empty()) {
		decision.result = DropResult::Place;
		return decision;
	}
	decision.result = DropResult::Swap;
	decision.target_card_id = overlapping_cards.front();
	return decision;
}

struct BoardSwapRequest {
	CardId first_card_id = EmptyCardId;
	Cell first_destination_anchor;
	std::vector<Cell> first_footprint;
	CardId second_card_id = EmptyCardId;
	Cell second_destination_anchor;
	std::vector<Cell> second_footprint;
};

[[nodiscard]] inline bool CanSwapBoardCards(
	const BoardSwapRequest& request, const BoardSnapshot& board) {
	// 2枚の盤面カードを交換できる配置か判定する．
	if ((request.first_card_id == EmptyCardId) || (request.second_card_id == EmptyCardId)
		|| (request.first_card_id == request.second_card_id)
		|| request.first_footprint.empty() || request.second_footprint.empty()) return false;
	std::vector<Cell> first_cells;
	first_cells.reserve(request.first_footprint.size());
	for (const Cell local_cell : request.first_footprint) {
		const Cell cell{ request.first_destination_anchor.x + local_cell.x,
			request.first_destination_anchor.y + local_cell.y };
		const BoardCell* board_cell = board.at(cell);
		if ((board_cell == nullptr) || !board_cell->usable
			|| ((board_cell->occupant != EmptyCardId)
				&& (board_cell->occupant != request.first_card_id)
				&& (board_cell->occupant != request.second_card_id))) return false;
		first_cells.push_back(cell);
	}
	for (const Cell local_cell : request.second_footprint) {
		const Cell cell{ request.second_destination_anchor.x + local_cell.x,
			request.second_destination_anchor.y + local_cell.y };
		const BoardCell* board_cell = board.at(cell);
		if ((board_cell == nullptr) || !board_cell->usable
			|| ((board_cell->occupant != EmptyCardId)
				&& (board_cell->occupant != request.first_card_id)
				&& (board_cell->occupant != request.second_card_id))) return false;
		for (const Cell first_cell : first_cells) {
			if (first_cell == cell) return false;
		}
	}
	return true;
}

[[nodiscard]] inline bool ShouldUseAutoRotatedPlacement(
	const DropResult current, const DropResult rotated,
	const bool manually_rotated = false) noexcept {
	return !manually_rotated
		&& (current == DropResult::ReturnToHand) && (rotated == DropResult::Place);
}

[[nodiscard]] inline bool ShouldRotateDraggedCard(
	const bool drag_active, const bool rotate_pressed) noexcept {
	return drag_active && rotate_pressed;
}

[[nodiscard]] inline bool CanResetCardRotation(const bool has_board_occupancy) noexcept {
	return !has_board_occupancy;
}

enum class PointerInputOwner {
	None,
	Card,
	Deck,
	Attack,
};

[[nodiscard]] inline PointerInputOwner CapturePointerOwner(
	const bool deck_modal,
	const bool drag_active,
	const bool can_interact,
	const bool deck_hit,
	const bool attack_hit,
	const bool hand_hit,
	const bool board_hit) noexcept {
	// モーダル，ドラッグ，通常操作の優先順位を固定する．
	if (deck_modal) return PointerInputOwner::Deck;
	if (drag_active) return PointerInputOwner::Card;
	if (!can_interact) return PointerInputOwner::None;
	if (deck_hit) return PointerInputOwner::Deck;
	if (attack_hit) return PointerInputOwner::Attack;
	if (hand_hit || board_hit) return PointerInputOwner::Card;
	return PointerInputOwner::None;
}

[[nodiscard]] inline bool CanAcceptBattleInput(
	const bool idle,
	const bool board_locked,
	const bool scene_transition_started) noexcept {
	// シーン状態と遷移状態をまたいだ入力受付条件を一元化する．
	return idle && !board_locked && !scene_transition_started;
}

[[nodiscard]] inline bool ShouldRefreshPlayerCombatValues(
	const bool is_idle, const bool was_idle,
	const bool was_dragging, const bool is_dragging,
	const bool drag_started_this_frame) noexcept {
	return is_idle && (!was_idle
		|| ((was_dragging || drag_started_this_frame) && !is_dragging));
}

inline bool BeginOneShotTransition(bool& started) noexcept {
	// 複数フレーム更新でもシーン遷移を一度だけ開始する．
	if (started) return false;
	started = true;
	return true;
}

struct ReservedHandSlot {
	CardId card_id = EmptyCardId;
	std::int32_t slot = -1;
};

enum class CardLifecycle {
	InDeck,
	InHand,
	DraggingFromHand,
	OnBoard,
	DraggingFromBoard,
	ReturningToHand,
	ReturningToBoard,
	InDiscard,
};

enum class CardDrawLayer {
	Hidden,
	StaticHand,
	StaticBoard,
	ReturningOverlay,
	DraggingOverlay,
};

[[nodiscard]] inline CardDrawLayer GetCardDrawLayer(const CardLifecycle lifecycle) noexcept {
	switch (lifecycle) {
	case CardLifecycle::InHand:
		return CardDrawLayer::StaticHand;
	case CardLifecycle::OnBoard:
		return CardDrawLayer::StaticBoard;
	case CardLifecycle::ReturningToHand:
	case CardLifecycle::ReturningToBoard:
		return CardDrawLayer::ReturningOverlay;
	case CardLifecycle::DraggingFromHand:
	case CardLifecycle::DraggingFromBoard:
		return CardDrawLayer::DraggingOverlay;
	case CardLifecycle::InDeck:
	case CardLifecycle::InDiscard:
		return CardDrawLayer::Hidden;
	}
	return CardDrawLayer::Hidden;
}

[[nodiscard]] inline bool CanProcessBoardInput(
	const bool can_accept_battle_input,
	const bool hand_capture_failed,
	const PointerInputOwner pointer_owner) noexcept {
	// ポインター所有権と盤面ロックを統合して入力可否を決める．
	return can_accept_battle_input && !hand_capture_failed
		&& ((pointer_owner == PointerInputOwner::Card)
			|| (pointer_owner == PointerInputOwner::None));
}

struct ScreenPoint {
	std::int32_t x = 0;
	std::int32_t y = 0;

	friend constexpr bool operator==(const ScreenPoint&, const ScreenPoint&) = default;
};

struct VisualMotion {
	bool active = false;
	ScreenPoint start;
	ScreenPoint end;
	ScreenPoint current;
	double elapsed_seconds = 0.0;
	double duration_seconds = 0.15;
};

enum class VisualMotionEasing {
	Linear,
	CubicEaseOut,
};

[[nodiscard]] inline constexpr double CubicEaseOut(const double progress) noexcept {
	const double inverse = 1.0 - progress;
	return 1.0 - inverse * inverse * inverse;
}

[[nodiscard]] inline constexpr VisualMotionEasing ResolveReturnMotionEasing(
	const CardLifecycle lifecycle) noexcept {
	if ((lifecycle == CardLifecycle::ReturningToHand)
		|| (lifecycle == CardLifecycle::ReturningToBoard)) {
		return VisualMotionEasing::CubicEaseOut;
	}
	return VisualMotionEasing::Linear;
}

inline constexpr double HandDealStaggerSeconds = 0.1;
inline constexpr double HandDealDurationSeconds = 0.3;
inline constexpr double HandDealTimeEpsilonSeconds = 0.000000001;
inline constexpr std::int32_t HandDealSoundLimit = 8;

enum class HandDealStage {
	RotatePile,
	DealCards,
	ReturnPile,
	Complete,
};

[[nodiscard]] inline constexpr HandDealStage ResolveHandDealStage(
	const bool has_hand_cards, const bool cards_dealt,
	const bool pile_fully_rotated, const bool pile_at_rest) noexcept {
	if (!has_hand_cards) return pile_at_rest ? HandDealStage::Complete : HandDealStage::ReturnPile;
	if (!cards_dealt) {
		return pile_fully_rotated ? HandDealStage::DealCards : HandDealStage::RotatePile;
	}
	return pile_at_rest ? HandDealStage::Complete : HandDealStage::ReturnPile;
}

[[nodiscard]] inline constexpr bool ShouldDrawHandCardDuringDeal(
	const std::int32_t slot, const std::int32_t launched_card_count,
	const bool deal_in_progress) noexcept {
	return !deal_in_progress
		|| ((0 <= slot) && (slot < launched_card_count));
}

[[nodiscard]] inline constexpr bool ShouldPlayHandDealSound(
	const std::int32_t slot) noexcept {
	return (0 <= slot) && (slot < HandDealSoundLimit);
}

struct HandDealProgress {
	bool started = false;
	bool complete = false;
	double linear_progress = 0.0;
	double eased_progress = 0.0;
};

[[nodiscard]] inline constexpr HandDealProgress ResolveHandDealProgress(
	const std::int32_t slot, const double elapsed_seconds) noexcept {
	if ((slot < 0) || (elapsed_seconds < slot * HandDealStaggerSeconds)) return {};
	const double start_seconds = slot * HandDealStaggerSeconds;
	const double local_seconds = elapsed_seconds - start_seconds;
	const double linear_progress = (HandDealDurationSeconds
		<= local_seconds + HandDealTimeEpsilonSeconds)
		? 1.0 : (local_seconds / HandDealDurationSeconds);
	return {
		true,
		linear_progress >= 1.0,
		linear_progress,
		CubicEaseOut(linear_progress),
	};
}

[[nodiscard]] inline constexpr double HandDealTotalDuration(
	const std::int32_t card_count) noexcept {
	return (0 < card_count)
		? ((card_count - 1) * HandDealStaggerSeconds + HandDealDurationSeconds)
		: 0.0;
}

inline constexpr double DiscardCardDurationSeconds = 0.15;
inline constexpr double DiscardFirstStartIntervalSeconds = 0.10;
inline constexpr double DiscardLastStartIntervalSeconds = 0.05;
inline constexpr double DiscardTimeEpsilonSeconds = 0.000000001;

struct DiscardCardProgress {
	bool started = false;
	bool complete = false;
	double linear_progress = 0.0;
};

[[nodiscard]] inline constexpr double DiscardCardStartIntervalSeconds(
	const std::int32_t interval_index, const std::int32_t card_count) noexcept {
	const std::int32_t interval_count = card_count - 1;
	if ((interval_index < 0) || (interval_count <= interval_index)) return 0.0;
	if (interval_count == 1) return DiscardFirstStartIntervalSeconds;
	const double rate = static_cast<double>(interval_index)
		/ static_cast<double>(interval_count - 1);
	return DiscardFirstStartIntervalSeconds
		+ (DiscardLastStartIntervalSeconds - DiscardFirstStartIntervalSeconds) * rate;
}

[[nodiscard]] inline constexpr double DiscardCardStartSeconds(
	const std::int32_t card_index, const std::int32_t card_count) noexcept {
	if ((card_index < 0) || (card_count <= card_index)) return 0.0;
	double start_seconds = 0.0;
	for (std::int32_t interval_index = 0; interval_index < card_index; ++interval_index) {
		start_seconds += DiscardCardStartIntervalSeconds(interval_index, card_count);
	}
	return start_seconds;
}

[[nodiscard]] inline constexpr DiscardCardProgress ResolveDiscardCardProgress(
	const std::int32_t card_index, const std::int32_t card_count,
	const double elapsed_seconds) noexcept {
	if ((card_index < 0) || (card_count <= card_index) || (elapsed_seconds < 0.0)) return {};
	const double start_seconds = DiscardCardStartSeconds(card_index, card_count);
	if (elapsed_seconds + DiscardTimeEpsilonSeconds < start_seconds) return {};
	const double local_seconds = elapsed_seconds - start_seconds;
	const double linear_progress = (DiscardCardDurationSeconds
		<= local_seconds + DiscardTimeEpsilonSeconds)
		? 1.0 : (local_seconds / DiscardCardDurationSeconds);
	return { true, linear_progress >= 1.0, linear_progress };
}

[[nodiscard]] inline constexpr double DiscardCollectionTotalDuration(
	const std::int32_t card_count) noexcept {
	return (0 < card_count)
		? (DiscardCardStartSeconds(card_count - 1, card_count) + DiscardCardDurationSeconds)
		: 0.0;
}

[[nodiscard]] inline bool IsLogicallyInHand(const CardLifecycle lifecycle) noexcept {
	return (lifecycle == CardLifecycle::InHand)
		|| (lifecycle == CardLifecycle::DraggingFromHand)
		|| (lifecycle == CardLifecycle::ReturningToHand);
}

[[nodiscard]] inline bool IsLogicallyOnBoard(const CardLifecycle lifecycle) noexcept {
	return (lifecycle == CardLifecycle::OnBoard)
		|| (lifecycle == CardLifecycle::DraggingFromBoard)
		|| (lifecycle == CardLifecycle::ReturningToBoard);
}

[[nodiscard]] inline bool CanStartCardDrag(const CardLifecycle lifecycle) noexcept {
	return (lifecycle == CardLifecycle::InHand) || (lifecycle == CardLifecycle::OnBoard);
}

[[nodiscard]] inline bool CanBeHandSwapTarget(const CardLifecycle lifecycle) noexcept {
	return lifecycle == CardLifecycle::OnBoard;
}

	inline void StartVisualMotion(VisualMotion& motion, const ScreenPoint start,
		const ScreenPoint end, const double duration_seconds = 0.25) noexcept {
	motion.active = true;
	motion.start = start;
	motion.end = end;
	motion.current = start;
	motion.elapsed_seconds = 0.0;
	motion.duration_seconds = (0.0 < duration_seconds) ? duration_seconds : 0.15;
}

inline bool AdvanceVisualMotion(VisualMotion& motion, const double delta_seconds,
	const VisualMotionEasing easing = VisualMotionEasing::Linear) noexcept {
	if (!motion.active) return false;
	motion.elapsed_seconds += (0.0 < delta_seconds) ? delta_seconds : 0.0;
	const double linear_rate = (motion.duration_seconds <= motion.elapsed_seconds)
		? 1.0 : (motion.elapsed_seconds / motion.duration_seconds);
	const double rate = (easing == VisualMotionEasing::CubicEaseOut)
		? CubicEaseOut(linear_rate) : linear_rate;
	const auto interpolate = [rate](const std::int32_t start, const std::int32_t end) {
		return static_cast<std::int32_t>(start + (end - start) * rate);
	};
	motion.current = {
		interpolate(motion.start.x, motion.end.x),
		interpolate(motion.start.y, motion.end.y),
	};
	if (linear_rate < 1.0) return false;
	motion.current = motion.end;
	motion.active = false;
	return true;
}

inline void CompleteVisualMotion(VisualMotion& motion) noexcept {
	if (!motion.active) return;
	motion.current = motion.end;
	motion.elapsed_seconds = motion.duration_seconds;
	motion.active = false;
}

inline void SettleReturnLifecycle(CardLifecycle& lifecycle) noexcept {
	if (lifecycle == CardLifecycle::ReturningToHand) {
		lifecycle = CardLifecycle::InHand;
	} else if (lifecycle == CardLifecycle::ReturningToBoard) {
		lifecycle = CardLifecycle::OnBoard;
	}
}

[[nodiscard]] inline bool HasUniqueReservedHandSlots(const std::vector<ReservedHandSlot>& reservations) noexcept {
	for (std::size_t i = 0; i < reservations.size(); ++i) {
		if ((reservations[i].card_id == EmptyCardId) || (reservations[i].slot < 0)) return false;
		for (std::size_t j = 0; j < i; ++j) {
			if ((reservations[i].card_id == reservations[j].card_id)
				|| (reservations[i].slot == reservations[j].slot)) return false;
		}
	}
	return true;
}

[[nodiscard]] inline bool OccupantsReferenceKnownCards(
	const BoardSnapshot& board,
	const std::vector<CardId>& known_cards) noexcept {
	for (const BoardCell& cell : board.cells) {
		if (cell.occupant == EmptyCardId) continue;
		bool known = false;
		for (const CardId card_id : known_cards) {
			if (cell.occupant == card_id) {
				known = true;
				break;
			}
		}
		if (!known) return false;
	}
	return true;
}

} // namespace BattleCardRules

#endif
