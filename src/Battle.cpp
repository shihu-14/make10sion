#include "Battle.hpp"
#include "Board.hpp"
#include <algorithm>
#include <cassert>
#include <tuple>
namespace Layout = BattleLayoutRules;
using BattleCombat::AttackPhase;

namespace {

const RectF BgmSliderTrack{ 650, 390, 650, 20 };
const RectF SeSliderTrack{ 650, 560, 650, 20 };
const RectF SettingsCloseButton{ 1600, 800, 225, 225 };

}

Battle::Battle(const InitData& init)
	: IScene(init),
	m_cards(getData().Deck)
{
	// 画像を読み込む．フォントと，ぼかしに使う描画先もここで用意しとく
	m_backgroundTexture = Texture(U"../../image/haikei_sentou.png");
	m_playerTexture = Texture(U"../../image/chara_player.png");
	m_drawPileTexture = Texture(U"../../image/yamahuda.png");
	m_discardPileTexture = Texture(U"../../image/sutehuda.png");
	m_attackButtonTexture = Texture(U"../../image/bottun_equal.png");
	m_effectTexture = Texture(U"../../image/effect_attack.png");
	m_attackIcon = Texture(U"../../image/icon_attack.png");
	m_defenseIcon = Texture(U"../../image/icon_seild.png");
	m_rewardTexture = Texture(U"../../image/UI_money.png");
	m_settingsBackgroundTexture = Texture(U"../../image/deck_background.png");
	m_settingsBackButtonTexture = Texture(U"../../image/back_button_deck0.png");
	m_rewardFont = Font{ 50, Typeface::Bold };
	m_numFont = Font{ 64, Typeface::Bold };
	m_combatFont = Font{ Layout::CombatFontSize, Typeface::Bold };
	m_sceneBuffer = RenderTexture(Scene::Size());
	m_blurBuffer = RenderTexture(Scene::Size());

	// ここから，この戦闘の盤面・敵・山札を準備する．
	m_cardCount = static_cast<int32>(m_cards.size());
	m_board.BeginBattle(getData().board_progress);
	m_handLimit = handLimit();
	setupEnemy(getData().enemy, GameStateRules::ActIndex(getData().Layer));
	m_playerHpBar = HPBar{ getData().MaxHP, getData().HP };
	m_actionCycle = static_cast<int32>(m_enemy.actions.size());
	m_reward = m_enemy.type == 0 ? 20 : m_enemy.type == 1 ? 40 : 100;
	// デモでは，毎回同じ手札を確認できるように並びを固定している．
	const bool preserve_deck_order = getData().demo_battle_overrides
		&& getData().demo_battle_overrides->preserve_deck_order;
	auto initial_draw_order = GameStateRules::CreateInitialDrawOrder(
		m_cardCount, preserve_deck_order);
	Array<int32> draw_order{ initial_draw_order.begin(), initial_draw_order.end() };
	for (int32 i = 0; i < m_cardCount; i++) {
		m_cards[i].ResetRuntimeState();
	}
	if (!preserve_deck_order) {
		draw_order.shuffle();
	}
	m_deck.Initialize(m_cardCount,
		std::vector<int32>{ draw_order.begin(), draw_order.end() });

	applyAudioSettings();
	m_battleBgm.play();
	beginDraw();
}

void Battle::update()
{
	BoardInputFrame input{ Cursor::Pos(), MouseL.down(), MouseL.pressed(), MouseL.up(),
		KeyR.down(), Window::GetState().focused, Scene::DeltaTime() };
	if (!input.focused || m_phase != Phase::Planning) {
		m_board.CancelActiveDrag();
		applyBoardChanges();
		if (!input.focused) {
			m_pointerOwner = BattleCardRules::PointerInputOwner::None;
			m_banner.CancelPointerGesture();
		}
	}
	// 設定やデッキを開いている間は，この先の戦闘更新を止める．
	if (updateOverlay(input)) return;
	if (m_phase != Phase::Planning) {
		// 配置できない間も，バナーの表示と盤面の見た目は更新する．
		if (updateDeckButton(input, false)) return;
		updateCardInput(input, -1);
		m_playerHpBar.update(0.1);
		m_enemyHpBar.update(0.1);
		m_deck.RecycleDiscard();
		m_playerAttackPulseTime = m_playerDefensePulseTime = Layout::CombatStatPulseDuration;
	}
	switch (m_phase) {
	case Phase::Planning: updatePlanning(input); break;
	case Phase::PlayerAttack:
	case Phase::EnemyAttack: updateAttack(input.delta_seconds); break;
	case Phase::Discarding: updateDiscard(); break;
	case Phase::Drawing: updateDraw(); break;
	case Phase::Victory: updateVictory(); break;
	case Phase::Defeat: updateDefeat(); break;
	}
}

void Battle::setupEnemy(int32 type, int32 layer)
{
	const EnemyData& data = m_enemyDB.getOneEnemy(type, layer);
	// デモ用の画像が指定されていれば，そちらを使う．
	const String texture_path = getData().demo_battle_overrides
		&& !getData().demo_battle_overrides->enemy_texture_path.isEmpty()
		? getData().demo_battle_overrides->enemy_texture_path : data.texturePath;
	m_enemy.texture = Texture(texture_path);
	m_enemyBaseScale = Layout::EnemyBaseScale(m_enemy.texture.height());
	m_enemyHitScale = 1.0;
	m_enemy.type = data.type;
	m_enemy.maxHp = data.maxHp;
	m_enemy.hp = data.maxHp;
	m_enemy.actions = data.actionPattern;
	m_enemyHpBar = HPBar{m_enemy.maxHp, m_enemy.hp};
}

