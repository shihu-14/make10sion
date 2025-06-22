#include "Battle.hpp"
#include "Board.hpp" // BoardクラスのConfirm()などを使うためにインクルード
#include <tuple> // tie関数を使用するためにインクルード
using std::tie; // std::tieを使用するために名前空間を指定


// -------------------------ここからBattleクラスの実装
// Constructor
Battle::Battle(const InitData& init)
	: IScene(init), 
    board_locked(false), // 盤面の操作を初期状態ではロックしない
    num_turn(0), // ターン数を初期化
    deck_width(15), // ターン数を初期化
    table_size(getTableSize()), // 手札のサイズを取得
    m_currentAnimState(BattleAnimationState::Idle) // アニメーション状態を初期化
{
    m_backgroundTexture = Texture(U"../../image/haikei_sentou.png"); // 背景画像のパスを指定
    m_myTexture = Texture(U"../../image/chara_player.png"); // 自分のカードのテクスチャ
    m_yamahudaTexture = Texture(U"../../image/yamahuda.png"); // 山札のテクスチャ
    m_sutehudaTexture = Texture(U"../../image/sutehuda.png"); // 捨て札のテクスチャ
    m_buttonTexture = Texture(U"../../image/bottun_equal.png"); // ボタンのテクスチャ
    m_effectTexture = Texture(U"../../image/effect_attack.png"); // エフェクトのテクスチャ
    m_attackIcon = Texture(U"../../image/icon_attack.png"); // 攻撃アイコンのテクスチャ
    m_defenceIcon = Texture(U"../../image/icon_defence.png"); // 防御アイコンのテクスチャ
    m_button_hantei = Rect{1300, 400, 200, 100}; // ボタンの位置とサイズを設定

    // init
    m_banner.init(getData().money, getData().Layer, getData().leric); // バナーの初期化
    // m_board.InitAll();
	// --- 戦う敵のセットアップ ---
	setupEnemy();
	// --- デッキの初期化 ---
	// GameDataからマスターデッキを取得し、バトル用の山札にコピー
	Deck_yama = getData().Deck;
	Deck_yama.shuffle();
	// // 最初の手札をセットアップ
	// for (int i = 0; i < table_size; ++i)
	// {
	// 	if (Deck_yama.isEmpty()) break;
	// 	Deck_table.push_back(Deck_yama.back());
	// 	Deck_yama.pop_back();
    //     Deck_table.back().SetStat(1); // 手札のステータスを1に設定
    //     // Edit here (座標)
    //     Deck_table.back().SetPos(300+i*50*deck_width, 600); // 手札の位置を設定
    //     // m_tehuda_hantei.emplace_back(300+i*50*deck_width, 600, deck_width, 100); 
	// }
    updateCardDrawEffect();
}


void Battle::setupEnemy()
{
	// 1. データベースから、まだ倒されていない敵の「設計図」を取得します。
	//    getOneEnemy() の内部で、倒されていない敵を選ぶロジックが実行されます。
	const EnemyData& data = m_enemyDB.getOneEnemy(false);
	// 2. 戦闘用の変数(m_enemy)に、設計図の情報をすべてコピーして初期化します。
	//    これにより、マスターデータを汚さずに済みます。
	m_enemy.name = data.name;
	m_enemy.texture = Texture(data.texturePath);
	m_enemy.maxHp = data.maxHp;
	m_enemy.hp = data.maxHp;
	m_enemy.actionPattern = data.actionPattern; // 行動パターンもコピーします
}
// 山札の枚数を盤面の情報から求める関数
int32 Battle::getTableSize() const
{
    // return 6;
    return Min(deck_width, m_board.unlocked_num/2+2);
}

// 盤面のデッキの状況をリアルタイムで監視する関数
void Battle::updateTableDeck()
{
    // 0:山札, 1:手札, 2:盤面, -1:捨て札
    // グローバルのDeckのstate変数を見て、盤面か手札かを参照し、Deck_tableとDeck_boardを更新する。
    for (const auto& block : getData().Deck)
    {
        if (block.GetStat() == 2 && Deck_board.includes(block) == false)
        {
            // 手札のブロックが盤面に移動している場合、盤面に追加する
            Deck_board.push_back(block);
        }
        if (block.GetStat() == 1 && Deck_table.includes(block) == false)
        {
            // 盤面のブロックが手札に移動している場合、手札に追加する
            Deck_table.push_back(block);
        }
    }
}

