#ifndef BattleCardRules_HPP
#define BattleCardRules_HPP

#include <algorithm>
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

	bool contains(Cell cell) const {
		return (0 <= cell.x) && (cell.x < width) && (0 <= cell.y) && (cell.y < height);
	}

	const BoardCell* at(Cell cell) const {
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

inline DropDecision ResolveDrop(const DropRequest& request, const BoardSnapshot& board) {
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

inline bool CanSwapBoardCards(
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

inline bool ShouldUseAutoRotatedPlacement(
	DropResult current, DropResult rotated,
	bool manually_rotated = false) {
	return !manually_rotated
		&& (current == DropResult::ReturnToHand) && (rotated == DropResult::Place);
}

enum class PointerInputOwner {
	None,
	Card,
	Deck,
	Attack,
};

inline bool BeginOneShotTransition(bool& started) {
	// 複数フレーム更新でもシーン遷移を一度だけ開始する．
	if (started) return false;
	started = true;
	return true;
}

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

inline CardDrawLayer GetCardDrawLayer(CardLifecycle lifecycle) {
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

inline bool CanProcessBoardInput(
	bool can_accept_battle_input,
	bool hand_capture_failed,
	PointerInputOwner pointer_owner) {
	// ポインター所有権と盤面ロックを統合して入力可否を決める．
	return can_accept_battle_input && !hand_capture_failed
		&& ((pointer_owner == PointerInputOwner::Card)
			|| (pointer_owner == PointerInputOwner::None));
}

inline double CubicEaseOut(double progress) {
	const double inverse = 1.0 - progress;
	return 1.0 - inverse * inverse * inverse;
}

inline constexpr double CardReturnDuration = 0.25;

struct ReturnMotion {
	bool active = false;
	double elapsedSeconds = 0.0;

	void Start() {
		active = true;
		elapsedSeconds = 0.0;
	}

	bool Update(double deltaSeconds) {
		if (!active) return false;
		elapsedSeconds = std::min(elapsedSeconds + deltaSeconds, CardReturnDuration);
		if (elapsedSeconds < CardReturnDuration) return false;
		active = false;
		return true;
	}

	void Complete() {
		if (!active) return;
		elapsedSeconds = CardReturnDuration;
		active = false;
	}

	double Rate() const {
		return CubicEaseOut(elapsedSeconds / CardReturnDuration);
	}
};

inline constexpr double HandDealStaggerSeconds = 0.1;
inline constexpr double HandDealDurationSeconds = 0.3;
inline constexpr double HandDealTimeEpsilonSeconds = 0.000000001;
inline constexpr std::int32_t HandDealSoundLimit = 8;

inline bool ShouldPlayHandDealSound(
	std::int32_t slot) {
	return (0 <= slot) && (slot < HandDealSoundLimit);
}

struct HandDealProgress {
	bool started = false;
	bool complete = false;
	double linear_progress = 0.0;
	double eased_progress = 0.0;
};

inline HandDealProgress ResolveHandDealProgress(
	std::int32_t slot, double elapsed_seconds) {
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

inline double HandDealTotalDuration(
	std::int32_t card_count) {
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

inline double DiscardCardStartIntervalSeconds(
	std::int32_t interval_index, std::int32_t card_count) {
	const std::int32_t interval_count = card_count - 1;
	if ((interval_index < 0) || (interval_count <= interval_index)) return 0.0;
	if (interval_count == 1) return DiscardFirstStartIntervalSeconds;
	const double rate = static_cast<double>(interval_index)
		/ (interval_count - 1);
	return DiscardFirstStartIntervalSeconds
		+ (DiscardLastStartIntervalSeconds - DiscardFirstStartIntervalSeconds) * rate;
}

inline double DiscardCardStartSeconds(
	std::int32_t card_index, std::int32_t card_count) {
	if ((card_index < 0) || (card_count <= card_index)) return 0.0;
	double start_seconds = 0.0;
	for (std::int32_t interval_index = 0; interval_index < card_index; ++interval_index) {
		start_seconds += DiscardCardStartIntervalSeconds(interval_index, card_count);
	}
	return start_seconds;
}

inline DiscardCardProgress ResolveDiscardCardProgress(
	std::int32_t card_index, std::int32_t card_count,
	double elapsed_seconds) {
	if ((card_index < 0) || (card_count <= card_index) || (elapsed_seconds < 0.0)) return {};
	const double start_seconds = DiscardCardStartSeconds(card_index, card_count);
	if (elapsed_seconds + DiscardTimeEpsilonSeconds < start_seconds) return {};
	const double local_seconds = elapsed_seconds - start_seconds;
	const double linear_progress = (DiscardCardDurationSeconds
		<= local_seconds + DiscardTimeEpsilonSeconds)
		? 1.0 : (local_seconds / DiscardCardDurationSeconds);
	return { true, linear_progress >= 1.0, linear_progress };
}

inline bool IsLogicallyInHand(CardLifecycle lifecycle) {
	return (lifecycle == CardLifecycle::InHand)
		|| (lifecycle == CardLifecycle::DraggingFromHand)
		|| (lifecycle == CardLifecycle::ReturningToHand);
}

inline bool IsLogicallyOnBoard(CardLifecycle lifecycle) {
	return (lifecycle == CardLifecycle::OnBoard)
		|| (lifecycle == CardLifecycle::DraggingFromBoard)
		|| (lifecycle == CardLifecycle::ReturningToBoard);
}

inline bool CanStartCardDrag(CardLifecycle lifecycle) {
	return (lifecycle == CardLifecycle::InHand) || (lifecycle == CardLifecycle::OnBoard);
}

inline bool CanBeHandSwapTarget(CardLifecycle lifecycle) {
	return lifecycle == CardLifecycle::OnBoard;
}

inline void SettleReturnLifecycle(CardLifecycle& lifecycle) {
	if (lifecycle == CardLifecycle::ReturningToHand) {
		lifecycle = CardLifecycle::InHand;
	} else if (lifecycle == CardLifecycle::ReturningToBoard) {
		lifecycle = CardLifecycle::OnBoard;
	}
}

} // namespace BattleCardRules

#endif
