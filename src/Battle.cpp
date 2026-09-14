#include "Battle.hpp"
#include "Board.hpp" // BoardクラスのConfirm()などを使うためにインクルード
#include <algorithm>
#include <cassert>
#include <numeric>
#include <tuple> // tie関数を使用するためにインクルード
using std::tie; // std::tieを使用するために名前空間を指定

namespace {

const RectF SettingsPanel{ 560, 245, 800, 560 }; // 設定パネルの表示範囲を定義する．
const RectF BgmSliderTrack{ 780, 420, 420, 12 }; // BGM音量スライダーの範囲を定義する．
const RectF SeSliderTrack{ 780, 560, 420, 12 }; // 効果音量スライダーの範囲を定義する．
const RectF SettingsCloseButton{ 850, 690, 220, 70 }; // 設定を閉じるボタンの範囲を定義する．

}

// Constructor
Battle::Battle(const InitData& init)
    : IScene(init),
    is_board_locked(true),
    now_turn(-1), // ターン数を初期化// 手札のサイズを取得
    deck_size((int32)getData().Deck.size()), // グローバルのDeckのサイズを取得
	m_cards(getData().Deck),
    m_currentAnimState(BattleAnimationState::CardDrawEffect), // アニメーション状態を初期化
    table_id(0)
{
    m_backgroundTexture = Texture(U"../../image/haikei_sentou.png"); // 背景画像のパスを指定
    m_myTexture = Texture(U"../../image/chara_player.png"); // 自分のカードのテクスチャ
    m_yamahudaTexture = Texture(U"../../image/yamahuda.png"); // 山札のテクスチャ
    m_sutehudaTexture = Texture(U"../../image/sutehuda.png"); // 捨て札のテクスチャ
    m_buttonTexture = Texture(U"../../image/bottun_equal.png"); // ボタンのテクスチャ
    m_effectTexture = Texture(U"../../image/effect_attack.png"); // エフェクトのテクスチャ
    m_attackIcon = Texture(U"../../image/icon_attack.png"); // 攻撃アイコンのテクスチャ
    m_defenceIcon = Texture(U"../../image/icon_seild.png"); // 防御アイコンのテクスチャ
    m_reward_money = Texture(U"../../image/UI_money.png"); // 報酬のテクスチャ
    m_rewardFont = Font{ 50, Typeface::Bold };
    m_numFont = Font{ 48, Typeface::Bold };
	m_combatFont = Font{ BattleLayoutRules::CombatFontSize, Typeface::Bold };
    m_combatSceneBuffer = RenderTexture(Scene::Size());
    m_blurInternalBuffer = RenderTexture(Scene::Size());

    // init
    m_board.BeginBattle(getData().board_progress);
    table_max_size = getTableSize();
    setupEnemy(getData().enemy, GameStateRules::ActIndex(getData().Layer));
    my_hpbar = HPBar{ getData().MaxHP, getData().HP }; // 自分のHPバーの初期化
    action_cycle = static_cast<int32>(m_enemy.actionPattern.size()); // 敵の行動パターンのサイクルを設定
    reward_money = m_enemy.type == 0 ? 20 : m_enemy.type == 1 ? 40 : 100; // 報酬の金額を設定
    // --- デッキの初期化 ---
    const bool preserve_deck_order = getData().debug_battle_overrides
        && getData().debug_battle_overrides->preserve_deck_order;
    const auto initial_draw_order = GameStateRules::CreateInitialDrawOrder(
        deck_size, preserve_deck_order);
    Array<int32> draw_order{ initial_draw_order.begin(), initial_draw_order.end() };
    for (int32 i = 0; i < deck_size; i++) {
        m_cards[i].ResetRuntimeState();
    }
    if (GameStateRules::ShouldShuffleInitialDrawOrder(preserve_deck_order)) {
        draw_order.shuffle();
    }
    m_deckState.Initialize(deck_size,
        std::vector<int32>{ draw_order.begin(), draw_order.end() });

    ApplyAudioSettings();
    //音楽再生！
    battle_bgm.play(); // 音楽を再生
}


void Battle::setupEnemy(int32 type, int32 layer)
{
	// Enemyの静的定義をBattleの実行状態へ転写する．
    const EnemyData& data = m_enemyDB.getOneEnemy(type, layer);
    m_enemy.name = data.name;
	const String texture_path = getData().debug_battle_overrides
		&& !getData().debug_battle_overrides->enemy_texture_path.isEmpty()
		? getData().debug_battle_overrides->enemy_texture_path : data.texturePath;
    m_enemy.texture = Texture(texture_path);
	enemy_base_scale = BattleLayoutRules::EnemyBaseScale(m_enemy.texture.height());
	enemy_scale_multiplier = 1.0;
    m_enemy.type = data.type;
    m_enemy.maxHp = data.maxHp;
    m_enemy.hp = data.maxHp;
    m_enemy.actionPattern = data.actionPattern;
    ene_hpbar = HPBar{m_enemy.maxHp, m_enemy.hp};
}

// 山札に配置できる最大枚数を盤面の情報から求める関数
int32 Battle::getTableSize() const
{
	const int32 override_limit = getData().debug_battle_overrides
		? getData().debug_battle_overrides->hand_limit : 0;
    return GameStateRules::ResolveBattleHandLimit(getData().board_progress, override_limit);
}

Point Battle::GetHandPosition(const int32 slot) const
{
	const auto& hand = Cards(GameStateRules::CardZone::Hand);
	int32 preceding_width_cells = 0;
	for (int32 index = 0; index < slot; ++index) {
		preceding_width_cells += m_cards.at(hand.at(index)).Size().first;
	}
	const int32 card_width_cells = m_cards.at(hand.at(slot)).Size().first;
	const auto position = BattleLayoutRules::HandPosition(
		slot, preceding_width_cells, card_width_cells);
	return { position.x, position.y };
}

Rect Battle::GetAttackButtonRect() const
{
	const auto bounds = BattleLayoutRules::EqualButtonBounds();
	return { bounds.x, bounds.y, bounds.width, bounds.height };
}

// 0:山札, 1:手札, 2:盤面, -1:捨て札
// 盤面と山札のデッキの状況をリアルタイムで監視する関数
void Battle::updateTableDeck()
{
    ApplyBoardZoneChanges();
    AssertCardOwnership("updateTableDeck");
}

void Battle::PrepareDrawPileForTurn()
{
	// 盤面の占有状況を基準に，GameStateRulesへ山札補充の要否を委譲する．
	const auto occupied_cells = [this](const std::vector<int32>& cards) {
		int32 total = 0;
		for (const int32 card_id : cards) {
			if ((0 <= card_id) && (card_id < static_cast<int32>(m_cards.size()))) {
				total += m_cards[card_id].OccupiedCellCount();
			}
		}
		return total;
	};
	const int32 remaining_cells = occupied_cells(Cards(GameStateRules::CardZone::DrawPile));
	std::vector<int32> all_cards(static_cast<std::size_t>(Max(deck_size, 0)));
	std::iota(all_cards.begin(), all_cards.end(), 0);
	const int32 full_deck_cells = occupied_cells(all_cards);
	if (!GameStateRules::ShouldRefreshDrawPile(remaining_cells, full_deck_cells,
		getData().board_progress.UnlockedCount())) return;

	const bool preserve_deck_order = getData().debug_battle_overrides
		&& getData().debug_battle_overrides->preserve_deck_order;
	const auto initial_order = GameStateRules::CreateInitialDrawOrder(deck_size, preserve_deck_order);
	Array<int32> draw_order{ initial_order.begin(), initial_order.end() };
	if (GameStateRules::ShouldShuffleInitialDrawOrder(preserve_deck_order)) draw_order.shuffle();
	const bool refreshed = m_deckState.RefreshDrawPile(
		std::vector<int32>{ draw_order.begin(), draw_order.end() });
#ifndef NDEBUG
	if (!refreshed) assert(false && "Draw pile refresh failed at a turn boundary");
#endif
	if (!refreshed) return;
	for (const int32 card_id : Cards(GameStateRules::CardZone::DrawPile)) {
		m_cards[card_id].SetStat(0);
	}
	AssertCardOwnership("PrepareDrawPileForTurn");
}