int32 Battle::handLimit() const
{
	int32 override_limit = getData().demo_battle_overrides
		? getData().demo_battle_overrides->hand_limit : 0;
	return GameStateRules::ResolveBattleHandLimit(getData().board_progress, override_limit);
}

Point Battle::handPosition(int32 slot) const
{
	const auto& hand = cards(GameStateRules::CardZone::Hand);
	// カードの幅がまちまちなので，前のカードの幅を足して場所を決める．
	int32 widthBefore = 0;
	for (int32 index = 0; index < slot; ++index) {
		widthBefore += m_cards[hand[index]].Size().first;
	}
	int32 cardWidth = m_cards[hand[slot]].Size().first;
	return Layout::HandPosition(slot, widthBefore, cardWidth);
}

void Battle::prepareDrawPile()
{
	// 山札が残っていても，使えるマスの半分を埋めるには足りないことがある．
	int32 remaining_cells = 0;
	for (int32 cardId : cards(GameStateRules::CardZone::DrawPile)) {
		remaining_cells += m_cards[cardId].OccupiedCellCount();
	}
	int32 full_deck_cells = 0;
	for (const Block& card : m_cards) full_deck_cells += card.OccupiedCellCount();
	if (!GameStateRules::ShouldRefreshDrawPile(remaining_cells, full_deck_cells,
		getData().board_progress.UnlockedCount())) return;

	const bool preserve_deck_order = getData().demo_battle_overrides
		&& getData().demo_battle_overrides->preserve_deck_order;
	auto initial_order = GameStateRules::CreateInitialDrawOrder(m_cardCount, preserve_deck_order);
	Array<int32> draw_order{ initial_order.begin(), initial_order.end() };
	if (!preserve_deck_order) draw_order.shuffle();
	const bool refreshed = m_deck.RefreshDrawPile(
		std::vector<int32>{ draw_order.begin(), draw_order.end() });
	assert(refreshed && "Draw pile refresh failed at a turn boundary");
	if (!refreshed) return;
}

void Battle::applyAudioSettings()
{
	const auto& settings = getData().audio_settings;
	m_battleBgm.setVolume(GameStateRules::ClampVolume(settings.bgm_volume));
	m_drawCardSound.setVolume(GameStateRules::ClampVolume(settings.se_volume));
	m_dragCardSound.setVolume(GameStateRules::ClampVolume(settings.se_volume));
	m_attackSound.setVolume(GameStateRules::ClampVolume(settings.se_volume));
}

void Battle::updateSettings(const BoardInputFrame& input)
{
	if (!input.focused) m_activeVolumeSlider = VolumeSlider::None;
	// 設定からデッキへ移っても，戦闘の時間は止めたまま．
	if (input.focused && input.left_down && m_banner.IsDeckButtonHovered(input.cursor)) {
		m_banner.OpenDeck(m_cards, cards(GameStateRules::CardZone::Discard));
		m_activeVolumeSlider = VolumeSlider::None;
		m_pointerOwner = BattleCardRules::PointerInputOwner::Deck;
		m_overlay = Overlay::Deck;
		return;
	}
	if (KeyEscape.down() || (input.left_down && SettingsCloseButton.contains(input.cursor))) {
		m_overlay = Overlay::None;
		m_activeVolumeSlider = VolumeSlider::None;
		m_phaseTimer.resume();
		return;
	}

	if (input.left_down) {
		if (BgmSliderTrack.stretched(20).contains(input.cursor)) {
			m_activeVolumeSlider = VolumeSlider::Bgm;
		} else if (SeSliderTrack.stretched(20).contains(input.cursor)) {
			m_activeVolumeSlider = VolumeSlider::Se;
		}
	}
	if ((input.left_down || input.left_pressed) && (m_activeVolumeSlider != VolumeSlider::None)) {
		auto& settings = getData().audio_settings;
		const RectF& track = (m_activeVolumeSlider == VolumeSlider::Bgm)
			? BgmSliderTrack : SeSliderTrack;
		double volume = GameStateRules::SliderVolumeAt(input.cursor.x, track.x, track.w);
		if (m_activeVolumeSlider == VolumeSlider::Bgm) settings.bgm_volume = volume;
		else settings.se_volume = volume;
		applyAudioSettings();
	}
	if (input.left_up) m_activeVolumeSlider = VolumeSlider::None;

	const bool interactive = SettingsCloseButton.contains(input.cursor)
		|| BgmSliderTrack.stretched(20).contains(input.cursor)
		|| SeSliderTrack.stretched(20).contains(input.cursor);
	if (interactive) Cursor::RequestStyle(CursorStyle::Hand);
}

const std::vector<int32>& Battle::cards(const GameStateRules::CardZone zone) const
{
	return m_deck.Cards(zone);
}