// 「=」ボタンが押された時に呼び出される
void Battle::attack()
{
	// 現在アニメーション中でない場合のみ処理を開始
	if (m_currentAnimState == BattleAnimationState::Idle)
	{
        // 盤面の操作をロックする
        board_locked = true; // 盤面の操作をロック

        // 攻撃・防御の処理を行う
        // 盤面から攻撃力と防御力を取得
         // BoardクラスのConfirm()を呼び出して攻撃力と防御力を取得
        tie(my_attack, my_defense) = m_board.Confirm();
        // 敵の攻撃・防御を取得
        ene_attack = m_enemy.actionPattern[num_turn % m_enemy.actionPattern.size()].attack;
        ene_defense = m_enemy.actionPattern[num_turn % m_enemy.actionPattern.size()].defense;

        // -------特殊攻撃--------
        if (ene_attack == -10)
        {
            ene_attack = 3+2*(table_size-Deck_table.size());
        }
        else if (ene_attack == -11)
        {
            ene_attack = 20;
            num_turn++;
            num_turn_start = num_turn;
        }
        else if (ene_attack == -12)
        {
            ene_attack = 60-4*(table_size-Deck_table.size());
        }
        else if (ene_attack == -13)
        {
            ene_attack = 40;
            getData().money -= 30;
        }
        else if (ene_attack == -14)
        {
            flag_exit = true;
            // 逃走の処理は保留
        }
        else if (ene_attack == -15)
        {
            ene_attack = 10+14*(table_size-Deck_table.size());
        }
        else if (ene_attack == -16)
        {
            ene_attack = 30;
            getData().money -= 20;
        }
        else if (ene_attack == -17)
        {
            ene_attack = 80;    
            is_boss3 = true; // ボス3の敵
        }
        else if (ene_attack == -18)
        {
            ene_attack = 2+3*(table_size-Deck_table.size());
        }
        else if (ene_attack == -19)
        {
            ene_attack = 3+5*(table_size-Deck_table.size());
        }
        // ------------------

        // 敵->プレイヤーの攻撃力を計算
        my_real_attack = Min(0, my_attack - ene_defense); // プレイヤーの攻撃力から敵の防御力を引く
        m_enemy.hp -= my_real_attack; // プレイヤーのHPを減らす

        // プレイヤー->敵の攻撃力を計算
        ene_real_attack = Min(0, ene_attack - my_defense); // 敵の攻撃力から防御力を引く
        getData().HP -= ene_real_attack; // 敵のHPを減らす
        if (is_boss3)
        {
            m_enemy.hp += my_real_attack;
        }
        my_damage_max_cnt = ene_real_attack/10;
        ene_damage_max_cnt = my_real_attack/10;
        m_currentAnimState = BattleAnimationState::CombatEnemyEffect;
        // m_currentAnimDuration = 1.0s; // アニメーションの時間を設定
        m_animeStopwatch.restart(); // ストップウォッチをリセットして開始
	}
}

// 戦闘演出の更新処理
void Battle::updateCombatEnemyEffect()
{
    if (ene_damage_effect_cnt < ene_damage_max_cnt)
    {
        if (m_animeStopwatch.sF() > 0.25*ene_damage_effect_cnt)
        {
            if (ene_damage_effect_cnt == 0)
            {
                ene_hpbar.damage(my_real_attack);
            }
            ene_effect_x = Random(1350, 1600); // エフェクトのX座標をランダムに設定
            ene_effect_y = Random(200, 450); // エフェクトのY座標をランダムに設定
            ene_damage_effect_cnt++;
            enemy_scale = 0.7;
        }
        return;
    }
    // 演出時間が経過したら、次の状態（例えばDiscardEffect）に遷移する
    // 敵にダメージを与える
    bool isLose = getData().HP <= 0; 
    if (isLose)
    {
        is_result = true;
        m_currentAnimState = BattleAnimationState::GameOver;
        m_animeStopwatch.restart();
        return;
    }
    enemy_scale = 0.85; // エフェクトの拡大を元に戻す
    m_currentAnimState = BattleAnimationState::CombatMyEffect;
    m_animeStopwatch.reset();
}