void Battle::ApplyAudioSettings() const
{
	const auto& settings = getData().audio_settings;
	battle_bgm.setVolume(GameStateRules::ClampVolume(settings.bgm_volume));
	draw_card_se.setVolume(GameStateRules::ClampVolume(settings.se_volume));
	drag_card_se.setVolume(GameStateRules::ClampVolume(settings.se_volume));
	attack_se.setVolume(GameStateRules::ClampVolume(settings.se_volume));
}

void Battle::updateSettingsOverlay(const BoardInputFrame& input)
{
	if (!input.focused) m_activeVolumeSlider = VolumeSlider::None;
	if (KeyEscape.down() || (input.left_down && SettingsCloseButton.contains(input.cursor))) {
		is_settings_open = false;
		m_activeVolumeSlider = VolumeSlider::None;
		m_animeStopwatch.resume();
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
		const double volume = GameStateRules::SliderVolumeAt(input.cursor.x, track.x, track.w);
		if (m_activeVolumeSlider == VolumeSlider::Bgm) settings.bgm_volume = volume;
		else settings.se_volume = volume;
		ApplyAudioSettings();
	}
	if (input.left_up) m_activeVolumeSlider = VolumeSlider::None;

	const bool interactive = SettingsCloseButton.contains(input.cursor)
		|| BgmSliderTrack.stretched(20).contains(input.cursor)
		|| SeSliderTrack.stretched(20).contains(input.cursor);
	if (interactive) Cursor::RequestStyle(CursorStyle::Hand);
}

const std::vector<int32>& Battle::Cards(const GameStateRules::CardZone zone) const
{
    return m_deckState.Cards(zone);
}

bool Battle::MoveCard(const int32 card_id, const GameStateRules::CardZone expected,
    const GameStateRules::CardZone destination)
{
	// DeckStateの整合性を保ちながら，Boardを含むカード領域を遷移させる．
	if (!m_deckState.CanMove(card_id, expected, destination)) return false;
	if (GameStateRules::NeedsBoardDetach(expected, destination)
		&& !m_board.DetachCard(card_id)) return false;
	const bool moved = m_deckState.Move(card_id, expected, destination);
#ifndef NDEBUG
	if (!moved) assert(false && "CardZone transition failed after successful preflight");
#else
	(void)moved;
#endif
	if (!moved) return false;
    const int32 stat = (destination == GameStateRules::CardZone::DrawPile) ? 0
        : (destination == GameStateRules::CardZone::Hand) ? 1
        : (destination == GameStateRules::CardZone::Board) ? 2 : -1;
    m_cards[card_id].SetStat(stat);
    return true;
}

bool Battle::ApplyBoardZoneChanges()
{
	// Boardの通知をBattleDeckStateへ反映し，カード表示状態も同期する．
	const auto changes = m_board.ConsumeZoneChanges();
	std::vector<bool> changed(static_cast<size_t>(Max(deck_size, 0)), false);
	for (const auto& change : changes) {
		if (!m_deckState.IsValidCard(change.card_id)
			|| changed[static_cast<size_t>(change.card_id)]
			|| (m_deckState.ZoneOf(change.card_id) != change.expected)) {
#ifndef NDEBUG
			Logger << U"Invalid Board CardZoneChange: card=" << change.card_id
				<< U", expected=" << static_cast<int32>(change.expected)
				<< U", actual=" << static_cast<int32>(m_deckState.ZoneOf(change.card_id))
				<< U", destination=" << static_cast<int32>(change.destination);
			assert(false && "Invalid Board CardZoneChange");
#endif
			return false;
		}
		changed[static_cast<size_t>(change.card_id)] = true;
	}
	for (const auto& change : changes) {
		if (!MoveCard(change.card_id, change.expected, change.destination)) return false;
	}
	return true;
}

void Battle::AssertCardOwnership(const char* context) const
{
	// DeckState，Blockの状態，Boardのライフサイクルが一致することを確認する．
#ifndef NDEBUG
    String diagnostic;
    const auto visit = [&](const std::vector<int32>& cards, const int32 expected_stat, const StringView area) {
        for (const int32 deck_index : cards) {
            if ((deck_index < 0) || (deck_size <= deck_index)) {
                diagnostic = U"invalid deck index in " + String{ area } + U": " + Format(deck_index);
                return false;
            }
            if (m_cards[deck_index].GetStat() != expected_stat) {
                diagnostic = U"card stat mismatch in " + String{ area } + U": deck_index=" + Format(deck_index)
                    + U", stat=" + Format(m_cards[deck_index].GetStat());
                return false;
            }
        }
        return true;
    };
    const bool valid_areas = visit(Cards(GameStateRules::CardZone::DrawPile), 0, U"deck")
        && visit(Cards(GameStateRules::CardZone::Hand), 1, U"hand")
        && visit(Cards(GameStateRules::CardZone::Board), 2, U"board")
        && visit(Cards(GameStateRules::CardZone::Discard), -1, U"discard");
    if (valid_areas && !m_deckState.Validate()) diagnostic = U"battle deck zone invariant failed";
	if (!diagnostic.isEmpty()) {
		Logger << U"Battle card invariant violation (" << Unicode::Widen(context) << U"): " << diagnostic
			<< U", frame=" << m_frameNumber << U", pointer_owner=" << static_cast<int32>(m_pointerInputOwner);
		Logger << U"Deck_table=" << Format(Cards(GameStateRules::CardZone::Hand))
			<< U", Deck_board=" << Format(Cards(GameStateRules::CardZone::Board))
			<< U", Deck_yama=" << Format(Cards(GameStateRules::CardZone::DrawPile))
			<< U", Deck_gomi=" << Format(Cards(GameStateRules::CardZone::Discard));
		assert(false && "Battle card invariant violation; see Logger output");
    }
#else
    (void)context;
#endif
}

void Battle::getEnemyInfo()
{
	// Enemyの行動パターンをBattleの敵意図状態へ変換する．
	if ((m_currentAnimState != BattleAnimationState::Idle) || m_enemy.actionPattern.isEmpty()) return;
	if (!m_enemyIntentState.has_action) {
		const int32 pattern_size = static_cast<int32>(m_enemy.actionPattern.size());
		const int64 raw_index = static_cast<int64>(now_turn % Max(action_cycle, 1)) + turn_start;
		const int32 action_index = static_cast<int32>(((raw_index % pattern_size) + pattern_size) % pattern_size);
		const EnemyAction& action = m_enemy.actionPattern[action_index];
		EnemyIntentRules::PrepareAction(m_enemyIntentState, action.attack, action.defense);
	}
	const auto intent = EnemyIntentRules::ResolveFrame(m_enemyIntentState.side_effects_prepared, {
		m_enemyIntentState.raw_attack,
		m_enemyIntentState.raw_defense,
		table_max_size,
		static_cast<int32>(Cards(GameStateRules::CardZone::Hand).size()),
	});
	ene_attack = intent.attack;
	ene_defense = intent.defense;
	is_exit = intent.exit_requested;
	is_boss3 = intent.cancels_player_damage;
	getData().money += intent.money_delta;
	if (intent.turn_advance != 0) {
		now_turn += intent.turn_advance;
		turn_start = now_turn;
	}
	if (intent.action_cycle_override != 0) action_cycle = intent.action_cycle_override;
}