bool Battle::moveCard(int32 cardId, const GameStateRules::CardZone expected,
	const GameStateRules::CardZone destination)
{
	if (!m_deck.Move(cardId, expected, destination)) return false;
	if (GameStateRules::NeedsBoardDetach(expected, destination)) m_board.DetachCard(cardId);
	return true;
}

void Battle::applyBoardChanges()
{
	for (const auto& change : m_board.ConsumeZoneChanges()) {
		const bool moved = moveCard(change.card_id, change.expected, change.destination);
		assert(moved && "Board and deck membership disagree");
		if (!moved) return;
	}
}

void Battle::updateEnemyIntent()
{
	if ((m_phase != Phase::Planning) || m_enemy.actions.isEmpty()) return;
	if (!m_enemyIntent.has_action) {
		int32 pattern_size = static_cast<int32>(m_enemy.actions.size());
		int32 action_index = (m_turn % m_actionCycle + m_turnStart) % pattern_size;
		const EnemyAction& action = m_enemy.actions[action_index];
		EnemyIntentRules::PrepareAction(m_enemyIntent, action.attack, action.defense);
	}
	// お金を減らす処理はターンに1回だけ．手札に応じた攻撃力は，配置中も変わる．
	auto intent = EnemyIntentRules::ResolveFrame(m_enemyIntent.side_effects_prepared, {
		m_enemyIntent.raw_attack,
		m_enemyIntent.raw_defense,
		m_handLimit,
		static_cast<int32>(cards(GameStateRules::CardZone::Hand).size()),
	});
	m_enemyStats.attack = intent.attack;
	m_enemyStats.defense = intent.defense;
	m_enemyDisplay = m_enemyStats;
	m_blocksPlayerDamage = intent.cancels_player_damage;
	getData().money += intent.money_delta;
	if (intent.turn_advance != 0) {
		m_turn += intent.turn_advance;
		m_turnStart = m_turn;
	}
	if (intent.action_cycle_override != 0) m_actionCycle = intent.action_cycle_override;
}

void Battle::beginDiscard()
{
	m_hitPosition = { -1, -1 };
	m_playerAngle = 0.0;
	m_discardMotions.clear();
	m_discardStarted = false;
	m_phase = Phase::Discarding;
	m_phaseTimer.restart();
}

void Battle::updateDiscard()
{
	const auto& hand = cards(GameStateRules::CardZone::Hand);
	if (!m_discardStarted && !hand.empty() && (m_discardPileAngle > -90_deg)) {
		m_discardPileAngle -= Scene::DeltaTime() * 4.0;
		m_phaseTimer.restart();
		return;
	}
	if (!m_discardStarted) {
		if (!hand.empty()) m_discardPileAngle = -90_deg;
		m_discardMotions.reserve(hand.size());
		for (int32 cardId : hand) {
			if (!m_board.ShouldDrawAsHand(cardId)) continue;
			auto hand_position = m_cards[cardId].GetPos();
			m_discardMotions.push_back({
				cardId,
				Vec2{ hand_position.first, hand_position.second },
			});
		}
		// 右にある手札から順にしまう
		std::stable_sort(m_discardMotions.begin(), m_discardMotions.end(),
			[](const DiscardMotion& left, const DiscardMotion& right) {
				return left.start.x > right.start.x;
			});
		m_discardStarted = true;
		m_phaseTimer.restart();
	}

	bool collection_complete = true;
	double elapsed_seconds = m_phaseTimer.sF();
	int32 card_count = static_cast<int32>(m_discardMotions.size());
	auto discard_target = Layout::DiscardTarget;
	const Vec2 target{ discard_target };
	for (int32 card_index = 0; card_index < card_count; ++card_index) {
		DiscardMotion& motion = m_discardMotions[card_index];
		if (motion.complete) continue;
		auto progress = BattleCardRules::ResolveDiscardCardProgress(
			card_index, card_count, elapsed_seconds);
		if (!progress.started) {
			collection_complete = false;
			continue;
		}
		// 位置を動かす前に Board から外す．
		// Board はカードが手札の定位置にあるものとして扱っているため．
		if (!motion.detached) {
			m_board.DetachCard(motion.cardId);
			motion.detached = true;
		}
		const Vec2 pos = motion.start.lerp(target, progress.linear_progress);
		m_cards[motion.cardId].SetPos(pos.x, pos.y);
		if (!progress.complete) {
			collection_complete = false;
			continue;
		}
		if (!moveCard(motion.cardId, GameStateRules::CardZone::Hand,
			GameStateRules::CardZone::Discard)) {
			assert(false && "Failed to move hand card to discard after animation");
			return;
		}
		motion.complete = true;
	}
	if (!collection_complete) return;

	if (m_discardPileAngle < 0.01) {
		m_discardPileAngle += Scene::DeltaTime() * 7.0;
		return;
	}
	// 次のターンの効果を確定してから，盤面のカードを捨てる．順番に注意．
	m_board.EndTurn();
	// 移動するたび元の一覧が変わるので，コピーを回す．
	auto board_cards = cards(GameStateRules::CardZone::Board);
	for (int32 deck_index : board_cards) {
		moveCard(deck_index, GameStateRules::CardZone::Board, GameStateRules::CardZone::Discard);
	}
	prepareDrawPile();
	m_board.BeginTurn();
	m_discardPileAngle = 0.0;
	beginDraw();
}

