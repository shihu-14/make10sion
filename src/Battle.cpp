#include "Battle.hpp"
#include "Board.hpp" // BoardクラスのConfirm()などを使うためにインクルード
#include <tuple> // tie関数を使用するためにインクルード
using std::tie; // std::tieを使用するために名前空間を指定

// Constructor
Battle::Battle(const InitData& init)
    : IScene(init),
    is_board_locked(true),
    now_turn(-1), // ターン数を初期化// 手札のサイズを取得
    deck_size((int32)getData().Deck.size()), // グローバルのDeckのサイズを取得
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
    m_banner.init(getData().money, getData().Layer, getData().leric); // バナーの初期化
    m_board.InitAll();
    table_max_size = getTableSize();
    // 通常的かエリートかボスかどうやって決めるの？
    int32 enemy_type = 0; 
    setupEnemy(enemy_type, getData().Layer+1);
    my_hpbar = HPBar{ getData().MaxHP, getData().HP }; // 自分のHPバーの初期化
    action_cycle = m_enemy.actionPattern.size(); // 敵の行動パターンのサイクルを設定
    reward_money = m_enemy.type == 0 ? 20 : m_enemy.type == 1 ? 40 : 100; // 報酬の金額を設定
    // --- デッキの初期化 ---
    for (int i = 0; i < deck_size; i++) {
        getData().Deck[i].SetStat(0);
        Deck_id.emplace_back(i);
    }
    Deck_yama = Deck_id; // 山札の初期化
    Deck_yama.shuffle();

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
    return Min(15, m_board.unlocked_num/2 + 2);
}

// 0:山札, 1:手札, 2:盤面, -1:捨て札
// 盤面と山札のデッキの状況をリアルタイムで監視する関数
void Battle::updateTableDeck()
{
    // グローバルのDeckのstate変数を見て、盤面か手札かを参照し、Deck_tableとDeck_boardを更新する。
    Deck_table.clear();
    Deck_board.clear();
    for (int i = 0; i < deck_size; i++) {
        const int32 stat = getData().Deck[i].GetStat();
        if (stat == 1) {
            Deck_table.push_back(i);
        } else if (stat == 2) {
            Deck_board.push_back(i);
        }
    }
}

void Battle::getEnemyInfo()
{
    if (m_currentAnimState == BattleAnimationState::Idle) {
        // 盤面の操作をロックする
        // 敵の攻撃・防御を取得
        ene_attack = m_enemy.actionPattern[now_turn % action_cycle + turn_start].attack;
        ene_defense = m_enemy.actionPattern[now_turn % action_cycle + turn_start].defense;
        // -------特殊攻撃--------
        if (ene_attack == -10) {
            ene_attack = 3 + 2 * (table_max_size - (int32)Deck_table.size());
        } else if (ene_attack == -11) {
            ene_attack = 20;
            now_turn++;
            turn_start = now_turn;
            action_cycle = 4;
        } else if (ene_attack == -12) {
            ene_attack = 60 - 4 * (table_max_size - (int32)Deck_table.size());
        } else if (ene_attack == -13) {
            ene_attack = 40;
            getData().money -= 30;
        } else if (ene_attack == -14) {
            is_exit = true;
            // 逃走の処理は保留
        } else if (ene_attack == -15) {
            ene_attack = 10 + 14 * (table_max_size - (int32)Deck_table.size());
        } else if (ene_attack == -16) {
            ene_attack = 30;
            now_turn++;
            turn_start = now_turn;
            action_cycle = 5;
        } else if (ene_attack == -17) {
            ene_attack = 80;
            is_boss3 = true; // ボス3の敵
        } else if (ene_attack == -18) {
            ene_attack = 2 + 3 * (table_max_size - (int32)Deck_table.size());
        } else if (ene_attack == -19) {
            ene_attack = 3 + 5 * (table_max_size - (int32)Deck_table.size());
        }      
    }
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
        is_result = true;
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
        if (getData().Layer >= 30) // 最後のボスか
        {
            is_result = true;
        }
        m_currentAnimState = BattleAnimationState::GameOver;
        m_animeStopwatch.restart();
        return;
    }
    table_id = Deck_table.size() - 1;
    my_angle = 0.0;
    m_currentAnimState = BattleAnimationState::DiscardEffect;
    m_animeStopwatch.restart();
}