// 「=」ボタンが押された時に呼び出される
void Battle::attack()
{
	// Boardの確定値とBattleDamageRulesの結果を戦闘演出状態へ渡す．
    // 現在アニメーション中でない場合のみ処理を開始
    if (m_currentAnimState == BattleAnimationState::Idle) {
        // 盤面の操作をロックする
        is_board_locked = true; // 盤面の操作をロック
        // プレイヤー->敵の攻撃力を計算
		const int32 player_damage = Max(0, my_attack - ene_defense);
		my_real_attack = is_boss3 ? 0 : Min(player_damage, m_enemy.hp);
		// 敵->プレイヤーの攻撃力を計算
		ene_real_attack = Min(Max(0, ene_attack - my_defense), getData().HP);
        // 自分・敵の防御力を減らすエフェクトのための変数を設定
        ene_defense_effect = ene_defense; // 敵の防御力を減らすエフェクトのための変数
		my_defense_effect = my_defense; // 自分の防御力を減
		my_attack_effect = my_attack;
		ene_attack_effect = ene_attack;
		// damage-effectの演出のための制御変数を設定
		m_enemyDamageHits = BattleDamageRules::SplitDamage(my_real_attack, m_enemy.maxHp);
		m_playerDamageHits = BattleDamageRules::SplitDamage(ene_real_attack, getData().MaxHP);
		ene_damage_effect_cnt = 0;
		my_damage_effect_cnt = 0;
		ene_effect_x = ene_effect_y = -1;
		my_effect_x = my_effect_y = -1;
        // attack/defecce の演出のための変数を設定
		const auto player_attack_start = BattleLayoutRules::PlayerAttackStart();
		const auto enemy_attack_arc_target = BattleLayoutRules::EnemyAttackArcTarget();
		const auto enemy_attack_start = BattleLayoutRules::EnemyAttackStart();
		const auto player_attack_arc_target = BattleLayoutRules::PlayerAttackArcTarget();
        my_attack_icon_start = Vec2{ player_attack_start.x, player_attack_start.y };
        my_attack_icon_end = Vec2{ enemy_attack_arc_target.x, enemy_attack_arc_target.y };
        ene_attack_icon_start = Vec2{ enemy_attack_start.x, enemy_attack_start.y };
        ene_attack_icon_end = Vec2{ player_attack_arc_target.x, player_attack_arc_target.y };
        my_attack_icon_pos = my_attack_icon_start; // 自分の攻撃アイコンの位置を初期化
        ene_attack_icon_pos = ene_attack_icon_start; // 敵の攻撃アイコン
		my_attack_icon_scale = 1.0;
		ene_attack_icon_scale = 1.0;
        my_attack_type = -1;
        ene_attack_type = 0;
        flag_once_draw = 0;
        //
        m_currentAnimState = BattleAnimationState::CombatEnemyEffect;
        m_animeStopwatch.restart(); // ストップウォッチをリセットして開始
    }
}

// 戦闘演出の更新処理
void Battle::updateCombatEnemyEffect()
{
    // 遅延のため
    if (my_attack_type == -1 && m_animeStopwatch.sF() < 0.4) {//攻撃演出開始前に0.4秒待つ．
        return;
    }
    if (my_attack_type == -1){
        my_attack_type = 0;
        m_animeStopwatch.restart();
    }
    // 自分の攻撃アイコンを敵の左上へ弧を描いて移動させる演出
    if (my_attack_type == 0
		&& m_animeStopwatch.sF() < BattleLayoutRules::AttackArcTravelDuration) {
		const auto motion = BattleLayoutRules::ResolveAttackArcMotion(
			BattleLayoutRules::PlayerAttackStart(),
			BattleLayoutRules::EnemyAttackArcTarget(), m_animeStopwatch.sF());
		my_attack_icon_pos = Vec2{ motion.x, motion.y };
		my_attack_icon_scale = motion.scale;
        return;
    }
    if (my_attack_type == 0) {
        my_attack_icon_pos = my_attack_icon_end; // エフェクトの位置を最終位置に設定
		my_attack_icon_scale = BattleLayoutRules::AttackArcArrivalScale;
        my_attack_icon_start = my_attack_icon_end;
		const auto enemy_hit = BattleLayoutRules::EnemyHitTarget();
        my_attack_icon_end = Vec2{ enemy_hit.x, enemy_hit.y };
		const auto exchange = BattleDamageRules::ResolveDefenseExchange(
			my_attack_effect, ene_defense_effect, my_real_attack);
		if (!exchange.has_contact) {
			ene_defense = exchange.defense_after;
			my_attack = exchange.attack_after;
			my_attack_type = (my_attack == 0 ? 3 : 2);
			m_animeStopwatch.restart();
			return;
		}
		my_attack_type = 1;
        m_animeStopwatch.restart(); // ストップウォッチをリセット
        return;
    }
	const double enemy_defense_exchange_duration =
		BattleDamageRules::ResolveDefenseExchangeDuration(
			my_attack_effect, ene_defense_effect, m_enemy.maxHp);
    // 敵の防御を減らす演出
    if (my_attack_type == 1
		&& m_animeStopwatch.sF() < enemy_defense_exchange_duration) {
        flag_once_draw++;
		const auto exchange = BattleDamageRules::ResolveDefenseExchange(
			my_attack_effect, ene_defense_effect, my_real_attack);
		my_attack = BattleLayoutRules::ResolveCombatValueChange(
			my_attack_effect, exchange.attack_after, m_animeStopwatch.sF(),
			enemy_defense_exchange_duration);
		ene_defense = BattleLayoutRules::ResolveCombatValueChange(
			ene_defense_effect, exchange.defense_after, m_animeStopwatch.sF(),
			enemy_defense_exchange_duration);
        return;
    }
    if (my_attack_type == 1){
		const auto exchange = BattleDamageRules::ResolveDefenseExchange(
			my_attack_effect, ene_defense_effect, my_real_attack);
		ene_defense = exchange.defense_after;
		my_attack = exchange.attack_after;
        my_attack_type = (my_attack == 0 ? 3 : 2);
        m_animeStopwatch.restart(); // ストップウォッチをリセット
        return;
    }
    // 自分の攻撃アイコン->敵の画像へ移動させる演出
    if (my_attack_type == 2
		&& m_animeStopwatch.sF() < BattleLayoutRules::BodyAttackTravelDuration) {
		my_attack_icon_pos = Math::Lerp(my_attack_icon_start, my_attack_icon_end,
			m_animeStopwatch.sF() / BattleLayoutRules::BodyAttackTravelDuration);
        return;
    }
    if (my_attack_type == 2){
        my_attack_icon_pos = my_attack_icon_end; // エフェクトの位置を最終位置に設定
        my_attack_type = 3;
        m_animeStopwatch.restart(); // ストップウォッチをリセット
        return;
    }
    enemy_scale_multiplier = Min(1.0,
        enemy_scale_multiplier + Scene::DeltaTime() * BattleLayoutRules::EnemyScaleRecoveryRate);
    // damage_effectを表示するための制御
	if (my_attack_type == 3
		&& ene_damage_effect_cnt < static_cast<int32>(m_enemyDamageHits.size())) {
		if (BattleLayoutRules::ShouldTriggerDamageEffect(
			m_animeStopwatch.sF(), ene_damage_effect_cnt)) {
			const int32 hit_damage = m_enemyDamageHits[ene_damage_effect_cnt];
			m_enemy.hp = BattleDamageRules::ApplyHit(m_enemy.hp, hit_damage);
			ene_hpbar.damage(hit_damage);
			const auto effect_bounds = BattleLayoutRules::EnemyDamageEffectBounds();
			ene_effect_x = Random(effect_bounds.x, effect_bounds.x + effect_bounds.width);
			ene_effect_y = Random(effect_bounds.y, effect_bounds.y + effect_bounds.height);
			ene_damage_effect_cnt++;
            enemy_scale_multiplier = BattleLayoutRules::EnemyHitScaleMultiplier;
			// SE再生
			attack_se.playOneShot(GameStateRules::ClampVolume(getData().audio_settings.se_volume));
			if (!BattleDamageRules::ShouldContinueHits(m_enemy.hp)) {
				ene_damage_effect_cnt = static_cast<int32>(m_enemyDamageHits.size());
				m_currentAnimState = BattleAnimationState::WinEffect;
				m_animeStopwatch.restart();
				return;
			}
		}
		return;
    }
    // 敵を倒したかの判定
    bool isWin = m_enemy.hp <= 0;
    if (isWin) {
        m_currentAnimState = BattleAnimationState::WinEffect;
        m_animeStopwatch.restart();
        return;
    }
    flag_once_draw = 0;
    my_damage_effect_cnt = 0;
    enemy_scale_multiplier = 1.0;
    m_currentAnimState = BattleAnimationState::CombatMyEffect;
    m_animeStopwatch.restart();
}