void Battle::beginDraw()
{
	m_dealtCardCount = 0;
	m_discardMotions.clear();
	m_discardStarted = false;
	m_dealPhase = DealPhase::RotatePile;
	m_phase = Phase::Drawing;
	m_phaseTimer.restart();
}

void Battle::updateDraw()
{
	// 所属だけ先に手札へ変えて，見た目はあとから追いつかせる．
	const auto& drawPile = cards(GameStateRules::CardZone::DrawPile);
	const auto& hand = cards(GameStateRules::CardZone::Hand);
	while (!drawPile.empty() && static_cast<int32>(hand.size()) < m_handLimit) {
		int32 cardId = drawPile.back();
		moveCard(cardId, GameStateRules::CardZone::DrawPile, GameStateRules::CardZone::Hand);
		m_cards[cardId].SetPos(50, 900);
	}

	if (m_dealPhase == DealPhase::RotatePile) {
		if (hand.empty()) {
			m_dealPhase = DealPhase::ReturnPile;
		} else if (m_drawPileAngle < 90_deg) {
			m_drawPileAngle += Scene::DeltaTime() * 4.0;
			m_phaseTimer.restart();
			return;
		} else {
			m_drawPileAngle = 90_deg;
			m_dealPhase = DealPhase::Cards;
		}
	}

	// 山札の回転が終わってから，カードごとに少しずつ時間をずらして配る．
	if (m_dealPhase == DealPhase::Cards) {
		double elapsed = m_phaseTimer.sF();
		while (m_dealtCardCount < static_cast<int32>(hand.size())
			&& BattleCardRules::ResolveHandDealProgress(m_dealtCardCount, elapsed).started) {
			if (BattleCardRules::ShouldPlayHandDealSound(m_dealtCardCount)) {
				m_drawCardSound.playOneShot(getData().audio_settings.se_volume);
			}
			++m_dealtCardCount;
		}

		Vec2 from{ 50, 900 };
		for (int32 slot = 0; slot < static_cast<int32>(hand.size()); ++slot) {
			auto progress = BattleCardRules::ResolveHandDealProgress(slot, elapsed);
			if (!progress.started) continue;
			Vec2 position = from.lerp(handPosition(slot), progress.eased_progress);
			m_cards[hand[slot]].SetPos(position.x, position.y);
		}
		if (elapsed < BattleCardRules::HandDealTotalDuration(static_cast<int32>(hand.size()))) return;
		m_dealPhase = DealPhase::ReturnPile;
		return;
	}

	if (m_drawPileAngle > 0.01) {
		m_drawPileAngle -= Scene::DeltaTime() * 6.0;
		return;
	}
	m_drawPileAngle = 0.0;
	beginPlanning();
}

void Battle::beginPlanning()
{
	m_phase = Phase::Planning;
	m_phaseTimer.restart();
	++m_turn;
	m_enemyIntent = {};
	m_playerAttackPulseTime = Min(Layout::CombatStatPulseDuration, m_playerAttackPulseTime + Scene::DeltaTime());
	m_playerDefensePulseTime = Min(Layout::CombatStatPulseDuration, m_playerDefensePulseTime + Scene::DeltaTime());
	// 配札が終わったフレームで表示を用意し，入力は次のフレームから受け付ける．
	m_board.ConsumeCalculationChange();
	refreshCombatPreview();
}

void Battle::updateVictory()
{
	// 敵を2秒で消して，残りの3秒は報酬を見せる．
	if (m_phaseTimer.sF() < 2.0)
	{
		m_enemyAlpha = Math::Lerp(1.0, 0.0, m_phaseTimer.sF()/2.0);
		return;
	}
	m_enemyAlpha = 0.0;
	if (m_phaseTimer.sF() < 5.0)
	{
		return;
	}
	if (!BattleCardRules::BeginOneShotTransition(m_transitionStarted)) return;
	getData().money += m_reward;
	auto progress = GameStateRules::ResolveVictory(m_enemy.type, getData().Layer);
	if (progress.destination == GameStateRules::VictoryDestination::Result) {
		getData().run_outcome = GameStateRules::RunOutcome::Clear;
		changeScene(State::Result);
	} else if (progress.destination == GameStateRules::VictoryDestination::NextActBattle) {
		getData().Layer = progress.next_layer;
		getData().Index = 1;
		getData().enemy = 0;
		changeScene(State::Battle);
	} else {
		changeScene(State::Map);
	}
}

void Battle::updateDefeat()
{
	if (m_phaseTimer.sF() < 1.0)
	{
		return;
	}
	if (!BattleCardRules::BeginOneShotTransition(m_transitionStarted)) return;
	getData().run_outcome = GameStateRules::RunOutcome::GameOver;
	changeScene(State::Result);
}

void Battle::refreshCombatPreview()
{
	auto [attack, defense] = m_board.Confirm();
	if (attack != m_playerStats.attack) m_playerAttackPulseTime = 0.0;
	if (defense != m_playerStats.defense) m_playerDefensePulseTime = 0.0;
	m_playerStats = { attack, defense };
	m_playerDisplay = m_playerStats;
}