// 捨て札アニメーション(盤面, 手札 -> 捨て札)の更新処理
void Battle::updateDiscardEffect()
{
    if (table_id >= 0) {
        if (sutehuda_angle > -90_deg) {
            table_id = Deck_table.size() - 1;
            sutehuda_angle -= Scene::DeltaTime() * 4.0;
            m_animeStopwatch.restart();
        } else {
            // 盤面にあるなら飛ばす
            if (getData().Deck[Deck_table[table_id]].GetStat() != 1){
                // board側で、stateを盤面(2)->捨て札(-1)に変更しているなら不要な処理
                if (getData().Deck[Deck_table[table_id]].GetStat() == 2)
                {
                    getData().Deck[Deck_table[table_id]].SetStat(-1);
                }
                Deck_gomi.emplace_back(Deck_table[table_id]);
                table_id--;
                return;
            }
            sutehuda_angle = -90_deg;
            tehuda_rate = Min(1.0, m_animeStopwatch.sF() / 0.15); // 捨て札の位置を徐々に変える
            Vec2 from{ 200 + table_id * 100, 900 };
            Vec2 to{ 1610, 950 };
            Vec2 pos = from.lerp(to, tehuda_rate);
            getData().Deck[Deck_table[table_id]].SetPos(pos.x, pos.y); // 手札
            if (tehuda_rate >= 1) {
                Deck_gomi.emplace_back(Deck_table[table_id]);
                getData().Deck[Deck_table[table_id]].SetStat(-1);
                table_id--;
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
    for (const int32 deck_index : Deck_board) {
        if ((deck_index < 0) || (deck_size <= deck_index)) continue;
        if (getData().Deck[deck_index].GetStat() == 2) {
            getData().Deck[deck_index].SetStat(-1);
            if (!Deck_gomi.includes(deck_index)) Deck_gomi.push_back(deck_index);
        }
    }
    Deck_board.clear();
    // m_board.clearBoard();
    sutehuda_angle = 0.0;
    table_id = 0;
    Deck_table.clear();
    m_currentAnimState = BattleAnimationState::CardDrawEffect;
    m_animeStopwatch.restart();
}

// カードドローアニメーションの更新処理
void Battle::updateCardDrawEffect()
{
    // 山札から手札へ移動する。
    while (Deck_yama.size() && (int32)Deck_table.size() < table_max_size) {
        int id = Deck_yama.back();
        Deck_yama.pop_back();
        Deck_table.push_back(id);
        getData().Deck.at(id).SetStat(1); // 手札のステータスを1に設定
        getData().Deck.at(id).SetPos(200, 950);
    }

    if (table_id < Deck_table.size()) // 手札のカードを山札から引く
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
            getData().Deck[Deck_table[table_id]].SetPos(pos.x, pos.y); // 手札
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
    m_board.InitAll();
    m_currentAnimState = BattleAnimationState::Idle;
    m_animeStopwatch.restart();
    is_board_locked = false;
    yamahuda_angle = 0.0;
    now_turn++;
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
    if (is_scene_transition_started) return;
    is_scene_transition_started = true;
    if (getData().Layer >= 30) // 最後の勝利か
    {
        changeScene(State::Result); // リザルト画面へ遷移
    }
    else
    {
        // 勝利した場合、報酬を与える
        getData().money += reward_money; // 報酬を追加
        m_enemyDB.markAsDefeated(m_enemy.name);
        // まだ倒すべき敵が残っている場合 -> Mapシーンへ戻る
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
    if (is_scene_transition_started) return;
    is_scene_transition_started = true;
    changeScene(State::Result);
}

void Battle::update()
{
    is_deck = m_banner.update(getData().Deck);
    const bool can_accept_board_input = (m_currentAnimState == BattleAnimationState::Idle)
        && !is_board_locked
        && !is_scene_transition_started;
    if (is_deck) {
        m_board.Update(0, getData().leric.getLeric(), false);
        return; // デッキ画面の場合は処理を受け付けない
    }
    if (can_accept_board_input && m_button_hantei.mouseOver()) { // 「=」ボタンにマウスオーバーしている場合
        Cursor::RequestStyle(CursorStyle::Hand);
    }
    if (can_accept_board_input){ // 今のターンの敵の攻撃・防御を計算する。
        getEnemyInfo();
    }
    if (m_button_hantei.leftClicked() && can_accept_board_input && !m_board.IsBusy()) { // 「=」ボタンがクリックされた場合
        attack();
        return;
    }
    if (Deck_yama.isEmpty() && Deck_table.isEmpty() && Deck_board.isEmpty()) { // 山札を使い切った場合
        // m_currentAnimState = BattleAnimationState::GameOver; // gameoverになるんだっけ？
        Deck_yama = Deck_gomi;
        Deck_gomi.clear();
        for (auto id: Deck_yama) {
            getData().Deck[id].SetStat(0); // 山札の状態に戻す
        }
    }
    if (can_accept_board_input && !m_board.IsBusy()){
        for (int32 i = static_cast<int32>(Deck_table.size()) - 1; 0 <= i; --i) {
            const int32 deck_index = Deck_table[i];
            if ((deck_index < 0) || (static_cast<int32>(getData().Deck.size()) <= deck_index)) continue;
            Block& block = getData().Deck[deck_index];
            if ((block.GetStat() == 1) && block.IsDragging()) {
                const Point hand_pos = { block.GetPos().first, block.GetPos().second };
                if (m_board.PassBlock(block, deck_index, hand_pos)) {
                    drag_card_se.playOneShot(); // ドラッグの効果音を再生
                    return;
                }
            }
            if (block.IsHovered()) {
                Cursor::RequestStyle(CursorStyle::Hand);
                break;
            }
        }
    }
    my_hpbar.update(0.1);
    ene_hpbar.update(0.1);
    m_board.Update(0, getData().leric.getLeric(), can_accept_board_input);
    // m_banner.update(getData().Deck);
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
    for (const int32 deck_index : Deck_table) {
        if ((deck_index < 0) || (static_cast<int32>(getData().Deck.size()) <= deck_index)) continue;
        const Block& block = getData().Deck[deck_index];
        if ((block.GetStat() == 1) && !m_board.IsDraggingDeck(deck_index)) {
            block.Draw(block.GetPos());
        }
    }
}

// 戦闘画面全体の描画。常に呼び出す。
bool Battle::drawDefault() const
{
    if (is_gamewin)
    {
        // 背景をぼかすための処理
        { 
            const ScopedRenderTarget2D target(m_combatSceneBuffer);
            m_backgroundTexture.scaled(0.5).draw();
            m_banner.draw();
            drawHandCards();
            m_board.DrawBoard(0);
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
                m_attackIcon.scaled(1.6).draw(1330, 640);
                m_numFont(U"{}"_fmt(ene_attack)).draw(1410, 640, Palette::Black);
            }
            if (m_currentAnimState == BattleAnimationState::Idle || 
                m_currentAnimState == BattleAnimationState::CombatEnemyEffect ||
                m_currentAnimState == BattleAnimationState::CombatMyEffect ||
                m_currentAnimState == BattleAnimationState::DiscardEffect) {
                // 敵の防御アイコンの描画
                m_defenceIcon.scaled(1.5).draw(1330, 720);
                m_numFont(U"{}"_fmt(ene_defense)).draw(1410, 720, Palette::Black);
            }
            // 自分の防御アイコンの描画
            if (m_currentAnimState == BattleAnimationState::CombatEnemyEffect || 
                m_currentAnimState == BattleAnimationState::CombatMyEffect ||
                m_currentAnimState == BattleAnimationState::DiscardEffect){
                m_defenceIcon.scaled(1.5).draw(440, 720);
                m_numFont(U"{}"_fmt(my_defense)).draw(520, 720, Palette::Black);
            }
        }
        Shader::GaussianBlur(m_combatSceneBuffer, m_blurInternalBuffer, m_combatSceneBuffer, BoxFilterSize::BoxFilter13x13); 
        m_combatSceneBuffer.draw();
    }
    else
    {
        m_backgroundTexture.scaled(0.5).draw();
        m_banner.draw();
        if (is_deck) return true;
        drawHandCards();
        m_board.DrawBoard(0);
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
            m_attackIcon.scaled(1.6).draw(1330, 640);
            m_numFont(U"{}"_fmt(ene_attack)).draw(1410, 640, Palette::Black);
        }
        if (m_currentAnimState == BattleAnimationState::Idle || 
            m_currentAnimState == BattleAnimationState::CombatEnemyEffect ||
            m_currentAnimState == BattleAnimationState::CombatMyEffect ||
            m_currentAnimState == BattleAnimationState::DiscardEffect) {
            // 敵の防御アイコンの描画
            m_defenceIcon.scaled(1.5).draw(1330, 720);
            m_numFont(U"{}"_fmt(ene_defense)).draw(1410, 720, Palette::Black);
        }
        // 自分の防御アイコンの描画
        if (m_currentAnimState == BattleAnimationState::CombatEnemyEffect || 
            m_currentAnimState == BattleAnimationState::CombatMyEffect ||
            m_currentAnimState == BattleAnimationState::DiscardEffect){
            m_defenceIcon.scaled(1.5).draw(440, 720);
            m_numFont(U"{}"_fmt(my_defense)).draw(520, 720, Palette::Black);
        }
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
    // m_backgroundTexture.scaled(0.5).draw();
    // m_banner.draw();
    if (drawDefault()) return;
    // drawTableDeck();
    // drawDefault();
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
    m_board.DrawDraggedBlock();
}