// 戦闘演出の更新処理
void Battle::updateCombatMyEffect()
{
    if (my_damage_effect_cnt < my_damage_max_cnt)
    {
        if (m_animeStopwatch.sF() > 0.25*my_damage_effect_cnt)
        {
            if (my_damage_effect_cnt == 0)
            {
                my_hpbar.damage(my_real_attack);
            }
            my_effect_x = Random(150, 300); // エフェクトのX座標をランダムに設定
            my_effect_y = Random(130, 230); // エフェクトのY座標をランダムに設定
            my_damage_effect_cnt++;
            my_angle = Random(-0.6, -0.1);
        }
        return;
    }
    // 2. 勝利判定を行う
    bool isVictory = m_enemy.hp <= 0;
    if (isVictory)
    {
        if (false) // 最後のボスか
        {
            is_result = true;
        }
        m_currentAnimState = BattleAnimationState::WinEffect;
        // m_currentAnimDuration = 2.0s;
        m_animeStopwatch.restart();
        return; // 勝利したので、以降の処理は行わない
    }
    my_angle = 0.0;
    table_id = Deck_table.size()-1;
    m_currentAnimState = BattleAnimationState::DiscardEffect;
    m_animeStopwatch.reset();
}

// 捨て札アニメーション(盤面, 手札 -> 捨て札)の更新処理
void Battle::updateDiscardEffect()
{
    flag_once_draw++;
	// アニメーションが完了したら
	if (table_id < Deck_table.size())
	{
        if (sutehuda_angle > -90_deg)
        {
            sutehuda_angle -= Scene::DeltaTime()*3.5; // 捨て札の角度を徐々に変える
        }
        else
        {
            sutehuda_angle = -90_deg; // 捨て札の角度を固定
            tehuda_rate = Min(1.0, m_animeStopwatch.sF()/0.5); // 捨て札の位置を徐々に変える
            if (tehuda_rate > 0.99)
            {
                // 捨て札に移動したから、stateを変更。Deck_gomiに追加する
                Deck_gomi.emplace_back(Deck_table[table_id]); // 手札のブロックを捨て札に移動
                Deck_table[table_id].SetStat(-1); // ブロックのステータスを捨て札に設定
                table_id--;
                m_animeStopwatch.reset(); // ストップウォッチをリセット
                tehuda_rate = 0; // 捨て札の位置を固定
            }
        }
    }
    if (sutehuda_angle < 0.0)
    {
        sutehuda_angle += Scene::DeltaTime()*5.5; // 山札の角度を徐々に戻す
        return;
    }
    // 1. Boardクラスの公開されているブロック配列から直接、捨て札に追加する
    for (auto& block : Deck_board)
    {
        block.SetStat(-1); // ブロックのステータスを捨て札に設定
        Deck_gomi.push_back(block);
    }
    // Boardに盤面をクリアするよう指示する
    // m_board.clearBoard();
    // for (auto& block : Deck_table)
    // {
    //     // 手札のブロックを捨て札に移動
    //     block.SetStat(-1); // ブロックのステータスを捨て札に設定
    //     Deck_gomi.push_back(block);
    // }
    Deck_table.clear();
    // (敗北判定もここで行う)
    
    // 3. 次の状態（カードドロー）へ遷移する準備
    table_id = 0;
    flag_once_draw = 0;
    m_currentAnimState = BattleAnimationState::CardDrawEffect;
    // m_currentAnimDuration = 0.6s; // 0.6秒かけてドロー
    m_animeStopwatch.restart();
}