void Battle::beginPlayerAttack()
{
	// この先は，演出中に盤面の値を読み直さない．
	m_turnResult = BattleCombat::ResolveCombat(m_playerStats, m_enemyStats,
		getData().HP, m_enemy.hp, m_blocksPlayerDamage);
	m_attack.Start(m_turnResult.player.attack, m_turnResult.enemy.defense,
		m_turnResult.damageToEnemy, m_enemy.maxHp, 0.4);
	m_hitPosition = { -1, -1 };
	m_phase = Phase::PlayerAttack;
}

void Battle::beginEnemyAttack()
{
	m_hitPosition = { -1, -1 };
	m_enemyHitScale = 1.0;
	m_attack.Start(m_turnResult.enemy.attack, m_turnResult.player.defense,
		m_turnResult.damageToPlayer, getData().MaxHP);
	m_phase = Phase::EnemyAttack;
}

void Battle::updateAttack(double deltaSeconds)
{
	bool playerAttacking = m_phase == Phase::PlayerAttack;
	auto events = m_attack.Update(deltaSeconds);
	auto& attacker = playerAttacking ? m_playerDisplay : m_enemyDisplay;
	auto& defender = playerAttacking ? m_enemyDisplay : m_playerDisplay;
	attacker.attack = m_attack.attack;
	defender.defense = m_attack.defense;
	m_enemyHitScale = Min(1.0, m_enemyHitScale + deltaSeconds * Layout::EnemyScaleRecoveryRate);
	m_playerAngle = Min(0.0, m_playerAngle + deltaSeconds);
	if (events.shieldHit || events.hit) {
		m_attackSound.playOneShot(getData().audio_settings.se_volume);
	}
	if (events.hit) {
		auto bounds = playerAttacking ? Layout::EnemyDamageEffectBounds : Layout::PlayerDamageEffectBounds;
		m_hitPosition = { Random(bounds.x, bounds.x + bounds.w), Random(bounds.y, bounds.y + bounds.h) };
		if (playerAttacking) {
			m_enemy.hp = BattleDamageRules::ApplyHit(m_enemy.hp, events.damage);
			m_enemyHpBar.damage(events.damage);
			m_enemyHitScale = Layout::EnemyHitScaleMultiplier;
		} else {
			getData().HP = BattleDamageRules::ApplyHit(getData().HP, events.damage);
			m_playerHpBar.damage(events.damage);
			m_playerAngle = Random(-0.52, -0.1);
		}
	}
	// 最後のヒットを反映してから，勝敗と次の攻撃を判定する．
	if (m_enemy.hp <= 0 || getData().HP <= 0) {
		m_phase = m_enemy.hp <= 0 ? Phase::Victory : Phase::Defeat;
		m_phaseTimer.restart();
		return;
	}
	// 最後のヒットも1フレーム描いてから，次の攻撃へ進む．
	if (events.hit || m_attack.phase != AttackPhase::Complete) return;
	if (playerAttacking) {
		beginEnemyAttack();
	} else {
		beginDiscard();
	}
}

int32 Battle::hoveredHandCard(Point cursor) const
{
	auto& hand = cards(GameStateRules::CardZone::Hand);
	// 後ろの要素ほど手前に描かれるので，逆から調べる．
	for (auto it = hand.rbegin(); it != hand.rend(); ++it) {
		if (m_board.CanStartHandDrag(*it) && m_cards[*it].IsHovered(cursor)) return *it;
	}
	return -1;
}

bool Battle::updateOverlay(const BoardInputFrame& input)
{
	if (m_overlay == Overlay::Deck) {
		if (input.focused && m_banner.IsSettingButtonHovered(input.cursor)) {
			Cursor::RequestStyle(CursorStyle::Hand);
			if (input.left_down) {
				m_banner.CloseDeck();
				m_overlay = Overlay::Settings;
				m_activeVolumeSlider = VolumeSlider::None;
				m_pointerOwner = BattleCardRules::PointerInputOwner::None;
				return true;
			}
		}
		bool open = m_banner.update(m_cards, false, input.cursor, input.left_down,
			input.left_up, input.focused, cards(GameStateRules::CardZone::Discard));
		m_board.Update(0, getData().leric.getLeric(), input, false);
		if (!open) {
			m_overlay = Overlay::None;
			m_pointerOwner = BattleCardRules::PointerInputOwner::None;
			m_phaseTimer.resume();
		}
		return true;
	}
	if (m_overlay == Overlay::Settings) {
		updateSettings(input);
		return true;
	}
	if (input.focused && m_banner.IsSettingButtonHovered(input.cursor)) {
		Cursor::RequestStyle(CursorStyle::Hand);
		if (input.left_down && !m_board.IsDragging()) {
			m_board.CompleteVisualMotions();
			applyBoardChanges();
			m_pointerOwner = BattleCardRules::PointerInputOwner::None;
			m_banner.CancelPointerGesture();
			m_activeVolumeSlider = VolumeSlider::None;
			m_overlay = Overlay::Settings;
			m_phaseTimer.pause();
			return true;
		}
	}
	return false;
}

bool Battle::updateDeckButton(const BoardInputFrame& input, bool allowOpen)
{
	if (!m_banner.update(m_cards, allowOpen, input.cursor, input.left_down,
		input.left_up, input.focused, cards(GameStateRules::CardZone::Discard))) return false;
	m_board.CompleteVisualMotions();
	m_overlay = Overlay::Deck;
	m_phaseTimer.pause();
	return true;
}