// 戦闘演出の更新処理
void Battle::updateCombatMyEffect()
{
    if (ene_attack_type == 0
		&& m_animeStopwatch.sF() < BattleLayoutRules::AttackArcTravelDuration) {
		const auto motion = BattleLayoutRules::ResolveAttackArcMotion(
			BattleLayoutRules::EnemyAttackStart(),
			BattleLayoutRules::PlayerAttackArcTarget(), m_animeStopwatch.sF());
		ene_attack_icon_pos = Vec2{ motion.x, motion.y };
		ene_attack_icon_scale = motion.scale;
        return;
    }
    if (ene_attack_type == 0) {
        ene_attack_icon_pos = ene_attack_icon_end; // エフェクトの位置を最終位置に設定
		ene_attack_icon_scale = BattleLayoutRules::AttackArcArrivalScale;
        ene_attack_icon_start = ene_attack_icon_end;
		const auto player_hit = BattleLayoutRules::PlayerHitTarget();
        ene_attack_icon_end = Vec2{ player_hit.x, player_hit.y };
		const auto exchange = BattleDamageRules::ResolveDefenseExchange(
			ene_attack_effect, my_defense_effect, ene_real_attack);
		if (!exchange.has_contact) {
			my_defense = exchange.defense_after;
			ene_attack = exchange.attack_after;
			ene_attack_type = (ene_attack == 0 ? 3 : 2);
			m_animeStopwatch.restart();
			return;
		}
		ene_attack_type = 1;
        m_animeStopwatch.restart(); // ストップウォッチをリセット
        return;
    }
	const double player_defense_exchange_duration =
		BattleDamageRules::ResolveDefenseExchangeDuration(
			ene_attack_effect, my_defense_effect, getData().MaxHP);
    if (ene_attack_type == 1
		&& m_animeStopwatch.sF() < player_defense_exchange_duration) {
        flag_once_draw++;
		const auto exchange = BattleDamageRules::ResolveDefenseExchange(
			ene_attack_effect, my_defense_effect, ene_real_attack);
		ene_attack = BattleLayoutRules::ResolveCombatValueChange(
			ene_attack_effect, exchange.attack_after, m_animeStopwatch.sF(),
			player_defense_exchange_duration);
		my_defense = BattleLayoutRules::ResolveCombatValueChange(
			my_defense_effect, exchange.defense_after, m_animeStopwatch.sF(),
			player_defense_exchange_duration);
        return;
    }
    if (ene_attack_type == 1){
		const auto exchange = BattleDamageRules::ResolveDefenseExchange(
			ene_attack_effect, my_defense_effect, ene_real_attack);
		my_defense = exchange.defense_after;
		ene_attack = exchange.attack_after;
        ene_attack_type = (ene_attack == 0 ? 3 : 2);
        m_animeStopwatch.restart(); // ストップウォッチをリセット
        return;
    }
    if (ene_attack_type == 2
		&& m_animeStopwatch.sF() < BattleLayoutRules::BodyAttackTravelDuration) {
		ene_attack_icon_pos = Math::Lerp(ene_attack_icon_start, ene_attack_icon_end,
			m_animeStopwatch.sF() / BattleLayoutRules::BodyAttackTravelDuration);
        return;
    }
    if (ene_attack_type == 2){
        ene_attack_icon_pos = ene_attack_icon_end; // エフェクトの位置を最終位置に設定
        ene_attack_type = 3;
        m_animeStopwatch.restart(); // ストップウォッチをリセット
        return;
    }
    my_angle = Min(0.0, my_angle + Scene::DeltaTime());
    // damage_effectを表示するための制御
	if (ene_attack_type == 3
		&& my_damage_effect_cnt < static_cast<int32>(m_playerDamageHits.size())) {
		if (BattleLayoutRules::ShouldTriggerDamageEffect(
			m_animeStopwatch.sF(), my_damage_effect_cnt)) {
			const int32 hit_damage = m_playerDamageHits[my_damage_effect_cnt];
			getData().HP = BattleDamageRules::ApplyHit(getData().HP, hit_damage);
			my_hpbar.damage(hit_damage);
			my_effect_x = Random(50, 200); // エフェクトのX座標をランダムに設定
            my_effect_y = Random(130, 230); // エフェクトのY座標をランダムに設定
            my_damage_effect_cnt++;
            my_angle = Random(-0.52, -0.1); // -π/4 ~ -π/6の範囲でプレイヤーを傾かさせる
			// SE再生
			attack_se.playOneShot(GameStateRules::ClampVolume(getData().audio_settings.se_volume));
			if (!BattleDamageRules::ShouldContinueHits(getData().HP)) {
				my_damage_effect_cnt = static_cast<int32>(m_playerDamageHits.size());
				m_currentAnimState = BattleAnimationState::GameOver;
				m_animeStopwatch.restart();
				return;
			}
		}
        return;
    }
    // 勝利判定を行う
    bool isVictory = getData().HP <= 0;
    if (isVictory) {
        m_currentAnimState = BattleAnimationState::GameOver;
        m_animeStopwatch.restart();
        return;
    }
    my_angle = 0.0;
	m_discardCardMotions.clear();
	m_discardCardMotionsInitialized = false;
    m_currentAnimState = BattleAnimationState::DiscardEffect;
    m_animeStopwatch.restart();
}

// 捨て札アニメーション(盤面, 手札 -> 捨て札)の更新処理
void Battle::updateDiscardEffect()
{
	const auto& hand = Cards(GameStateRules::CardZone::Hand);
    if (!m_discardCardMotionsInitialized && !hand.empty() && (sutehuda_angle > -90_deg)) {
		sutehuda_angle -= Scene::DeltaTime() * 4.0;//捨て札の回転速度を4.0として進める．
		m_animeStopwatch.restart();
		return;
	}
	if (!m_discardCardMotionsInitialized) {
		if (!hand.empty()) sutehuda_angle = -90_deg;
		m_discardCardMotions.reserve(hand.size());
		for (const int32 card_id : hand) {
			if ((m_cards[card_id].GetStat() != 1) || !m_board.ShouldDrawAsHand(card_id)) continue;
			const auto hand_position = m_cards[card_id].GetPos();
			m_discardCardMotions.push_back({
				card_id,
				Vec2{ hand_position.first, hand_position.second },
			});
		}
		std::stable_sort(m_discardCardMotions.begin(), m_discardCardMotions.end(),
			[](const DiscardCardMotion& left, const DiscardCardMotion& right) {
				return left.start.x > right.start.x;
			});
		m_discardCardMotionsInitialized = true;
		m_animeStopwatch.restart();
	}

	bool collection_complete = true;
	const double elapsed_seconds = m_animeStopwatch.sF();
	const int32 card_count = static_cast<int32>(m_discardCardMotions.size());
	const auto discard_target = BattleLayoutRules::DiscardTarget();
	const Vec2 target{ discard_target.x, discard_target.y };
	for (int32 card_index = 0; card_index < card_count; ++card_index) {
		DiscardCardMotion& motion = m_discardCardMotions[card_index];
		if (motion.complete) continue;
		const auto progress = BattleCardRules::ResolveDiscardCardProgress(
			card_index, card_count, elapsed_seconds);
		if (!progress.started) {
			collection_complete = false;
			continue;
		}
		if (!motion.detached) {
			if (!m_board.DetachCard(motion.card_id)) {
#ifndef NDEBUG
				assert(false && "Failed to detach hand card before discard animation");
#endif
				return;
			}
			motion.detached = true;
		}
		const Vec2 pos = motion.start.lerp(target, progress.linear_progress);
		m_cards[motion.card_id].SetPos(pos.x, pos.y);
		if (!progress.complete) {
			collection_complete = false;
			continue;
		}
		if (!MoveCard(motion.card_id, GameStateRules::CardZone::Hand,
			GameStateRules::CardZone::Discard)) {
#ifndef NDEBUG
			assert(false && "Failed to move hand card to discard after animation");
#endif
			return;
		}
		motion.complete = true;
	}
	if (!collection_complete) return;

    if (sutehuda_angle < 0.01) {
        sutehuda_angle += Scene::DeltaTime() * 7.0; // 捨て札の回転速度を7.0として戻す．
        return;
    }
    m_board.EndTurn();
    const auto board_cards = Cards(GameStateRules::CardZone::Board);
    for (const int32 deck_index : board_cards) {
        MoveCard(deck_index, GameStateRules::CardZone::Board, GameStateRules::CardZone::Discard);
    }
    PrepareDrawPileForTurn();
    m_board.BeginTurn();
    sutehuda_angle = 0.0;
    table_id = 0;
	m_discardCardMotions.clear();
	m_discardCardMotionsInitialized = false;
	m_handDealComplete = false;
    m_currentAnimState = BattleAnimationState::CardDrawEffect;
    m_animeStopwatch.restart();
}