// カードドローアニメーションの更新処理
void Battle::updateCardDrawEffect()
{
    // 手札を補充する枚数だけ、山札からアニメーションリストへ移す
    for (int i = 0; i < table_size; ++i)
    {
        // 山札が空なら、捨て札をシャッフルして戻す (reshuffle関数があるとより良い)
        if (Deck_yama.empty()) break;
        // 山札からカードを1枚引く
        Block card = Deck_yama.back();
        Deck_yama.pop_back(); // 山札から削除
        Deck_table.push_back(card); // 手札に追加
        Deck_table.back().SetStat(1); // 手札のステータスを1に設定
        Deck_table.back().SetPos(300 + i*deck_width, 500); //
    }
    if (table_id < Deck_table.size())
	{
        if (yamahuda_angle < 90_deg)
        {
            yamahuda_angle += Scene::DeltaTime()*3.5; // 捨て札の角度を徐々に変える
        }
        else
        {
            yamahuda_angle = 90_deg;
            tehuda_rate = Min(1.0, m_animeStopwatch.sF()/0.5); // 捨て札の位置を徐々に変える
            if (tehuda_rate > 0.99)
            {
                // 捨て札に移動したから、stateを変更。Deck_gomiに追加する
                Deck_table[table_id].SetStat(1); // ブロックのステータスを捨て札に設定
                table_id++;
                m_animeStopwatch.reset(); // ストップウォッチをリセット
                tehuda_rate = 0; // 捨て札の位置を固定
            }
        }
        return;
    }
    if (yamahuda_angle > 0.0)
    {
        yamahuda_angle -= Scene::DeltaTime()*5.5; // 山札の角度を徐々に戻す
        return;
    }
    m_board.InitAll();
    m_currentAnimState = BattleAnimationState::Idle;
    m_animeStopwatch.reset(); // ストップウォッチをリセット
    board_locked = false; // 盤面の操作をアンロック
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
	// 「=」ボタンの代わりのデバッグ操作
	if (m_button_hantei.leftClicked() && !board_locked)
	{
		attack();
        return;
	}
    // if (KeyS.down() && !board_locked)
    // {
    //     m_deck.draw();
    //     return;
    // }
    for (int i = 0; i < Deck_table.size(); ++i)
    {
        if (m_tehuda_hantei[i].leftClicked() && !board_locked)
        {
            // Edit here
            m_board.PassBlock(Deck_table[i], {Deck_table[i].GetPos().first, Deck_table[i].GetPos().second}); // 手札のブロックを盤面に移動
            return; // 一度のクリックで一つのブロックのみ処理する
        }
    }
    my_hpbar.update(0.1);
    ene_hpbar.update(0.1);
    m_board.Update(is_result, getData().leric.getLeric());
    m_banner.update(getData().Deck);
    // 現在の状態で処理を分岐
	switch (m_currentAnimState)
	{
	case BattleAnimationState::Idle:
        // ここにデッキと盤面の移動についての処理を記述する
        updateTableDeck();
		break;
	case BattleAnimationState::CombatEnemyEffect:
        enemy_scale = Min(0.85, enemy_scale + Scene::DeltaTime()); // 敵のエフェクトの拡大
		updateCombatEnemyEffect();
		break;
    case BattleAnimationState::CombatMyEffect:
        my_angle = Min(0.0, my_angle + Scene::DeltaTime()); // 敵のエフェクトの拡大
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
            // ここで、勝利した敵を「倒した」状態にする
            changeScene(State::Result); // リザルト画面へ遷移
        }   
        else{
            m_enemyDB.markAsDefeated(m_enemy.name); // 敵を倒した状態に更新
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

// 手札の描画
void Battle::drawTableDeck() const
{
    for (const auto& block: getData().Deck)
    {
        if (block.GetStat() == 1) // 手札の状態
        {
            auto [x, y] = block.GetPos(); // ブロックの位置を取得
            block.Draw({x, y}); // BlockクラスにDrawメソッドがあると仮定
        }
    }
}

void Battle::drawDefault() const
{
    // 盤面の描画
    // 盤面の背景を描画
    // m_backgroundTexture.scaled(0.5).draw();
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
    my_hpbar.draw(RectF{130, 700, 320, 20});
    ene_hpbar.draw(RectF{1480, 700, 320, 20});
}

// 戦闘演出の描画
void Battle::drawCombatEnemyEffect() const
{
    if (ene_effect_x != -1 && ene_effect_y != -1)
    {
        m_effectTexture.scaled(0.5).draw(ene_effect_x, ene_effect_y);
    }
}
void Battle::drawCombatMyEffect() const
{
    if (my_effect_x != -1 && my_effect_y != -1)
    {
        m_effectTexture.scaled(0.5).draw(my_effect_x, my_effect_y);
    }
}

// 捨て札/カードドローアニメーションの描画
void Battle::drawDiscardEffect() const
{
    if (flag_once_draw == 0)
    {
        // Edit here
        // m_board.Discard();
    }
    auto [sx, sy] = Deck_table[table_id].GetPos();
    Vec2 pos = Vec2{sx, sy}.lerp(Vec2{1560, 750}, tehuda_rate); // 手札の位置を取得
    Deck_table[table_id].Draw({(int32)pos.x, (int32)pos.y}, 1.0, sutehuda_angle); 
    
}

void Battle::drawCardDrawEffect() const
{
    auto [sx, sy] = Deck_table[table_id].GetPos();
    Vec2 pos = Vec2{sx, sy}.lerp(Vec2{1560, 750}, tehuda_rate); // 手札の位置を取得
    Deck_table[table_id].Draw({pos.x, pos.y}, 1.0, yamahuda_angle); 
}


void Battle::draw() const
{
    m_backgroundTexture.scaled(0.5).draw();
    m_banner.draw();
    if (is_deck) return;
	// ... (背景や手札、山札などの基本描画)
    drawDefault();
    drawTableDeck();
	// 現在の状態で描画処理を分岐
	switch (m_currentAnimState)
	{
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
		// TODO: 勝利演出の描画
		// drawWinEffect();
		break;
	}
}