void Battle::updatePlanning(const BoardInputFrame& input)
{
	updateEnemyIntent();
	bool canInteract = !m_transitionStarted && input.focused;
	if (canInteract) {
		auto& hand = cards(GameStateRules::CardZone::Hand);
		for (int32 slot = 0; slot < static_cast<int32>(hand.size()); ++slot) {
			int32 cardId = hand[slot];
			auto position = m_cards[cardId].GetPos();
			bool registered = m_board.RegisterHandBlock(m_cards[cardId], cardId,
				slot, { position.first, position.second });
			if (!registered) {
				assert(false && "Hand registration failed");
				return;
			}
		}
	}
	int32 hoveredCardId = hoveredHandCard(input.cursor);
	if (m_board.IsDragging()) m_pointerOwner = BattleCardRules::PointerInputOwner::Card;
	// 押した場所で操作の相手を決めて，離すまでは変えない．
	if (m_pointerOwner == BattleCardRules::PointerInputOwner::None && input.left_down && canInteract) {
		if (m_banner.IsDeckButtonHovered(input.cursor)) {
			m_pointerOwner = BattleCardRules::PointerInputOwner::Deck;
		} else if (Layout::EqualButtonBounds.contains(input.cursor)) {
			m_pointerOwner = BattleCardRules::PointerInputOwner::Attack;
		} else if (hoveredCardId >= 0 || m_board.GetBoardCellAt(input.cursor) != Point{ -1, -1 }) {
			m_pointerOwner = BattleCardRules::PointerInputOwner::Card;
		}
	}
	bool allowDeck = canInteract && !m_board.IsDragging()
		&& (m_pointerOwner == BattleCardRules::PointerInputOwner::None
			|| m_pointerOwner == BattleCardRules::PointerInputOwner::Deck);
	if (updateDeckButton(input, allowDeck)) return;
	if (canInteract && Layout::EqualButtonBounds.contains(input.cursor)) Cursor::RequestStyle(CursorStyle::Hand);
	if (canInteract && m_pointerOwner == BattleCardRules::PointerInputOwner::Attack
		&& input.left_down && !m_board.IsDragging()) {
		m_board.CompleteVisualMotions();
		refreshCombatPreview();
		beginPlayerAttack();
		return;
	}
	updateCardInput(input, hoveredCardId);
	m_playerHpBar.update(0.1);
	m_enemyHpBar.update(0.1);
	m_deck.RecycleDiscard();
	m_playerAttackPulseTime = Min(Layout::CombatStatPulseDuration, m_playerAttackPulseTime + input.delta_seconds);
	m_playerDefensePulseTime = Min(Layout::CombatStatPulseDuration, m_playerDefensePulseTime + input.delta_seconds);
	if (m_board.ConsumeCalculationChange()) refreshCombatPreview();
}

void Battle::updateCardInput(const BoardInputFrame& input, int32 hoveredCardId)
{
	bool canInteract = m_phase == Phase::Planning && !m_transitionStarted && input.focused;
	// 手札をつかめなかったクリックで，奥の盤面まで反応させない．
	bool handCaptureFailed = false;
	if (canInteract && hoveredCardId >= 0) {
		Cursor::RequestStyle(CursorStyle::Hand);
		if (m_pointerOwner == BattleCardRules::PointerInputOwner::Card
			&& input.left_down && !m_board.IsDragging()) {
			handCaptureFailed = !m_board.PassBlock(m_cards[hoveredCardId], hoveredCardId, input.cursor);
			if (!handCaptureFailed) m_dragCardSound.playOneShot(getData().audio_settings.se_volume);
		}
	}
	m_board.Update(0, getData().leric.getLeric(), input,
		BattleCardRules::CanProcessBoardInput(canInteract, handCaptureFailed, m_pointerOwner));
	applyBoardChanges();
	if (input.left_up || !input.focused || (!input.left_down && !input.left_pressed)) {
		m_banner.CancelPointerGesture();
		m_pointerOwner = BattleCardRules::PointerInputOwner::None;
	}
}

void Battle::drawHand() const
{
	auto& hand = cards(GameStateRules::CardZone::Hand);
	bool dealing = m_phase == Phase::Drawing && m_dealPhase != DealPhase::ReturnPile;
	int32 hoveredCardId = -1;
	if (m_phase == Phase::Planning && !m_transitionStarted && !m_board.IsDragging()
		&& m_overlay == Overlay::None && Window::GetState().focused) {
		hoveredCardId = hoveredHandCard(Cursor::Pos());
	}
	for (int32 slot = 0; slot < static_cast<int32>(hand.size()); ++slot) {
		if (dealing && slot >= m_dealtCardCount) continue;
		int32 cardId = hand[slot];
		if (cardId != hoveredCardId && m_board.ShouldDrawAsHand(cardId)) m_cards[cardId].Draw(m_cards[cardId].GetPos());
	}
	if (hoveredCardId >= 0) {
		auto [x, y] = m_cards[hoveredCardId].GetPos();
		m_cards[hoveredCardId].Draw({ x, y - Layout::HandHoverLift }, Layout::HandHoverScale);
	}
}

