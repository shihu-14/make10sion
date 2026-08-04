#ifndef BattleCardRules_HPP
#define BattleCardRules_HPP

#include <cstdint>
#include <vector>

namespace BattleCardRules {

using CardId = std::int32_t;
inline constexpr CardId EmptyCardId = -1;

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
		if (!overlapping_cards.empty()) {
			decision.result = DropResult::RestoreToBoard;
			return decision;
		}
		if (invalid) {
			decision.result = DropResult::ReturnToHand;
			return decision;
		}
		if ((request.candidate_anchor == request.original_anchor) || request.near_start) {
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
	return idle && !board_locked && !scene_transition_started;
}

inline bool BeginOneShotTransition(bool& started) noexcept {
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

inline void StartVisualMotion(VisualMotion& motion, const ScreenPoint start,
	const ScreenPoint end, const double duration_seconds = 0.15) noexcept {
	motion.active = true;
	motion.start = start;
	motion.end = end;
	motion.current = start;
	motion.elapsed_seconds = 0.0;
	motion.duration_seconds = (0.0 < duration_seconds) ? duration_seconds : 0.15;
}

inline bool AdvanceVisualMotion(VisualMotion& motion, const double delta_seconds) noexcept {
	if (!motion.active) return false;
	motion.elapsed_seconds += (0.0 < delta_seconds) ? delta_seconds : 0.0;
	const double rate = (motion.duration_seconds <= motion.elapsed_seconds)
		? 1.0 : (motion.elapsed_seconds / motion.duration_seconds);
	const auto interpolate = [rate](const std::int32_t start, const std::int32_t end) {
		return static_cast<std::int32_t>(start + (end - start) * rate);
	};
	motion.current = {
		interpolate(motion.start.x, motion.end.x),
		interpolate(motion.start.y, motion.end.y),
	};
	if (rate < 1.0) return false;
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
