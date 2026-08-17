#include "Battle.hpp"
#include "Board.hpp" // BoardクラスのConfirm()などを使うためにインクルード
#include <cassert>
#include <tuple> // tie関数を使用するためにインクルード
using std::tie; // std::tieを使用するために名前空間を指定

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
    m_button_hantei = Rect{ 1600, 750, 175, 100 }; // ボタンの位置とサイズを設定
    m_rewardFont = Font{ 50, Typeface::Bold };
    m_numFont = Font{ 48, Typeface::Bold };
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
    Array<int32> draw_order;
    for (int32 i = 0; i < deck_size; i++) {
        m_cards[i].ResetRuntimeState();
        draw_order.push_back(i);
    }
    draw_order.shuffle();
    m_deckState.Initialize(deck_size,
        std::vector<int32>{ draw_order.begin(), draw_order.end() });

    //音楽再生！
    battle_bgm.play(); // 音楽を再生
}


void Battle::setupEnemy(int32 type, int32 layer)
{
    const EnemyData& data = m_enemyDB.getOneEnemy(type, layer);
    m_enemy.name = data.name;
    m_enemy.texture = Texture(data.texturePath);
    m_enemy.type = data.type;
    m_enemy.maxHp = data.maxHp;
    m_enemy.hp = data.maxHp;
    m_enemy.actionPattern = data.actionPattern;
    ene_hpbar = HPBar{m_enemy.maxHp, m_enemy.hp};
}

// 山札に配置できる最大枚数を盤面の情報から求める関数
int32 Battle::getTableSize() const
{
    return GameStateRules::CalculateHandLimit(getData().board_progress);
}

// 0:山札, 1:手札, 2:盤面, -1:捨て札
// 盤面と山札のデッキの状況をリアルタイムで監視する関数
void Battle::updateTableDeck()
{
    ApplyBoardZoneChanges();
    AssertCardOwnership("updateTableDeck");
}

const std::vector<int32>& Battle::Cards(const GameStateRules::CardZone zone) const
{
    return m_deckState.Cards(zone);
}