// 音とダメージは updateAttack() にまとめてある．ここでは見た目を動かす．
void Battle::drawAttack() const
{
	bool playerAttacking = m_phase == Phase::PlayerAttack;
	auto start = playerAttacking ? Layout::PlayerAttackStart : Layout::EnemyAttackStart;
	auto arcEnd = playerAttacking ? Layout::EnemyAttackArcTarget : Layout::PlayerAttackArcTarget;
	auto hitTarget = playerAttacking ? Layout::EnemyHitTarget : Layout::PlayerHitTarget;
	Vec2 position{ start };
	double scale = 1.0;
	double alpha = 1.0;
	switch (m_attack.phase) {
	case AttackPhase::Delay: break;
	case AttackPhase::Arc: {
		auto motion = Layout::ResolveAttackArcMotion(start, arcEnd, m_attack.elapsed / BattleCombat::ArcDuration);
		position = motion.position;
		scale = motion.scale;
		break;
	}
	case AttackPhase::Defense:
		position = Vec2{ arcEnd };
		scale = Layout::AttackArcArrivalScale;
		if (m_attack.elapsed < 0.2) {
			auto shield = playerAttacking ? Layout::EnemyDefenseIcon : Layout::PlayerDefenseIcon;
			m_effectTexture.scaled(0.4).draw(shield.x, shield.y);
		}
		break;
	case AttackPhase::Impact:
		position = Vec2{ arcEnd }.lerp(Vec2{ hitTarget }, m_attack.elapsed / BattleCombat::ImpactDuration);
		scale = Layout::AttackArcArrivalScale;
		alpha = 1.0 - m_attack.elapsed / BattleCombat::ImpactDuration;
		break;
	case AttackPhase::Hits:
	case AttackPhase::Complete:
		if (m_hitPosition.x >= 0) m_effectTexture.scaled(0.5).draw(m_hitPosition.x, m_hitPosition.y);
		return;
	}
	Float2 pivot{ static_cast<float>(position.x), static_cast<float>(position.y) };
	const Transformer2D transform{ Mat3x2::Scale(scale, pivot) };
	m_attackIcon.scaled(Layout::AttackIconScale).draw(position, ColorF{ 1.0, 1.0, 1.0, alpha });
	m_combatFont(U"{}"_fmt(m_attack.attack)).draw(position + Vec2{ Layout::CombatValueAnimationOffset },
		ColorF{ 0.0, 0.0, 0.0, alpha });
}

void Battle::draw() const
{
	if (m_overlay == Overlay::Deck) {
		m_backgroundTexture.scaled(0.5).draw();
		m_banner.draw(getData().money, getData().Layer, getData().leric);
		return;
	}
	// 報酬までぼかさないよう，戦闘画面をいったん別の描画先に出す．
	if (m_phase == Phase::Victory && m_phaseTimer.sF() >= 2.0) {
		{
			const ScopedRenderTarget2D target(m_sceneBuffer);
			drawScene();
		}
		// 報酬が出そろってから背景をぼかす．
		if (m_phaseTimer.sF() >= 2.0 + Layout::RewardFadeInDuration) {
			for (int pass = 0; pass < Layout::RewardBlurPassCount; ++pass) {
				Shader::GaussianBlur(m_sceneBuffer, m_blurBuffer, m_sceneBuffer, static_cast<BoxFilterSize>(Layout::RewardBlurFilterSize));
			}
		}
		m_sceneBuffer.draw();
	} else {
		drawScene();
	}
	if (m_phase == Phase::PlayerAttack || m_phase == Phase::EnemyAttack) drawAttack();
	if (m_phase == Phase::Victory) drawVictory();
	m_board.DrawInteractionOverlay();
	if (m_overlay == Overlay::Settings) drawSettings();
}