// カードドローアニメーションの更新処理
void Battle::updateCardDrawEffect()
{
    // 山札から手札へ移動する。
	const auto& draw_pile = Cards(GameStateRules::CardZone::DrawPile);
	const auto& hand = Cards(GameStateRules::CardZone::Hand);
    while (!draw_pile.empty() && static_cast<int32>(hand.size()) < table_max_size) {
        const int32 id = draw_pile.back();
        MoveCard(id, GameStateRules::CardZone::DrawPile, GameStateRules::CardZone::Hand);
		m_cards.at(id).SetPos(50, 900); // カードを配布アニメーションの開始位置へ置く．
    }

	const auto deal_stage = BattleCardRules::ResolveHandDealStage(
		!hand.empty(), m_handDealComplete,
		yamahuda_angle >= 90_deg, yamahuda_angle <= 0.01);
	if (deal_stage == BattleCardRules::HandDealStage::RotatePile) {
		table_id = 0;
		yamahuda_angle += Scene::DeltaTime() * 4.0;//山札の回転速度を4.0として進める．
		m_animeStopwatch.restart();
		return;
	}

	if (deal_stage == BattleCardRules::HandDealStage::DealCards) {
		yamahuda_angle = 90_deg;
		const double elapsed_seconds = m_animeStopwatch.sF();
		while (table_id < static_cast<int32>(hand.size())
			&& BattleCardRules::ResolveHandDealProgress(table_id, elapsed_seconds).started) {
			if (BattleCardRules::ShouldPlayHandDealSound(table_id)) {
				draw_card_se.playOneShot(
					GameStateRules::ClampVolume(getData().audio_settings.se_volume));
			}
			table_id++;
		}

		const Vec2 from{ 50, 900 };
		for (int32 slot = 0; slot < static_cast<int32>(hand.size()); ++slot) {
			const auto progress = BattleCardRules::ResolveHandDealProgress(slot, elapsed_seconds);
			if (!progress.started) continue;
			const Point hand_position = GetHandPosition(slot);
			const Vec2 to{ hand_position.x, hand_position.y };
			const Vec2 pos = from.lerp(to, progress.eased_progress);
			m_cards[hand[slot]].SetPos(pos.x, pos.y);
		}

		if (elapsed_seconds < BattleCardRules::HandDealTotalDuration(
			static_cast<int32>(hand.size()))) return;
		m_handDealComplete = true;
		return;
	}

    if (deal_stage == BattleCardRules::HandDealStage::ReturnPile) {
        yamahuda_angle -= Scene::DeltaTime() * 6.0; // 山札の回転速度を6.0として戻す．
        return;
    }
    m_currentAnimState = BattleAnimationState::Idle;
    m_animeStopwatch.restart();
    is_board_locked = false;
    yamahuda_angle = 0.0;
    now_turn++;
	m_enemyIntentState = {};
}

void Battle::updateWinEffect()
{
	// 敵撃破後の演出を完了させ，Victory結果に応じて次のシーンへ渡す．
    is_board_locked = true; // 盤面の操作をロック
    // 勝利演出の更新処理
    if (m_animeStopwatch.sF() < 2.0) // 2秒かけて敵をフェードアウトする．
    {   
        enemy_image_alpha = Math::Lerp(1.0, 0.0, m_animeStopwatch.sF()/2.0); // 敵の画像をフェードアウト
        return;
    }
    enemy_image_alpha = 0.0; // 敵の画像を完全にフェードアウト
    is_gamewin = true; // 勝利フラグを立てる
    if (m_animeStopwatch.sF() < 5.0) // 勝利演出を5秒まで待つ．
    {
        return; // 勝利演出の時間を待つ
    }
    if (!BattleCardRules::BeginOneShotTransition(is_scene_transition_started)) return;
    getData().money += reward_money;
	const auto progress = GameStateRules::ResolveVictory(m_enemy.type, getData().Layer);
    if (progress.destination == GameStateRules::VictoryDestination::Result) {
        getData().run_outcome = GameStateRules::RunOutcome::Clear;
        changeScene(State::Result); // リザルト画面へ遷移
    } else if (progress.destination == GameStateRules::VictoryDestination::NextActBattle) {
        getData().Layer = progress.next_layer;
        getData().Index = 1;
        getData().enemy = 0;
        changeScene(State::Battle);
    } else {
        changeScene(State::Map);
    }
}

void Battle::updateGameOverEffect()
{
	// 敗北演出を完了させ，RunOutcomeをResultシーンへ渡す．
    // 勝利演出の更新処理
    if (m_animeStopwatch.sF() < 1.0) // 1秒間ゲームオーバー演出を続ける．
    {   
        return;
    }
    if (!BattleCardRules::BeginOneShotTransition(is_scene_transition_started)) return;
    getData().run_outcome = GameStateRules::RunOutcome::GameOver;
    changeScene(State::Result);
}