bool Battle::MoveCard(const int32 card_id, const GameStateRules::CardZone expected,
    const GameStateRules::CardZone destination)
{
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
    // 現在アニメーション中でない場合のみ処理を開始
    if (m_currentAnimState == BattleAnimationState::Idle) {
        // 盤面の操作をロックする
        is_board_locked = true; // 盤面の操作をロック
        // 攻撃・防御の処理を行う
        // 盤面から攻撃力と防御力を取得
        tie(my_attack, my_defense) = m_board.Confirm();
        // プレイヤー->敵の攻撃力を計算
        my_real_attack = Max(0, my_attack - ene_defense); // プレイヤーの攻撃力から敵の防御力を引く
        m_enemy.hp -= my_real_attack; // プレイヤーのHPを減らす
        // 敵->プレイヤーの攻撃力を計算
        ene_real_attack = Max(0, ene_attack - my_defense); // 敵の攻撃力から防御力を引く
        getData().HP -= ene_real_attack; // 敵のHPを減らす
        if (getData().HP < 0) getData().HP = 0; // プレイヤーのHPが負にならないようにする
        // 自分・敵の防御力を減らすエフェクトのための変数を設定
        ene_defense_effect = ene_defense; // 敵の防御力を減らすエフェクトのための変数
        my_defense_effect = my_defense; // 自分の防御力を減
        // 敵がボス3の場合、敵のHPを増やす
        if (is_boss3) {
            m_enemy.hp += my_real_attack;
        }
        // damage-effectの演出のための制御変数を設定
        my_res_real_attack = my_real_attack % damage_effect_width;
        ene_res_real_attack = ene_real_attack % damage_effect_width;
        ene_damage_effect_cnt = 0;
        ene_damage_max_cnt = (my_real_attack+damage_effect_width-1) / damage_effect_width;
        my_damage_max_cnt = (ene_real_attack+damage_effect_width-1) / damage_effect_width;
        // attack/defecce の演出のための変数を設定
        my_attack_icon_start = Vec2{440, 640};
        my_attack_icon_end = Vec2{1270, 760};
        ene_attack_icon_start = Vec2{1330, 640};
        ene_attack_icon_end = Vec2{460, 760};
        my_attack_icon_pos = my_attack_icon_start; // 自分の攻撃アイコンの位置を初期化
        ene_attack_icon_pos = ene_attack_icon_start; // 敵の攻撃アイコン
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
    if (my_attack_type == -1 && m_animeStopwatch.sF() < 0.4) {
        return;
    }
    if (my_attack_type == -1){
        my_attack_type = 0;
        m_animeStopwatch.restart();
    }
    // 自分の攻撃アイコン->敵の防御アイコンへ移動させる演出
    if (my_attack_type == 0 && m_animeStopwatch.sF() < 0.4) {
        my_attack_icon_pos = Math::Lerp(my_attack_icon_start, my_attack_icon_end, m_animeStopwatch.sF()/0.4);
        return;
    }
    if (my_attack_type == 0) {
        my_attack_type = 1;
        my_attack_icon_pos = my_attack_icon_end; // エフェクトの位置を最終位置に設定
        my_attack_icon_start = my_attack_icon_end;
        my_attack_icon_end = Vec2{1500, 400};
        m_animeStopwatch.restart(); // ストップウォッチをリセット
        return;
    }
    // 敵の防御を減らす演出
    if (my_attack_type == 1 && m_animeStopwatch.sF() < 0.7) {
        flag_once_draw++;
        ene_defense = Math::Lerp(ene_defense_effect, Max(0, ene_defense_effect-my_attack), m_animeStopwatch.sF()/0.7);
        return;
    }
    if (my_attack_type == 1){
        ene_defense = Max(0, ene_defense_effect - my_attack);
        my_attack = my_real_attack;
        my_attack_type = (my_attack == 0 ? 3 : 2);
        m_animeStopwatch.restart(); // ストップウォッチをリセット
        return;
    }
    // 自分の攻撃アイコン->敵の画像へ移動させる演出
    if (my_attack_type == 2 && m_animeStopwatch.sF() < 0.35) {
        my_attack_icon_pos = Math::Lerp(my_attack_icon_start, my_attack_icon_end, m_animeStopwatch.sF()/0.35);
        return;
    }
    if (my_attack_type == 2){
        my_attack_icon_pos = my_attack_icon_end; // エフェクトの位置を最終位置に設定
        my_attack_type = 3;
        m_animeStopwatch.restart(); // ストップウォッチをリセット
        return;
    }
    enemy_scale = Min(0.85, enemy_scale + Scene::DeltaTime());
    // damage_effectを表示するための制御
    if (my_attack_type == 3 && ene_damage_effect_cnt < ene_damage_max_cnt) {
        if (m_animeStopwatch.sF() > 0.20 * ene_damage_effect_cnt) {
            if (ene_damage_effect_cnt == 0 && my_res_real_attack) ene_hpbar.damage(my_res_real_attack); // 敵のHPバーを減らす
            else ene_hpbar.damage(damage_effect_width); // 敵のHPバーを減らす
            ene_effect_x = Random(1350, 1600); // エフェクトのX座標をランダムに設定
            ene_effect_y = Random(200, 450); // エフェクトのY座標をランダムに設定
            ene_damage_effect_cnt++;
            enemy_scale = 0.7;
            // SE再生
            attack_se.playOneShot();
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
    enemy_scale = 0.85; // エフェクトの拡大を元に戻す
    m_currentAnimState = BattleAnimationState::CombatMyEffect;
    m_animeStopwatch.restart();
}

// 戦闘演出の更新処理
void Battle::updateCombatMyEffect()
{
    if (ene_attack_type == 0 && m_animeStopwatch.sF() < 0.4) {
        ene_attack_icon_pos = Math::Lerp(ene_attack_icon_start, ene_attack_icon_end, m_animeStopwatch.sF()/0.4);
        return;
    }
    if (ene_attack_type == 0) {
        ene_attack_type = 1;
        ene_attack_icon_pos = ene_attack_icon_end; // エフェクトの位置を最終位置に設定
        ene_attack_icon_start = ene_attack_icon_end;
        ene_attack_icon_end = Vec2{200, 250};
        m_animeStopwatch.restart(); // ストップウォッチをリセット
        return;
    }
    if (ene_attack_type == 1 && m_animeStopwatch.sF() < 0.7) {
        flag_once_draw++;
        my_defense = Math::Lerp(my_defense_effect, Max(0, my_defense_effect-ene_attack), m_animeStopwatch.sF()/0.7);
        return;
    }
    if (ene_attack_type == 1){
        my_defense = Max(0, my_defense_effect - ene_attack);
        ene_attack = ene_real_attack;
        ene_attack_type = (ene_attack == 0 ? 3 : 2);
        m_animeStopwatch.restart(); // ストップウォッチをリセット
        return;
    }
    if (ene_attack_type == 2 && m_animeStopwatch.sF() < 0.35) {
        ene_attack_icon_pos = Math::Lerp(ene_attack_icon_start, ene_attack_icon_end, m_animeStopwatch.sF()/0.35);
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
    if (ene_attack_type == 3 && my_damage_effect_cnt < my_damage_max_cnt) {
        if ((m_animeStopwatch.sF()) > 0.20 * my_damage_effect_cnt) {
            if (my_damage_effect_cnt == 0 && ene_res_real_attack) my_hpbar.damage(ene_res_real_attack); // 自分のHPバーを減らす
            else my_hpbar.damage(damage_effect_width); // 自分のHPバーを減らす
            my_effect_x = Random(150, 300); // エフェクトのX座標をランダムに設定
            my_effect_y = Random(130, 230); // エフェクトのY座標をランダムに設定
            my_damage_effect_cnt++;
            my_angle = Random(-0.52, -0.1); // -π/4 ~ -π/6の範囲でプレイヤーを傾かさせる
            // SE再生
            attack_se.playOneShot();
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
    table_id = static_cast<int32>(Cards(GameStateRules::CardZone::Hand).size()) - 1;
    my_angle = 0.0;
    m_currentAnimState = BattleAnimationState::DiscardEffect;
    m_animeStopwatch.restart();
}

// 捨て札アニメーション(盤面, 手札 -> 捨て札)の更新処理
void Battle::updateDiscardEffect()
{
	const auto& hand = Cards(GameStateRules::CardZone::Hand);
    if ((0 <= table_id) && (table_id < static_cast<int32>(hand.size()))) {
        if (sutehuda_angle > -90_deg) {
            table_id = static_cast<int32>(hand.size()) - 1;
            sutehuda_angle -= Scene::DeltaTime() * 4.0;
            m_animeStopwatch.restart();
        } else {
            sutehuda_angle = -90_deg;
            tehuda_rate = Min(1.0, m_animeStopwatch.sF() / 0.15); // 捨て札の位置を徐々に変える
            Vec2 from{ 200 + table_id * 100, 900 };
            Vec2 to{ 1610, 950 };
            Vec2 pos = from.lerp(to, tehuda_rate);
            const int32 card_id = hand[table_id];
			if (!m_board.DetachCard(card_id)) {
#ifndef NDEBUG
				assert(false && "Failed to detach hand card before discard animation");
#endif
				return;
			}
            m_cards[card_id].SetPos(pos.x, pos.y); // 手札
            if (tehuda_rate >= 1) {
                MoveCard(card_id, GameStateRules::CardZone::Hand, GameStateRules::CardZone::Discard);
                table_id = static_cast<int32>(hand.size()) - 1;
                tehuda_rate = 0; // 捨て札の位置を固定
                m_animeStopwatch.restart(); // ストップウォッチをリセット
            }
        }
        return;
    }
    if (sutehuda_angle < 0.01) {
        sutehuda_angle += Scene::DeltaTime() * 7.0; // 山札の角度を徐々に戻す
        return;
    }
    m_board.EndTurn();
    const auto board_cards = Cards(GameStateRules::CardZone::Board);
    for (const int32 deck_index : board_cards) {
        MoveCard(deck_index, GameStateRules::CardZone::Board, GameStateRules::CardZone::Discard);
    }
    m_board.BeginTurn();
    sutehuda_angle = 0.0;
    table_id = 0;
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
        m_cards.at(id).SetPos(200, 950);
    }

    if ((0 <= table_id) && (table_id < static_cast<int32>(hand.size()))) // 手札のカードを山札から引く
    {
        if (yamahuda_angle < 90_deg) {
            table_id = 0;
            yamahuda_angle += Scene::DeltaTime() * 4.0;
            tehuda_rate = 0;
            m_animeStopwatch.restart();
        } else {
            yamahuda_angle = 90_deg;
            if (tehuda_rate == 0.0) {
                //効果音
                draw_card_se.playOneShot(); // カードドローの効果音を再生
            }
            tehuda_rate = Min(1.0, m_animeStopwatch.sF() / 0.3);
            Vec2 from{ 50, 900 };
            Vec2 to{ 350 + table_id * 75, 900 };
            Vec2 pos = from.lerp(to, tehuda_rate);
            m_cards[hand[table_id]].SetPos(pos.x, pos.y); // 手札
            if (tehuda_rate >= 1) {
                table_id++;
                tehuda_rate = 0;
                m_animeStopwatch.restart();
            }
        }
        return;
    }
    if (yamahuda_angle > 0.01) {
        yamahuda_angle -= Scene::DeltaTime() * 6.0; // 山札の角度を徐々に戻す
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
    is_board_locked = true; // 盤面の操作をロック
    // 勝利演出の更新処理
    if (m_animeStopwatch.sF() < 2.0) // 1秒経過したら
    {   
        enemy_image_alpha = Math::Lerp(1.0, 0.0, m_animeStopwatch.sF()/2.0); // 敵の画像をフェードアウト
        return;
    }
    enemy_image_alpha = 0.0; // 敵の画像を完全にフェードアウト
    is_gamewin = true; // 勝利フラグを立てる
    if (m_animeStopwatch.sF() < 5.0) // 2秒経過したら
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
    // 勝利演出の更新処理
    if (m_animeStopwatch.sF() < 1.0) // 1秒経過したら
    {   
        return;
    }
    if (!BattleCardRules::BeginOneShotTransition(is_scene_transition_started)) return;
    getData().run_outcome = GameStateRules::RunOutcome::GameOver;
    changeScene(State::Result);
}

void Battle::update()
{
	m_frameNumber++;
	const BoardInputFrame input{
		Cursor::Pos(),
        MouseL.down(),
        MouseL.pressed(),
        MouseL.up(),
		MouseR.down(),
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
        const bool board_hit = Rect{ 600, 170, 7 * 90, 6 * 90 }.contains(input.cursor);
        m_pointerInputOwner = BattleCardRules::CapturePointerOwner(
            false,
            m_board.IsDragging(),
			can_accept_board_input && !m_board.IsDragging(),
            m_banner.IsDeckButtonHovered(input.cursor),
            m_button_hantei.contains(input.cursor),
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
    if (can_accept_board_input && m_button_hantei.contains(input.cursor)) { // 「=」ボタンにマウスオーバーしている場合
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
                drag_card_se.playOneShot(); // ドラッグの効果音を再生
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
}

void Battle::drawHandCards() const
{
    for (const int32 deck_index : Cards(GameStateRules::CardZone::Hand)) {
        if ((deck_index < 0) || (static_cast<int32>(m_cards.size()) <= deck_index)) continue;
        const Block& block = m_cards[deck_index];
		if ((block.GetStat() == 1) && m_board.ShouldDrawAsHand(deck_index)) {
            block.Draw(block.GetPos());
        }
    }
}

// 戦闘画面全体の描画。常に呼び出す。
bool Battle::drawDefault() const
{
	constexpr int32 enemy_intent_icon_x = 1450;
	constexpr int32 enemy_intent_value_x = 1530;
    if (is_gamewin)
    {
        // 背景をぼかすための処理
        { 
            const ScopedRenderTarget2D target(m_combatSceneBuffer);
            m_backgroundTexture.scaled(0.5).draw();
            m_board.DrawBoard(0);
            drawHandCards();
            // プレイヤーのキャラクターを描画
            m_myTexture.scaled(0.75).rotated(my_angle).draw(180, 230);
            // 敵の情報を描画
            m_enemy.texture.scaled(enemy_scale).draw(1480, 350, ColorF(1.0, 1.0, 1.0, enemy_image_alpha));
            // 山札のテクスチャを描画
            m_yamahudaTexture.scaled(0.75).rotated(yamahuda_angle).draw(50, 800);
            // 捨て札のテクスチャを描画
            m_sutehudaTexture.scaled(0.6).rotated(sutehuda_angle).draw(1600, 880);
            // =buttonのテクスチャを描画
            m_buttonTexture.scaled(0.7).draw(1600, 750);
            my_hpbar.draw(RectF{130, 700, 320, 20 });
            ene_hpbar.draw(RectF{1500, 700, 320, 20 });
            // 敵の攻撃アイコンの描画
            if (m_currentAnimState == BattleAnimationState::Idle || 
                m_currentAnimState == BattleAnimationState::CombatEnemyEffect){
                m_attackIcon.scaled(1.6).draw(enemy_intent_icon_x, 640);
                m_numFont(U"{}"_fmt(ene_attack)).draw(enemy_intent_value_x, 640, Palette::Black);
            }
            if (m_currentAnimState == BattleAnimationState::Idle || 
                m_currentAnimState == BattleAnimationState::CombatEnemyEffect ||
                m_currentAnimState == BattleAnimationState::CombatMyEffect ||
                m_currentAnimState == BattleAnimationState::DiscardEffect) {
                // 敵の防御アイコンの描画
                m_defenceIcon.scaled(1.5).draw(enemy_intent_icon_x, 720);
                m_numFont(U"{}"_fmt(ene_defense)).draw(enemy_intent_value_x, 720, Palette::Black);
            }
            // 自分の防御アイコンの描画
            if (m_currentAnimState == BattleAnimationState::CombatEnemyEffect || 
                m_currentAnimState == BattleAnimationState::CombatMyEffect ||
                m_currentAnimState == BattleAnimationState::DiscardEffect){
                m_defenceIcon.scaled(1.5).draw(440, 720);
                m_numFont(U"{}"_fmt(my_defense)).draw(520, 720, Palette::Black);
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
        m_board.DrawBoard(0);
        drawHandCards();
        // プレイヤーのキャラクターを描画
        m_myTexture.scaled(0.75).rotated(my_angle).draw(180, 230);
        // 敵の情報を描画
        m_enemy.texture.scaled(enemy_scale).draw(1480, 350, ColorF(1.0, 1.0, 1.0, enemy_image_alpha));
        // 山札のテクスチャを描画
        m_yamahudaTexture.scaled(0.75).rotated(yamahuda_angle).draw(50, 800);
        // 捨て札のテクスチャを描画
        m_sutehudaTexture.scaled(0.6).rotated(sutehuda_angle).draw(1600, 880);
        // =buttonのテクスチャを描画
        m_buttonTexture.scaled(0.7).draw(1600, 750);
        my_hpbar.draw(RectF{ 110, 700, 320, 20 });
        ene_hpbar.draw(RectF{ 1500, 700, 320, 20 });
        // 敵の攻撃アイコンの描画
        if (m_currentAnimState == BattleAnimationState::Idle || 
            m_currentAnimState == BattleAnimationState::CombatEnemyEffect){
            m_attackIcon.scaled(1.6).draw(enemy_intent_icon_x, 640);
            m_numFont(U"{}"_fmt(ene_attack)).draw(enemy_intent_value_x, 640, Palette::Black);
        }
        if (m_currentAnimState == BattleAnimationState::Idle || 
            m_currentAnimState == BattleAnimationState::CombatEnemyEffect ||
            m_currentAnimState == BattleAnimationState::CombatMyEffect ||
            m_currentAnimState == BattleAnimationState::DiscardEffect) {
            // 敵の防御アイコンの描画
            m_defenceIcon.scaled(1.5).draw(enemy_intent_icon_x, 720);
            m_numFont(U"{}"_fmt(ene_defense)).draw(enemy_intent_value_x, 720, Palette::Black);
        }
        // 自分の防御アイコンの描画
        if (m_currentAnimState == BattleAnimationState::CombatEnemyEffect || 
            m_currentAnimState == BattleAnimationState::CombatMyEffect ||
            m_currentAnimState == BattleAnimationState::DiscardEffect){
            m_defenceIcon.scaled(1.5).draw(440, 720);
            m_numFont(U"{}"_fmt(my_defense)).draw(520, 720, Palette::Black);
        }
        m_banner.draw(getData().money, getData().Layer, getData().leric);
    }
    return false;
}

// 戦闘演出の描画
void Battle::drawCombatEnemyEffect() const
{
    if (my_attack_type == -1 || my_attack_type == 0 || my_attack_type == 2)
    {
        m_attackIcon.scaled(1.6).draw(my_attack_icon_pos);
        m_numFont(U"{}"_fmt(my_attack)).draw(my_attack_icon_pos+Vec2{80, 0}, Palette::Black);
    }
    else if (my_attack_type == 1){
        if (m_animeStopwatch.sF() < 0.2){
            m_effectTexture.scaled(0.4).draw(1330, 720);
            if (flag_once_draw == 0){
                attack_se.playOneShot();
            }
        }
        m_attackIcon.scaled(1.6).draw(my_attack_icon_pos);
        m_numFont(U"{}"_fmt(my_attack)).draw(my_attack_icon_pos+Vec2{80, 0}, Palette::Black);
    }
    else if (my_attack_type == 3){ // attack_effect
        if (ene_effect_x != -1 && ene_effect_y != -1) {
            m_effectTexture.scaled(0.5).draw(ene_effect_x, ene_effect_y);
        } 
    }
}
void Battle::drawCombatMyEffect() const
{
    if (ene_attack_type == 0 || ene_attack_type == 2)
    {
        m_attackIcon.scaled(1.6).draw(ene_attack_icon_pos);
        m_numFont(U"{}"_fmt(ene_attack)).draw(ene_attack_icon_pos+Vec2{80, 0}, Palette::Black);
    }
    else if (ene_attack_type == 1){
        if (m_animeStopwatch.sF() < 0.2){
            m_effectTexture.scaled(0.4).draw(440, 720);
            if (flag_once_draw == 0){
                attack_se.playOneShot();
            }
        }
        m_attackIcon.scaled(1.6).draw(ene_attack_icon_pos);
        m_numFont(U"{}"_fmt(ene_attack)).draw(ene_attack_icon_pos+Vec2{80, 0}, Palette::Black);
    }
    else if (ene_attack_type == 3){ // attack_effect
        if (my_effect_x != -1 && my_effect_y != -1) {
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
}
