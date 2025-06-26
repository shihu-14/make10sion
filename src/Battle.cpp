#include "Battle.hpp"
#include "Board.hpp" // BoardクラスのConfirm()などを使うためにインクルード
#include <tuple> // tie関数を使用するためにインクルード
using std::tie; // std::tieを使用するために名前空間を指定

// Constructor
Battle::Battle(const InitData& init)
    : IScene(init),
    is_board_locked(true),
    now_turn(0), // ターン数を初期化// 手札のサイズを取得
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
    m_defenceIcon = Texture(U"../../image/icon_defence.png"); // 防御アイコンのテクスチャ
    m_button_hantei = Rect{ 1600, 750, 175, 100 }; // ボタンの位置とサイズを設定

    // init
    m_banner.init(getData().money, getData().Layer, getData().leric); // バナーの初期化
    m_board.InitAll();
    table_max_size = getTableSize();
    setupEnemy(getData().Layer >= 15);

    // --- デッキの初期化 ---
    for (int i = 0; i < deck_size; i++) {
        getData().Deck[i].SetStat(0);
        Deck_id.emplace_back(i);
    }
    Deck_yama = Deck_id; // 山札の初期化
    Deck_yama.shuffle();
}


void Battle::setupEnemy(bool is_boss)
{
    const EnemyData& data = m_enemyDB.getOneEnemy(is_boss);
    m_enemy.name = data.name;
    m_enemy.texture = Texture(data.texturePath);
    m_enemy.maxHp = data.maxHp;
    m_enemy.hp = data.maxHp;
    m_enemy.actionPattern = data.actionPattern;
}
// 山札に配置できる最大枚数を盤面の情報から求める関数
int32 Battle::getTableSize() const
{
    return Min(15, m_board.unlocked_num / 2 + 2);
}