void Battle::update()
{
	// ポインター所有権を確定し，Boardとバトル状態を一貫して更新する．
	m_frameNumber++;
	const bool was_idle = (m_currentAnimState == BattleAnimationState::Idle);
	const bool was_dragging = m_board.IsDragging();
	bool drag_started_this_frame = false;
	const BoardInputFrame input{
		Cursor::Pos(),
        MouseL.down(),
        MouseL.pressed(),
        MouseL.up(),
			KeyR.down(),
		Window::GetState().focused,
		m_frameNumber,
		Scene::DeltaTime(),
	};
    const bool can_accept_board_input = BattleCardRules::CanAcceptBattleInput(
        m_currentAnimState == BattleAnimationState::Idle,
		is_board_locked,
		is_scene_transition_started);
	if (!input.focused) {
		m_board.CancelActiveDrag();
		ApplyBoardZoneChanges();
		m_pointerInputOwner = BattleCardRules::PointerInputOwner::None;
		m_banner.CancelPointerGesture();
	} else if (!can_accept_board_input) {
		m_board.CancelActiveDrag();
		ApplyBoardZoneChanges();
	}

    if (is_deck) {
        m_pointerInputOwner = BattleCardRules::PointerInputOwner::Deck;
        is_deck = m_banner.update(m_cards, false, input.cursor,
            input.left_down, input.left_up, input.focused);
		m_board.Update(0, getData().leric.getLeric(), input, false);
		if (!is_deck) m_pointerInputOwner = BattleCardRules::PointerInputOwner::None;
		return;
	}
	if (is_settings_open) {
		updateSettingsOverlay(input);
		return;
	}
	if (input.focused && m_banner.IsSettingButtonHovered(input.cursor)) {
		Cursor::RequestStyle(CursorStyle::Hand);
	}
	if (!m_board.IsDragging() && input.focused && input.left_down
		&& m_banner.IsSettingButtonHovered(input.cursor)) {
		m_board.CompleteVisualMotions();
		ApplyBoardZoneChanges();
		m_pointerInputOwner = BattleCardRules::PointerInputOwner::None;
		m_banner.CancelPointerGesture();
		m_activeVolumeSlider = VolumeSlider::None;
		is_settings_open = true;
		m_animeStopwatch.pause();
		return;
	}

	if (m_board.IsDragging()) m_pointerInputOwner = BattleCardRules::PointerInputOwner::Card;
	if (can_accept_board_input) {
		const auto& hand = Cards(GameStateRules::CardZone::Hand);
		for (int32 slot = 0; slot < static_cast<int32>(hand.size()); slot++) {
			const int32 deck_index = hand[slot];
			if ((deck_index < 0) || (static_cast<int32>(m_cards.size()) <= deck_index)) continue;
			Block& block = m_cards[deck_index];
			if (block.GetStat() != 1) continue;
			const Point hand_pos = { block.GetPos().first, block.GetPos().second };
			const bool registered = m_board.RegisterHandBlock(block, deck_index, slot, hand_pos);
#ifndef NDEBUG
			if (!registered) {
				Logger << U"Failed to register hand reservation: frame=" << m_frameNumber
					<< U", deck_index=" << deck_index << U", slot=" << slot << U", position=" << hand_pos;
				assert(false && "Failed to register a unique hand reservation");
			}
#else
			(void)registered;
#endif
		}
	}

	int32 hand_hit_index = -1;
    const auto& hand = Cards(GameStateRules::CardZone::Hand);
    for (int32 i = static_cast<int32>(hand.size()) - 1; 0 <= i; --i) {
        const int32 deck_index = hand[i];
        if ((deck_index < 0) || (static_cast<int32>(m_cards.size()) <= deck_index)) continue;
        const Block& block = m_cards[deck_index];
		if ((block.GetStat() != 1) || !m_board.CanStartHandDrag(deck_index)
			|| !block.IsHovered(input.cursor)) continue;
		hand_hit_index = deck_index;
		break;
	}

    if ((m_pointerInputOwner == BattleCardRules::PointerInputOwner::None) && input.left_down) {
		const bool board_hit = (m_board.GetBoardCellAt(input.cursor) != Point{ -1,-1 });
        m_pointerInputOwner = BattleCardRules::CapturePointerOwner(
            false,
            m_board.IsDragging(),
			can_accept_board_input && !m_board.IsDragging(),
            m_banner.IsDeckButtonHovered(input.cursor),
			GetAttackButtonRect().contains(input.cursor),
			(0 <= hand_hit_index),
            board_hit);
    }

	const bool allow_deck_open = can_accept_board_input
		&& !m_board.IsDragging()
        && ((m_pointerInputOwner == BattleCardRules::PointerInputOwner::None)
            || (m_pointerInputOwner == BattleCardRules::PointerInputOwner::Deck));
	is_deck = m_banner.update(m_cards, allow_deck_open, input.cursor,
		input.left_down, input.left_up, input.focused);
	if (is_deck) m_board.CompleteVisualMotions();
#ifndef NDEBUG
    if (is_deck) assert(!m_board.IsDragging());
#endif
    if (is_deck) {
        m_board.Update(0, getData().leric.getLeric(), input, false);
        return; // デッキ画面の場合は処理を受け付けない
    }
    if (can_accept_board_input && GetAttackButtonRect().contains(input.cursor)) { // 「=」ボタンにマウスオーバーしている場合
        Cursor::RequestStyle(CursorStyle::Hand);
    }
    if (can_accept_board_input){ // 今のターンの敵の攻撃・防御を計算する。
        getEnemyInfo();
    }
	if ((m_pointerInputOwner == BattleCardRules::PointerInputOwner::Attack)
		&& input.left_down && can_accept_board_input && !m_board.IsDragging()) { // 「=」ボタンがクリックされた場合
        m_board.CancelActiveDrag();
		ApplyBoardZoneChanges();
        attack();
        return;
    }
    if (Cards(GameStateRules::CardZone::DrawPile).empty()
        && Cards(GameStateRules::CardZone::Hand).empty()
        && Cards(GameStateRules::CardZone::Board).empty()) {
        if (m_deckState.RecycleDiscard()) {
            for (const int32 id : Cards(GameStateRules::CardZone::DrawPile)) m_cards[id].SetStat(0);
        }
    }
    bool hand_capture_failed = false;
    if (can_accept_board_input && (0 <= hand_hit_index)) {
        Cursor::RequestStyle(CursorStyle::Hand);
		if ((m_pointerInputOwner == BattleCardRules::PointerInputOwner::Card)
			&& input.left_down && !m_board.IsDragging()) {
			Block& block = m_cards[hand_hit_index];
			if (m_board.PassBlock(block, hand_hit_index, input.cursor)) {
				drag_started_this_frame = true;
                drag_card_se.playOneShot(GameStateRules::ClampVolume(getData().audio_settings.se_volume)); // ドラッグの効果音を再生
            } else {
                hand_capture_failed = true;
            }
        }
    }
    my_hpbar.update(0.1);
    ene_hpbar.update(0.1);
	m_board.Update(0, getData().leric.getLeric(), input,
		BattleCardRules::CanProcessBoardInput(can_accept_board_input,
			hand_capture_failed, m_pointerInputOwner));
	ApplyBoardZoneChanges();
    if (input.left_up || !input.focused || (!input.left_down && !input.left_pressed)) {
        if (m_pointerInputOwner == BattleCardRules::PointerInputOwner::Deck) {
            m_banner.CancelPointerGesture();
        }
        m_pointerInputOwner = BattleCardRules::PointerInputOwner::None;
    }
    // 現在の状態で処理を分岐
    switch (m_currentAnimState) {
    case BattleAnimationState::Idle:
        updateTableDeck();
        break;
    case BattleAnimationState::CombatEnemyEffect:
        updateCombatEnemyEffect();
        break;
    case BattleAnimationState::CombatMyEffect:
        updateCombatMyEffect();
        break;
    case BattleAnimationState::DiscardEffect:
        updateDiscardEffect();
        break;
    case BattleAnimationState::CardDrawEffect:
        updateCardDrawEffect();
        break;
    case BattleAnimationState::WinEffect:
        updateWinEffect();
        break;
	case BattleAnimationState::GameOver:
		updateGameOverEffect();
		break;
	}
	const bool is_idle = (m_currentAnimState == BattleAnimationState::Idle);
	if (is_idle) {
		const double delta_seconds = Max(0.0, input.delta_seconds);
		m_playerAttackStatPulseElapsed = Min(
			BattleLayoutRules::CombatStatPulseDuration,
			m_playerAttackStatPulseElapsed + delta_seconds);
		m_playerDefenseStatPulseElapsed = Min(
			BattleLayoutRules::CombatStatPulseDuration,
			m_playerDefenseStatPulseElapsed + delta_seconds);
	} else {
		m_playerAttackStatPulseElapsed = BattleLayoutRules::CombatStatPulseDuration;
		m_playerDefenseStatPulseElapsed = BattleLayoutRules::CombatStatPulseDuration;
		m_playerCombatStatPulseChanges = {};
	}
	if (BattleCardRules::ShouldRefreshPlayerCombatValues(
		is_idle, was_idle,
		was_dragging, m_board.IsDragging(), drag_started_this_frame)) {
		const int32 previous_attack = my_attack;
		const int32 previous_defense = my_defense;
		tie(my_attack, my_defense) = m_board.Confirm();
		m_playerCombatStatPulseChanges =
			BattleLayoutRules::ResolvePlayerCombatStatPulseChanges(
				previous_attack, previous_defense, my_attack, my_defense);
		m_playerAttackStatPulseElapsed =
			m_playerCombatStatPulseChanges.attack
			? 0.0 : BattleLayoutRules::CombatStatPulseDuration;
		m_playerDefenseStatPulseElapsed =
			m_playerCombatStatPulseChanges.defense
			? 0.0 : BattleLayoutRules::CombatStatPulseDuration;
	}
}

void Battle::drawHandCards() const
{
	int32 hovered_deck_index = -1;
	const auto& hand = Cards(GameStateRules::CardZone::Hand);
	const bool deal_in_progress = (m_currentAnimState == BattleAnimationState::CardDrawEffect)
		&& !m_handDealComplete;
	const bool can_highlight = BattleCardRules::CanAcceptBattleInput(
		m_currentAnimState == BattleAnimationState::Idle,
		is_board_locked,
		is_scene_transition_started)
		&& !m_board.IsDragging()
		&& !is_settings_open
		&& Window::GetState().focused;
	if (can_highlight) {
		for (int32 slot = static_cast<int32>(hand.size()) - 1; 0 <= slot; --slot) {
			const int32 deck_index = hand[slot];
			if ((deck_index < 0) || (static_cast<int32>(m_cards.size()) <= deck_index)) continue;
			const Block& block = m_cards[deck_index];
			if ((block.GetStat() != 1) || !m_board.ShouldDrawAsHand(deck_index)
				|| !block.IsHovered(Cursor::Pos())) continue;
			hovered_deck_index = deck_index;
			break;
		}
	}

	for (int32 slot = 0; slot < static_cast<int32>(hand.size()); ++slot) {
		if (!BattleCardRules::ShouldDrawHandCardDuringDeal(
			slot, table_id, deal_in_progress)) continue;
		const int32 deck_index = hand[slot];
        if ((deck_index < 0) || (static_cast<int32>(m_cards.size()) <= deck_index)) continue;
        const Block& block = m_cards[deck_index];
		if ((deck_index != hovered_deck_index) && (block.GetStat() == 1)
			&& m_board.ShouldDrawAsHand(deck_index)) {
            block.Draw(block.GetPos());
        }
    }
	if (0 <= hovered_deck_index) {
		const Block& block = m_cards[hovered_deck_index];
		const auto [x, y] = block.GetPos();
		block.Draw({ x, y - BattleLayoutRules::HandHoverLift },
			BattleLayoutRules::HandHoverScale);
	}
}

