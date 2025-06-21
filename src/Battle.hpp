#ifndef BATTLE_HPP
#define BATTLE_HPP

# include <Siv3D.hpp> // Siv3Dの主要ヘッダ
# include "Block.hpp" // Block クラスの定義があるヘッダファイルをインクルード
# include "common.hpp"
# include "Board.hpp" // Board クラスの定義があるヘッダファイルをインクルード
# include "Enemy.hpp" // Enemy クラスの定義があるヘッダファイルをインクルード

// Data Manager の Deck を模倣したグローバル変数
// 実際には Data Manager クラス (DataManager.hpp) で定義し、ここからインクルードするのが望ましい

class Battle : public App::Scene
{
private:


	Board m_board; // 盤面の状態を管理する Board クラスのインスタンス
	bool board_locked = false; // 盤面の操作がロックされているかどうか
	int32 num_turn = 0; // ターン数
	Enemy m_enemyDB;

	enum class BattleAnimationState
	{
		Idle,               // 何もアニメーションしていない状態
		CombatEffect,       // 攻撃・防御演出中
		DiscardEffect,      // ブロックが捨て札に行く演出中
		CardDrawEffect,     // カードドロー演出中
		WinEffect,      // 勝利演出中
		GameOver    // ゲームオーバー表示中
	};
	// 現在のアニメーション状態
	BattleAnimationState m_currentAnimState = BattleAnimationState::Idle;

	// ゲームの状態に関するメンバー変数
    // Battle Class では盤面に関すること以外のすべてを実装する。
    // 具体例としては、手札の管理、各キャラクターのHP、DFの管理、ターンの管理、
    // アニメーションの再生、エフェクトの描画など 。
	Stopwatch m_animStopwatch; // 各アニメーションの時間を計測
	Duration m_currentAnimDuration; // 現在のアニメーションの全体時間

	
	// Battle Sceneでは、Data ManagerのDeck(vector<Block>)をコピーして使用状況を管理する
	Array<Block> Deck_yama;     // 山札 (元のGlobalDeckのコピー) 
	Array<Block> Deck_table;   // 手札 
	Array<Block> Deck_gomi;     // 捨て札

	// Board Class が管理するとされる盤面の情報 (Battle Class 内では参照・操作に使う)
	// Board Classは、盤面の状態を管理し、ブロックの配置、回転、削除、盤面の描画などを行う 。
	// ここでは RectF の配列で仮定。
	int getTableSize() const;
	void attack();
	// 各演出の更新関数 (private)
    // これらはupdate()から呼ばれ、アニメーションの状態を更新し、完了時に次の状態へ遷移させる
	void updateCombatEffect();
	void updateDiscardEffect();
	void updateCardDrawEffect();
	void updateWinEffect();

	// 各演出の描画関数 (private, const)
    // これらはdraw()から呼ばれ、現在のアニメーション状態に基づいて描画を行う
	void drawCombatEffect() const;
	void drawDiscardEffect() const;
	void drawCardDrawEffect() const;
	void drawWinEffect() const;


public:
	
	Battle(const InitData& init);
		
	~Battle(); 
	
	void update() override;
	
	void draw() const override;
};

#endif