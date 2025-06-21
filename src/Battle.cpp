#include "Battle.hpp"
#include "Board.hpp" // BoardクラスのConfirm()などを使うためにインクルード

// ... (Constructorなど)


// 山札の枚数を盤面の情報から求める関数
int Battle::getTableSize() const
{
    // 未定
    return 0;
}

// 「=」ボタンが押された時に呼び出される
void Battle::attack()
{
	// 現在アニメーション中でない場合のみ処理を開始
	if (m_currentAnimState != BattleAnimationState::Idle)
	{
		return;
	}
}

void Battle::update()
{
	// 「=」ボタンの代わりのデバッグ操作
	if (KeyEnter.down() && !board_locked)
	{
        // 盤面の操作を不能にする
        // SetStat(false);
        board_locked = true; // 盤面の操作をロック
        num_turn++;
		attack();
	}
    // 現在の状態で処理を分岐
	switch (m_currentAnimState)
	{
	case BattleAnimationState::Idle:
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
		break;
	case BattleAnimationState::GameOver:
        // ゲームオーバーから戻る処理
        changeScene(State::Title);
		break;
	}
}

// 戦闘演出の更新処理
void Battle::updateCombatEffect()
{
    // 演出時間が経過したら、次の状態（例えばDiscardEffect）に遷移する
    // ここでは仮にIdleに戻す
    if (m_animStopwatch > 0.5s)
    {
        m_currentAnimState = BattleAnimationState::Idle;
        m_animStopwatch.reset();
    }
}

// 捨て札アニメーション(盤面, 手札 -> 捨て札)の更新処理
void Battle::updateDiscardEffect()
{
	// アニメーションが完了したら
	if (m_animStopwatch > m_currentAnimDuration)
	{
		// 1. Boardクラスの公開されているブロック配列から直接、捨て札に追加する
		for (const auto& block : m_board.placedBlocks)
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
        Deck_table.clear(); // 手札をクリア

		// 2. 勝利判定を行う
		bool isVictory = false; // (例: 敵のHP <= 0)
		if (isVictory)
		{
			m_currentAnimState = BattleAnimationState::WinEffect;
			m_currentAnimDuration = 2.0s;
			m_animStopwatch.restart();
			return; // 勝利したので、以降の処理は行わない
		}

		// (敗北判定もここで行う)
		bool isLose = false; // (例: 自分のHP <= 0)
		if (isLose)
		{
			m_currentAnimState = BattleAnimationState::GameOver;
			// ゲームオーバー演出は即時開始するため、タイマー設定は不要な場合もある
            m_animStopwatch.restart();
            // ここでゲームオーバーの処理を行う
            // 例えば、HPをリセットしたり、タイトル画面に戻る準備をする
            // ゲームオーバー画面へ遷移する
            changeScene(State::Title); // タイトル画面へ戻る
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
    for (int i = 0; i < getTableSize(); ++i)
    {
        // 山札が空なら、捨て札をシャッフルして戻す (reshuffle関数があるとより良い)
        if (Deck_yama.empty()) break;
        // 山札からカードを1枚引く
        Block card = Deck_yama.back();
        Deck_yama.pop_back(); // 山札から削除
        Deck_table.push_back(card); // 手札に追加
    }
	// アニメーションが完了したら
	if (m_animStopwatch > m_currentAnimDuration)
	{
		m_currentAnimState = BattleAnimationState::Idle;
	}
}



void Battle::draw() const
{
	// ... (背景や手札、山札などの基本描画)

	// 現在の状態で描画処理を分岐
	switch (m_currentAnimState)
	{
    case BattleAnimationState::Idle:
        // 通常状態
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