// 戦闘画面全体の描画。常に呼び出す。
bool Battle::drawDefault() const
{
	const auto player_position = BattleLayoutRules::PlayerPosition();
	const auto player_hp_position = BattleLayoutRules::PlayerHpPosition();
	const auto enemy_position = BattleLayoutRules::EnemyPosition();
	const auto enemy_hp_position = BattleLayoutRules::EnemyHpPosition();
	const auto draw_pile_position = BattleLayoutRules::DrawPilePosition();
	const auto discard_pile_position = BattleLayoutRules::DiscardPilePosition();
	const auto equal_button = BattleLayoutRules::EqualButtonBounds();
	const double enemy_display_scale = enemy_base_scale * enemy_scale_multiplier;
	const auto player_combat_visibility = BattleLayoutRules::ResolvePlayerCombatValueVisibility(
		m_currentAnimState == BattleAnimationState::Idle,
		m_currentAnimState == BattleAnimationState::CombatEnemyEffect,
		m_currentAnimState == BattleAnimationState::CombatMyEffect,
		m_currentAnimState == BattleAnimationState::DiscardEffect);
	const bool is_idle = (m_currentAnimState == BattleAnimationState::Idle);
	const auto player_attack_combat_pulse = BattleLayoutRules::ResolveCombatStatPulse(
		m_playerAttackStatPulseElapsed,
		is_idle && m_playerCombatStatPulseChanges.attack);
	const auto player_defense_combat_pulse = BattleLayoutRules::ResolveCombatStatPulse(
		m_playerDefenseStatPulseElapsed,
		is_idle && m_playerCombatStatPulseChanges.defense);
	const auto draw_combat_stat = [this](const Texture& icon, const double icon_scale,
		const int32 value, const BattleLayoutRules::ScreenPoint icon_position,
		const BattleLayoutRules::ScreenPoint value_position,
		const BattleLayoutRules::CombatStatPulse pulse) {
		const Float2 pivot{
			static_cast<float>((icon_position.x + value_position.x) / 2.0),
			static_cast<float>((icon_position.y + value_position.y
				+ BattleLayoutRules::CombatFontSize) / 2.0),
		};
		const Transformer2D transform{
			Mat3x2::Scale(pulse.scale, pivot).translated(0.0, -pulse.lift) };
		icon.scaled(icon_scale).draw(icon_position.x, icon_position.y);
		m_combatFont(U"{}"_fmt(value)).draw(
			value_position.x, value_position.y, Palette::Black);
	};
    if (is_gamewin)
    {
        // 背景をぼかすための処理
        { 
            const ScopedRenderTarget2D target(m_combatSceneBuffer);
            m_backgroundTexture.scaled(0.5).draw();
			m_board.DrawBoard(0, m_currentAnimState == BattleAnimationState::Idle);
            drawHandCards();
            // プレイヤーのキャラクターを描画
            m_myTexture.scaled(BattleLayoutRules::PlayerDisplayScale).rotated(my_angle)
				.draw(player_position.x, player_position.y);
            // 敵の情報を描画
            m_enemy.texture.scaled(enemy_display_scale).drawAt(enemy_position.x, enemy_position.y,
				ColorF(1.0, 1.0, 1.0, enemy_image_alpha));
            // 山札のテクスチャを描画
            m_yamahudaTexture.scaled(0.75).rotated(yamahuda_angle).draw(draw_pile_position.x, draw_pile_position.y);
            // 捨て札のテクスチャを描画
            m_sutehudaTexture.scaled(0.6).rotated(sutehuda_angle).draw(discard_pile_position.x, discard_pile_position.y);
            // =buttonのテクスチャを描画
            m_buttonTexture.scaled(0.7).draw(equal_button.x, equal_button.y);
            my_hpbar.draw(RectF{ player_hp_position.x, player_hp_position.y, 240, 15 });
            ene_hpbar.draw(RectF{ enemy_hp_position.x, enemy_hp_position.y, 320, 20 });
            // 敵の攻撃アイコンの描画
            if (m_currentAnimState == BattleAnimationState::Idle || 
                m_currentAnimState == BattleAnimationState::CombatEnemyEffect){
				draw_combat_stat(m_attackIcon, BattleLayoutRules::AttackIconScale,
					ene_attack, BattleLayoutRules::EnemyCombatAttackIconPosition,
					BattleLayoutRules::EnemyCombatAttackValuePosition, {});
            }
            if (m_currentAnimState == BattleAnimationState::Idle || 
                m_currentAnimState == BattleAnimationState::CombatEnemyEffect ||
                m_currentAnimState == BattleAnimationState::CombatMyEffect ||
                m_currentAnimState == BattleAnimationState::DiscardEffect) {
                // 敵の防御アイコンの描画
				draw_combat_stat(m_defenceIcon, BattleLayoutRules::DefenseIconScale,
					ene_defense, BattleLayoutRules::EnemyCombatDefenseIconPosition,
					BattleLayoutRules::EnemyCombatDefenseValuePosition, {});
            }
			if (player_combat_visibility.attack) {
				draw_combat_stat(m_attackIcon, BattleLayoutRules::AttackIconScale,
					my_attack, BattleLayoutRules::PlayerCombatAttackIconPosition,
					BattleLayoutRules::PlayerCombatAttackValuePosition,
					player_attack_combat_pulse);
			}
            // 自分の防御アイコンの描画
			if (player_combat_visibility.defense) {
				draw_combat_stat(m_defenceIcon, BattleLayoutRules::DefenseIconScale,
					my_defense, BattleLayoutRules::PlayerCombatDefenseIconPosition,
					BattleLayoutRules::PlayerCombatDefenseValuePosition,
					player_defense_combat_pulse);
            }
            m_banner.draw(getData().money, getData().Layer, getData().leric);
        }
        Shader::GaussianBlur(m_combatSceneBuffer, m_blurInternalBuffer, m_combatSceneBuffer, BoxFilterSize::BoxFilter13x13); 
        m_combatSceneBuffer.draw();
    }
    else
    {
        m_backgroundTexture.scaled(0.5).draw();
        if (is_deck) {
            m_banner.draw(getData().money, getData().Layer, getData().leric);
            return true;
        }
		m_board.DrawBoard(0, m_currentAnimState == BattleAnimationState::Idle);
        drawHandCards();
        // プレイヤーのキャラクターを描画
        m_myTexture.scaled(BattleLayoutRules::PlayerDisplayScale).rotated(my_angle)
			.draw(player_position.x, player_position.y);
        // 敵の情報を描画
        m_enemy.texture.scaled(enemy_display_scale).drawAt(enemy_position.x, enemy_position.y,
			ColorF(1.0, 1.0, 1.0, enemy_image_alpha));
        // 山札のテクスチャを描画
        m_yamahudaTexture.scaled(0.75).rotated(yamahuda_angle).draw(draw_pile_position.x, draw_pile_position.y);
        // 捨て札のテクスチャを描画
        m_sutehudaTexture.scaled(0.6).rotated(sutehuda_angle).draw(discard_pile_position.x, discard_pile_position.y);
        // =buttonのテクスチャを描画
        m_buttonTexture.scaled(0.7).draw(equal_button.x, equal_button.y);
        my_hpbar.draw(RectF{ player_hp_position.x, player_hp_position.y, 240, 15 });
        ene_hpbar.draw(RectF{ enemy_hp_position.x, enemy_hp_position.y, 320, 20 });
        // 敵の攻撃アイコンの描画
        if (m_currentAnimState == BattleAnimationState::Idle || 
            m_currentAnimState == BattleAnimationState::CombatEnemyEffect){
			draw_combat_stat(m_attackIcon, BattleLayoutRules::AttackIconScale,
				ene_attack, BattleLayoutRules::EnemyCombatAttackIconPosition,
				BattleLayoutRules::EnemyCombatAttackValuePosition, {});
        }
        if (m_currentAnimState == BattleAnimationState::Idle || 
            m_currentAnimState == BattleAnimationState::CombatEnemyEffect ||
            m_currentAnimState == BattleAnimationState::CombatMyEffect ||
            m_currentAnimState == BattleAnimationState::DiscardEffect) {
            // 敵の防御アイコンの描画
			draw_combat_stat(m_defenceIcon, BattleLayoutRules::DefenseIconScale,
				ene_defense, BattleLayoutRules::EnemyCombatDefenseIconPosition,
				BattleLayoutRules::EnemyCombatDefenseValuePosition, {});
        }
		if (player_combat_visibility.attack) {
			draw_combat_stat(m_attackIcon, BattleLayoutRules::AttackIconScale,
				my_attack, BattleLayoutRules::PlayerCombatAttackIconPosition,
				BattleLayoutRules::PlayerCombatAttackValuePosition,
				player_attack_combat_pulse);
		}
        // 自分の防御アイコンの描画
		if (player_combat_visibility.defense) {
			draw_combat_stat(m_defenceIcon, BattleLayoutRules::DefenseIconScale,
				my_defense, BattleLayoutRules::PlayerCombatDefenseIconPosition,
				BattleLayoutRules::PlayerCombatDefenseValuePosition,
				player_defense_combat_pulse);
        }
        m_banner.draw(getData().money, getData().Layer, getData().leric);
    }
    return false;
}

