#include "Battle.hpp"
#include "Board.hpp" // BoardクラスのConfirm()などを使うためにインクルード
#include <tuple> // tie関数を使用するためにインクルード
using std::tie; // std::tieを使用するために名前空間を指定

// Constructor
Battle::Battle(const InitData& init)
	: IScene(init), 
    m_board(init._s->board), // GameDataからBoardを取得
    board_locked(false), // 盤面の操作を初期状態ではロックしない
    num_turn(0), // ターン数を初期化
    table_size(getTableSize()), // 手札のサイズを取得
    m_currentAnimState(BattleAnimationState::Idle) // アニメーション状態を初期化
{
	// --- 戦う敵のセットアップ ---
	setupEnemy();
	// --- デッキの初期化 ---
	// GameDataからマスターデッキを取得し、バトル用の山札にコピー
	Deck_yama = getData().Deck;
	Deck_yama.shuffle();

	// 最初の手札をセットアップ
	for (int i = 0; i < table_size; ++i)
	{
		if (Deck_yama.isEmpty()) break;
		Deck_table.push_back(Deck_yama.back());
		Deck_yama.pop_back();
	}
    drawCombatEffect(); // 
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
    // 未定
    // boardの現在どれくらいunlockされているマスがあるか。-> unlocked_num;
    // return num/2+2;
    return 0;
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
        // 敵にダメージを与える
        m_currentAnimState = BattleAnimationState::CombatEffect;
        // ここから未定
        m_currentAnimDuration = 1.0s; // アニメーションの時間を設定
        m_animStopwatch.restart(); // ストップウォッチをリセットして開始
	}
}


// 戦闘演出の更新処理
void Battle::updateCombatEffect()
{
    // 演出時間が経過したら、次の状態（例えばDiscardEffect）に遷移する
    if (m_animStopwatch > 1.0s)
    {
        // 敵にダメージを与える
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


        // 敵->プレイヤーの攻撃力を計算
        int32 my_real_attack = Min(0, my_attack - ene_defense); // プレイヤーの攻撃力から敵の防御力を引く
        m_enemy.hp -= my_real_attack; // プレイヤーのHPを減らす

        // プレイヤー->敵の攻撃力を計算
        int32 ene_real_attack = Min(0, ene_attack - my_defense); // 敵の攻撃力から防御力を引く
        getData().HP -= ene_real_attack; // 敵のHPを減らす
        if (is_boss3)
        {
            m_enemy.hp += my_real_attack;
        }
        m_currentAnimState = BattleAnimationState::DiscardEffect;
        m_animStopwatch.reset();
    }
}

// 捨て札アニメーション(盤面, 手札 -> 捨て札)の更新処理
void Battle::updateDiscardEffect()
{
	// アニメーションが完了したら
	if (m_animStopwatch > 1.0s)
	{
		// 1. Boardクラスの公開されているブロック配列から直接、捨て札に追加する
		for (const auto& block : Deck_board)
		{
			Deck_gomi.push_back(block);
		}
		// Boardに盤面をクリアするよう指示する
		// m_board.clearBoard();

        for (const auto& block : Deck_table)
        {
            // 手札のブロックを捨て札に移動
            Deck_gomi.push_back(block);
        }
        Deck_table.clear();

		// 2. 勝利判定を行う
		bool isVictory = m_enemy.hp <= 0;
		if (isVictory)
		{
			m_currentAnimState = BattleAnimationState::WinEffect;
			m_currentAnimDuration = 2.0s;
			m_animStopwatch.restart();
			return; // 勝利したので、以降の処理は行わない
		}

		// (敗北判定もここで行う)
		bool isLose = getData().HP <= 0; 
		if (isLose)
		{
			m_currentAnimState = BattleAnimationState::GameOver;
            m_animStopwatch.restart();
			return;
		}
		// 3. 次の状態（カードドロー）へ遷移する準備
		m_currentAnimState = BattleAnimationState::CardDrawEffect;
		m_currentAnimDuration = 0.6s; // 0.6秒かけてドロー
		m_animStopwatch.restart();
	}
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
    }
	// アニメーションが完了したら
	if (m_animStopwatch > 1.0s)
	{
		m_currentAnimState = BattleAnimationState::Idle;
        m_animStopwatch.reset(); // ストップウォッチをリセット
        board_locked = false; // 盤面の操作をアンロック
	}
}


void Battle::update()
{
	// 「=」ボタンの代わりのデバッグ操作
	if (KeyEnter.down() && !board_locked)
	{
		attack();
        return;
	}
    if (KeyS.down() && !board_locked)
    {
        // デッキの一覧を表示する。
        // showDeck(Deck_gomi);
        return;
    }
    // 現在の状態で処理を分岐
	switch (m_currentAnimState)
	{
	case BattleAnimationState::Idle:
        // ここにデッキと盤面の移動についての処理を記述する
        updateTableDeck();
		break;
	case BattleAnimationState::CombatEffect:
		updateCombatEffect();
		break;
	case BattleAnimationState::DiscardEffect:
		updateDiscardEffect();
		break;
	case BattleAnimationState::CardDrawEffect:
		updateCardDrawEffect();
		break;
	case BattleAnimationState::WinEffect:
		updateWinEffect();
        if (true) // 最後の勝利か
        {
            // ここで、勝利した敵を「倒した」状態にする
            m_enemyDB.markAsDefeated(m_enemy.name); // 敵を倒した状態に更新
            changeScene(State::Result); // リザルト画面へ遷移
        }   
        else{
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


void Battle::draw() const
{
	// ... (背景や手札、山札などの基本描画)

	// 現在の状態で描画処理を分岐
	switch (m_currentAnimState)
	{
    case BattleAnimationState::Idle:
        drawTableDeck();
        break;
    case BattleAnimationState::CombatEffect:
        drawCombatEffect();
        break;
	case BattleAnimationState::DiscardEffect:
		drawDiscardEffect();
		break;
	case BattleAnimationState::CardDrawEffect:
		drawCardDrawEffect();
		break;
	case BattleAnimationState::WinEffect:
		// TODO: 勝利演出の描画
		drawWinEffect();
		break;
	}
}

// 手札の描画
void drawTableDeck()
{
    // 
}

// 戦闘演出の描画
void Battle::drawCombatEffect() const
{
    // ここでは仮に攻撃・防御のエフェクトを描画する
    // 例えば、攻撃のエフェクトを表示するなど
    // 具体的な描画内容はゲームの仕様に依存する
    const Vec2 effectPos = Vec2{ 400, 300 }; // 仮の位置
    Circle(effectPos, 100).draw(ColorF(1.0, 0.5)); // 半透明の赤い円を描画
}

// 捨て札/カードドローアニメーションの描画
void Battle::drawDiscardEffect() const
{
    // 
}

void Battle::drawCardDrawEffect() const
{
	// アニメーション進捗 (0.0～1.0)
	const double progress = m_animStopwatch.sF() / m_currentAnimDuration.count();
	const double easedProgress = EaseOutCubic(progress); // 滑らかな動きにする
}