void Battle::drawScene() const
{
	auto player_position = Layout::PlayerPosition;
	auto player_hp_position = Layout::PlayerHpPosition;
	auto enemy_position = Layout::EnemyPosition;
	auto enemy_hp_position = Layout::EnemyHpPosition;
	auto draw_pile_position = Layout::DrawPilePosition;
	auto discard_pile_position = Layout::DiscardPilePosition;
	auto equal_button = Layout::EqualButtonBounds;
	double enemy_display_scale = m_enemyBaseScale * m_enemyHitScale;
	// 攻撃中の数字は別の場所で動かすので，ここでは必要なものだけ残す．
	bool showAttack = m_phase == Phase::Planning;
	bool showDefense = showAttack || m_phase == Phase::PlayerAttack
		|| m_phase == Phase::EnemyAttack || m_phase == Phase::Discarding;
	auto attackPulse = Layout::ResolveCombatStatPulse(
		m_playerAttackPulseTime,
		showAttack);
	auto defensePulse = Layout::ResolveCombatStatPulse(
		m_playerDefensePulseTime,
		showAttack);
	auto drawStat = [this](const Texture& icon, double icon_scale,
		int32 value, Point icon_position,
		Point value_position,
		Layout::CombatStatPulse pulse) {
		const Float2 pivot{
			static_cast<float>((icon_position.x + value_position.x) / 2.0),
			static_cast<float>((icon_position.y + value_position.y
				+ Layout::CombatFontSize) / 2.0),
		};
		const Transformer2D transform{
			Mat3x2::Scale(pulse.scale, pivot).translated(0.0, -pulse.lift) };
		icon.scaled(icon_scale).draw(icon_position.x, icon_position.y);
		m_combatFont(U"{}"_fmt(value)).draw(
			value_position.x, value_position.y, Palette::Black);
	};
	m_backgroundTexture.scaled(0.5).draw();
	m_board.DrawBoard(0, showAttack);
	drawHand();
	m_playerTexture.scaled(Layout::PlayerDisplayScale).rotated(m_playerAngle)
		.draw(player_position.x, player_position.y);
	m_enemy.texture.scaled(enemy_display_scale).drawAt(enemy_position.x, enemy_position.y,
		ColorF(1.0, 1.0, 1.0, m_enemyAlpha));
	m_drawPileTexture.scaled(0.75).rotated(m_drawPileAngle).draw(draw_pile_position.x, draw_pile_position.y);
	m_discardPileTexture.scaled(0.6).rotated(m_discardPileAngle).draw(discard_pile_position.x, discard_pile_position.y);
	m_attackButtonTexture.scaled(0.7).draw(equal_button.x, equal_button.y);
	m_playerHpBar.draw(RectF{ player_hp_position.x, player_hp_position.y, 240, 15 });
	m_enemyHpBar.draw(RectF{ enemy_hp_position.x, enemy_hp_position.y, 320, 20 });
	if (showAttack || m_phase == Phase::PlayerAttack) {
		drawStat(m_attackIcon, Layout::AttackIconScale,
			m_enemyDisplay.attack, Layout::EnemyAttackIcon,
			Layout::EnemyAttackValue, {});
	}
	if (showDefense) {
		drawStat(m_defenseIcon, Layout::DefenseIconScale,
			m_enemyDisplay.defense, Layout::EnemyDefenseIcon,
			Layout::EnemyDefenseValue, {});
	}
	if (showAttack) {
		drawStat(m_attackIcon, Layout::AttackIconScale,
			m_playerDisplay.attack, Layout::PlayerAttackIcon,
			Layout::PlayerAttackValue, attackPulse);
	}
	if (showDefense) {
		drawStat(m_defenseIcon, Layout::DefenseIconScale,
			m_playerDisplay.defense, Layout::PlayerDefenseIcon,
			Layout::PlayerDefenseValue, defensePulse);
	}
	m_banner.draw(getData().money, getData().Layer, getData().leric);
}

void Battle::drawVictory() const
{
	if (m_phaseTimer.sF() >= 2.0){
		double reward_alpha = Min(1.0, Max(0.0,
			(m_phaseTimer.sF() - 2.0) / Layout::RewardFadeInDuration));
		const Rect rect{ Arg::center = Scene::Center(), 600, 400 };
		const RectF textRect = m_rewardFont(U"報酬").region(Arg::center = rect.topCenter());
		const ColorF panel_color{ 1.0, reward_alpha };
		const ColorF frame_color{ 0.0, reward_alpha };
		const ColorF title_color{ 1.0, reward_alpha };
		const ColorF reward_color{ 1.0, 1.0, 1.0, reward_alpha };

		rect.draw(panel_color);
		rect.drawFrame(5.0, frame_color);
		textRect.stretched(10, 0).draw(title_color);

		m_rewardFont(U"報酬").draw(Arg::center = rect.topCenter(), ColorF{ 0.0, 0.0, 0.0, reward_alpha });
		m_rewardTexture.scaled(1.0).draw(Arg::center = rect.center()+Vec2{0.0, 80}, reward_color);
		m_rewardFont(U"+{}"_fmt(m_reward)).draw(
			Arg::center = rect.center() + Vec2{0, -20}, reward_color);
	}
	return;
}

void Battle::drawSettings() const
{
	// デッキ一覧と同じく，上部のバナー領域を残して背景を描画する．
	m_settingsBackgroundTexture(Rect{ 0, 150, Scene::Width(), Scene::Height() - 150 })
		.draw(0, 150);

	auto draw_slider = [this](const StringView label, const RectF& track,
		double volume) {
		m_numFont(label).drawAt(
			track.x - 100,
			track.y + track.h / 2.0,
			Palette::Black
		);
		track.rounded(10).draw(ColorF{ 0.62 });
		RectF{ track.x, track.y, track.w * GameStateRules::ClampVolume(volume), track.h }
			.rounded(10).draw(ColorF{ 0.25, 0.52, 0.82 });
		Circle{ track.x + track.w * GameStateRules::ClampVolume(volume),
			track.y + track.h / 2.0, 24 }.draw(Palette::White).drawFrame(4, ColorF{ 0.25 });
		m_numFont(U"{}%"_fmt(GameStateRules::VolumePercent(volume)))
			.draw(track.x + track.w + 50, track.y - 40, Palette::Black);
	};
	draw_slider(U"BGM", BgmSliderTrack, getData().audio_settings.bgm_volume);
	draw_slider(U"SE", SeSliderTrack, getData().audio_settings.se_volume);

	const bool close_hovered = SettingsCloseButton.contains(Cursor::Pos());
	double button_scale = close_hovered ? 0.95 : 1.0;
	m_settingsBackButtonTexture.scaled(0.75 * button_scale).draw(
		SettingsCloseButton.x, SettingsCloseButton.y,
		close_hovered ? ColorF{ 0.75 } : ColorF{ 1.0 });
}