// 戦闘演出の描画
void Battle::drawCombatEnemyEffect() const
{
	const auto draw_attack = [&](const double alpha) {
		const Float2 pivot{
			static_cast<float>(my_attack_icon_pos.x),
			static_cast<float>(my_attack_icon_pos.y),
		};
		const Transformer2D transform{
			Mat3x2::Scale(my_attack_icon_scale, pivot) };
		m_attackIcon.scaled(BattleLayoutRules::AttackIconScale).draw(
			my_attack_icon_pos, ColorF{ 1.0, 1.0, 1.0, alpha });
		m_combatFont(U"{}"_fmt(my_attack)).draw(
			my_attack_icon_pos + Vec2{ BattleLayoutRules::CombatValueAnimationOffset.x,
				BattleLayoutRules::CombatValueAnimationOffset.y },
			ColorF{ 0.0, 0.0, 0.0, alpha });
	};
    if (my_attack_type == -1 || my_attack_type == 0 || my_attack_type == 2)
    {
		draw_attack((my_attack_type == 2)
			? BattleLayoutRules::ResolveBodyAttackAlpha(m_animeStopwatch.sF()) : 1.0);
    }
    else if (my_attack_type == 1){
        if (m_animeStopwatch.sF() < 0.2){
			m_effectTexture.scaled(0.4).draw(my_attack_icon_pos);
            if (flag_once_draw == 0){
                attack_se.playOneShot(GameStateRules::ClampVolume(getData().audio_settings.se_volume));
            }
        }
		draw_attack(1.0);
    }
    else if (my_attack_type == 3){ // attack_effect
        if (BattleDamageRules::ShouldDrawHitEffect(
			static_cast<int32>(m_enemyDamageHits.size()), ene_effect_x, ene_effect_y)) {
            m_effectTexture.scaled(0.5).draw(ene_effect_x, ene_effect_y);
        } 
    }
}
void Battle::drawCombatMyEffect() const
{
	const auto draw_attack = [&](const double alpha) {
		const Float2 pivot{
			static_cast<float>(ene_attack_icon_pos.x),
			static_cast<float>(ene_attack_icon_pos.y),
		};
		const Transformer2D transform{
			Mat3x2::Scale(ene_attack_icon_scale, pivot) };
		m_attackIcon.scaled(BattleLayoutRules::AttackIconScale).draw(
			ene_attack_icon_pos, ColorF{ 1.0, 1.0, 1.0, alpha });
		m_combatFont(U"{}"_fmt(ene_attack)).draw(
			ene_attack_icon_pos + Vec2{ BattleLayoutRules::CombatValueAnimationOffset.x,
				BattleLayoutRules::CombatValueAnimationOffset.y },
			ColorF{ 0.0, 0.0, 0.0, alpha });
	};
    if (ene_attack_type == 0 || ene_attack_type == 2)
    {
		draw_attack((ene_attack_type == 2)
			? BattleLayoutRules::ResolveBodyAttackAlpha(m_animeStopwatch.sF()) : 1.0);
    }
    else if (ene_attack_type == 1){
        if (m_animeStopwatch.sF() < 0.2){
			m_effectTexture.scaled(0.4).draw(ene_attack_icon_pos);
            if (flag_once_draw == 0){
                attack_se.playOneShot(GameStateRules::ClampVolume(getData().audio_settings.se_volume));
            }
        }
		draw_attack(1.0);
    }
    else if (ene_attack_type == 3){ // attack_effect
        if (BattleDamageRules::ShouldDrawHitEffect(
			static_cast<int32>(m_playerDamageHits.size()), my_effect_x, my_effect_y)) {
            m_effectTexture.scaled(0.5).draw(my_effect_x, my_effect_y);

        } 
    }
}

// 戦闘後、余った手札を捨て札に移動するアニメーションの描画
void Battle::drawDiscardEffect() const
{
    if (flag_once_draw == 0) {
        // 修正
        // m_board.Discard();
    }
}
// 山札から手札に移動するアニメーションの描画
void Battle::drawCardDrawEffect() const
{
    // nothing
    return;
}

void Battle::drawWinEffect() const
{
    if (is_gamewin){
        // 勝利演出の描画(仮)
        // 2. ウィンドウの基本となる長方形を画面中央に定義します
        const Rect rect{ Arg::center = Scene::Center(), 600, 400 };

        // 3. 報酬というテキストを描画した際の、正確な領域を取得します
        const RectF textRect = m_rewardFont(U"報酬").region(Arg::center = rect.topCenter()); // font を m_rewardFont に変更

        rect.draw(Palette::White);
        rect.drawFrame(5.0, Palette::Black);
        textRect.stretched(10, 0).draw(Palette::White);
        
        m_rewardFont(U"報酬").draw(Arg::center = rect.topCenter(), Palette::Black);
        // 報酬の金額を描画
        m_reward_money.scaled(1.0).draw(Arg::center = rect.center()+Vec2{0.0, 80});
        // 報酬の金額を描画
        m_rewardFont(U"+{}"_fmt(reward_money)).draw(Arg::center = rect.center() + Vec2{0, -20}, Palette::White);
    }
    return;
}

void Battle::drawSettingsOverlay() const
{
	RectF{ 0, 0, Scene::Width(), Scene::Height() }.draw(ColorF{ 0.0, 0.0, 0.0, 0.58 });
	SettingsPanel.draw(ColorF{ 0.96, 0.94, 0.88 });
	SettingsPanel.drawFrame(4, ColorF{ 0.18 });
	m_numFont(U"設定").drawAt(SettingsPanel.center().x, SettingsPanel.y + 70, Palette::Black);

	const auto draw_slider = [this](const StringView label, const RectF& track,
		const double volume) {
		m_numFont(label).draw(650, track.y - 28, Palette::Black);
		track.rounded(6).draw(ColorF{ 0.62 });
		RectF{ track.x, track.y, track.w * GameStateRules::ClampVolume(volume), track.h }
			.rounded(6).draw(ColorF{ 0.25, 0.52, 0.82 });
		Circle{ track.x + track.w * GameStateRules::ClampVolume(volume),
			track.y + track.h / 2.0, 16 }.draw(Palette::White).drawFrame(3, ColorF{ 0.25 });
		m_numFont(U"{}%"_fmt(GameStateRules::VolumePercent(volume)))
			.draw(1230, track.y - 28, Palette::Black);
	};
	draw_slider(U"BGM", BgmSliderTrack, getData().audio_settings.bgm_volume);
	draw_slider(U"SE", SeSliderTrack, getData().audio_settings.se_volume);

	const bool close_hovered = SettingsCloseButton.contains(Cursor::Pos());
	SettingsCloseButton.rounded(12).draw(close_hovered
		? ColorF{ 0.62, 0.72, 0.86 } : ColorF{ 0.72, 0.78, 0.88 });
	SettingsCloseButton.rounded(12).drawFrame(3, ColorF{ 0.2 });
	m_numFont(U"戻る").drawAt(SettingsCloseButton.center(), Palette::Black);
}

void Battle::draw() const
{
    if (drawDefault()) return;
    // 現在の状態で描画処理を分岐
    switch (m_currentAnimState) {
    case BattleAnimationState::Idle:
        break;
    case BattleAnimationState::CombatEnemyEffect:
        drawCombatEnemyEffect();
        break;
    case BattleAnimationState::CombatMyEffect:
        drawCombatMyEffect();
        break;
    case BattleAnimationState::DiscardEffect:
        drawDiscardEffect();
        break;
    case BattleAnimationState::CardDrawEffect:
        drawCardDrawEffect();
        break;
    case BattleAnimationState::WinEffect:
        drawWinEffect();
        break;
    case BattleAnimationState::GameOver:
        // ゲームオーバーの描画処理
        break;
    }
	m_board.DrawInteractionOverlay();
	if (is_settings_open) drawSettingsOverlay();
}