// 0:山札, 1:手札, 2:盤面, -1:捨て札
// 盤面と山札のデッキの状況をリアルタイムで監視する関数
void Battle::updateTableDeck()
{
    // グローバルのDeckのstate変数を見て、盤面か手札かを参照し、Deck_tableとDeck_boardを更新する。
    for (int i = 0; i < deck_size; i++) {
        // 手札のブロックが盤面に移動している場合、盤面に追加する
        if (getData().Deck[i].GetStat() == 2 && Deck_board.includes(i) == false) {
            Deck_board.push_back(i);
        }
        // 盤面のブロックが手札に移動している場合、手札に追加する
        if (getData().Deck[i].GetStat() == 1 && Deck_table.includes(i) == false) {
            Deck_table.push_back(i);
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
        // 敵の攻撃・防御を取得
        ene_attack = m_enemy.actionPattern[now_turn % m_enemy.actionPattern.size()].attack;
        ene_defense = m_enemy.actionPattern[now_turn % m_enemy.actionPattern.size()].defense;
        // -------特殊攻撃--------
        if (ene_attack == -10) {
            ene_attack = 3 + 2 * (table_max_size - (int32)Deck_table.size());
        } else if (ene_attack == -11) {
            ene_attack = 20;
            now_turn++;
            num_turn_start = now_turn;
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
            getData().money -= 20;
        } else if (ene_attack == -17) {
            ene_attack = 80;
            is_boss3 = true; // ボス3の敵
        } else if (ene_attack == -18) {
            ene_attack = 2 + 3 * (table_max_size - (int32)Deck_table.size());
        } else if (ene_attack == -19) {
            ene_attack = 3 + 5 * (table_max_size - (int32)Deck_table.size());
        }
        // ------------------
        // 敵->プレイヤーの攻撃力を計算
        my_attack = 50; // for debug
        my_defense = 0; // for debug
        my_real_attack = Max(0, my_attack - ene_defense); // プレイヤーの攻撃力から敵の防御力を引く
        m_enemy.hp -= my_real_attack; // プレイヤーのHPを減らす

        // プレイヤー->敵の攻撃力を計算
        ene_real_attack = Max(0, ene_attack - my_defense); // 敵の攻撃力から防御力を引く
        getData().HP -= ene_real_attack; // 敵のHPを減らす
        // for debug
        // Print << U"Player Attack: " << my_real_attack << U", Enemy Attack: " << ene_real_attack << U"\n";
        // Print << U"Player HP: " << getData().HP << U", Enemy HP: " << m_enemy.hp << U"\n";
        // Print << U"Player Defense: " << my_defense << U", Enemy Defense: " << ene_defense << U"\n";

        if (is_boss3) {
            m_enemy.hp += my_real_attack;
        }
        // attack effectの演出のための制御変数を設定
        my_per_real_attack = my_real_attack / damage_effect_width;
        my_res_real_attack = my_real_attack % damage_effect_width;
        ene_per_real_attack = ene_real_attack / damage_effect_width;
        ene_res_real_attack = ene_real_attack % damage_effect_width;
        ene_damage_effect_cnt = 0;
        ene_damage_max_cnt = my_real_attack / damage_effect_width;
        my_damage_max_cnt = ene_real_attack / damage_effect_width;
        m_currentAnimState = BattleAnimationState::CombatEnemyEffect;
        m_animeStopwatch.restart(); // ストップウォッチをリセットして開始
    }
}

// 戦闘演出の更新処理
void Battle::updateCombatEnemyEffect()
{
    // damage_effectを表示するための制御
    if (ene_damage_effect_cnt < ene_damage_max_cnt) {
        if (m_animeStopwatch.sF() > 0.20 * ene_damage_effect_cnt) {
            if (ene_damage_effect_cnt == 0) ene_hpbar.damage(my_per_real_attack + my_res_real_attack); // 敵のHPバーを減らす
            else  ene_hpbar.damage(my_per_real_attack); // 敵のHPバーを減らす
            ene_effect_x = Random(1350, 1600); // エフェクトのX座標をランダムに設定
            ene_effect_y = Random(200, 450); // エフェクトのY座標をランダムに設定
            ene_damage_effect_cnt++;
            enemy_scale = 0.7;
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
    my_damage_effect_cnt = 0;
    enemy_scale = 0.85; // エフェクトの拡大を元に戻す
    m_currentAnimState = BattleAnimationState::CombatMyEffect;
    m_animeStopwatch.restart();
}

// 戦闘演出の更新処理
void Battle::updateCombatMyEffect()
{
    if (m_animeStopwatch.sF() < 0.8) {
        return;
    }
    // damage_effectを表示するための制御
    if (my_damage_effect_cnt < my_damage_max_cnt) {
        if ((m_animeStopwatch.sF() - 0.8) > 0.20 * my_damage_effect_cnt) {
            if (my_damage_effect_cnt == 0) my_hpbar.damage(ene_per_real_attack + ene_res_real_attack); // 自分のHPバーを減らす
            else my_hpbar.damage(ene_per_real_attack); // 自分のHPバーを減らす
            my_effect_x = Random(150, 300); // エフェクトのX座標をランダムに設定
            my_effect_y = Random(130, 230); // エフェクトのY座標をランダムに設定
            my_damage_effect_cnt++;
            my_angle = Random(-0.52, -0.1); // -π/4 ~ -π/6の範囲でプレイヤーを傾かさせる
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
        m_currentAnimState = BattleAnimationState::WinEffect;
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
            if (getData().Deck[Deck_table[table_id]].GetStat() != 1) {
                table_id--;
                return;
            } else {
                getData().Deck[Deck_table[table_id]].SetStat(-1);
                Deck_gomi.emplace_back(Deck_table[table_id]);
                Deck_table.erase(Deck_table.begin() + table_id); // 盤面から削除
                table_id--;
                sutehuda_angle = -90_deg; // 捨て札の角度を固定
                m_animeStopwatch.restart(); // ストップウォッチをリセット
                return;
            }
            sutehuda_angle = -90_deg;
            tehuda_rate = Min(1.0, m_animeStopwatch.sF() / 0.15); // 捨て札の位置を徐々に変える
            Vec2 from{ 200 + table_id * 100, 900 };
            Vec2 to{ 1600, 1100 };
            Vec2 pos = from.lerp(to, tehuda_rate);
            getData().Deck[Deck_table[table_id]].SetPos(pos.x, pos.y); // 手札
            if (tehuda_rate > 0.999999) {
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
            m_animeStopwatch.restart();
            yamahuda_angle += Scene::DeltaTime() * 4.0;
        } else {
            yamahuda_angle = 90_deg;
            tehuda_rate = Min(1.0, m_animeStopwatch.sF() / 0.3);
            Vec2 from{ 50, 900 };
            Vec2 to{ 350 + table_id * 75, 900 };
            Vec2 pos = from.lerp(to, tehuda_rate);
            getData().Deck[Deck_table[table_id]].SetPos(pos.x, pos.y); // 手札
            if (tehuda_rate > 0.99) {
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
}

// void Battle::updateWinEffect()
// {
//     // 勝利演出の更新処理
//     if (m_animeStopwatch.sF() < 3.0) // 1秒経過したら
//     {   
//         return;
//     }
// }

void Battle::update()
{
    is_deck = m_banner.update(getData().Deck);
    if (is_deck) return;
    if (m_button_hantei.mouseOver()) {
        Cursor::RequestStyle(CursorStyle::Hand);
    }
    if (m_button_hantei.leftClicked() && !is_board_locked) {
        attack();
        return;
    }
    for (int i = 0; i < Deck_table.size(); ++i) {
        if (getData().Deck.at(Deck_table[i]).IsDragging() && !is_board_locked) {
            m_board.PassBlock(getData().Deck[i], { getData().Deck[i].GetPos().first, getData().Deck[i].GetPos().second });
            return;
        }
        if (getData().Deck.at(Deck_table[i]).IsHovered() && !is_board_locked) {
            Cursor::RequestStyle(CursorStyle::Hand);
        }
    }
    my_hpbar.update(0.2);
    ene_hpbar.update(0.2);
    m_board.Update(is_result, getData().leric.getLeric());
    // m_banner.update(getData().Deck);
    // 現在の状態で処理を分岐
    switch (m_currentAnimState) {
    case BattleAnimationState::Idle:
        updateTableDeck();
        break;
    case BattleAnimationState::CombatEnemyEffect:
        enemy_scale = Min(0.85, enemy_scale + Scene::DeltaTime());
        updateCombatEnemyEffect();
        break;
    case BattleAnimationState::CombatMyEffect:
        my_angle = Min(0.0, my_angle + Scene::DeltaTime());
        updateCombatMyEffect();
        break;
    case BattleAnimationState::DiscardEffect:
        updateDiscardEffect();
        break;
    case BattleAnimationState::CardDrawEffect:
        updateCardDrawEffect();
        break;
    case BattleAnimationState::WinEffect:
        if (getData().Layer >= 30) // 最後の勝利か
        {
            changeScene(State::Result); // リザルト画面へ遷移
        } else {
            m_enemyDB.markAsDefeated(m_enemy.name);
            // まだ倒すべき敵が残っている場合 -> Mapシーンへ戻る
            changeScene(State::Map);
        }
        break;
    case BattleAnimationState::GameOver:
        // ゲームオーバーからリザルド画面へ戻る処理
        changeScene(State::Result);
        break;
    }
}

// 手札を描画
void Battle::drawTableDeck() const
{
    for (const auto& block : getData().Deck) {
        if (block.GetStat() == 1) // 手札の状態
        {
            block.Draw(block.GetPos());
        }
    }
}

// 戦闘画面全体の描画。常に呼び出す。
void Battle::drawDefault() const
{
    m_board.DrawBoard(0);
    // プレイヤーのキャラクターを描画
    m_myTexture.scaled(0.75).rotated(my_angle).draw(180, 230);
    // 敵の情報を描画
    m_enemy.texture.scaled(enemy_scale).draw(1450, 350);
    // 山札のテクスチャを描画
    m_yamahudaTexture.scaled(0.75).rotated(yamahuda_angle).draw(50, 800);
    // 捨て札のテクスチャを描画
    m_sutehudaTexture.scaled(0.6).rotated(sutehuda_angle).draw(1600, 880);
    // =buttonのテクスチャを描画
    m_buttonTexture.scaled(0.7).draw(1600, 750);
    // m_button_hantei.draw(Palette::Red); // for debug
    my_hpbar.draw(RectF{ 130, 700, 320, 20 });
    ene_hpbar.draw(RectF{ 1480, 700, 320, 20 });
}

// 戦闘演出の描画
void Battle::drawCombatEnemyEffect() const
{
    if (ene_effect_x != -1 && ene_effect_y != -1) {
        m_effectTexture.scaled(0.5).draw(ene_effect_x, ene_effect_y);
    }
}
void Battle::drawCombatMyEffect() const
{
    if (my_effect_x != -1 && my_effect_y != -1) {
        m_effectTexture.scaled(0.5).draw(my_effect_x, my_effect_y);
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
    return;
    // 
}


void Battle::draw() const
{
    m_backgroundTexture.scaled(0.5).draw();
    m_banner.draw();
    if (is_deck) return;
    drawTableDeck();
    drawDefault();
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
        // drawWinEffect();
        break;
    case BattleAnimationState::GameOver:
        // ゲームオーバーの描画処理
        break;
    }
}

