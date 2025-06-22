#ifndef BATTLE_HPP
#define BATTLE_HPP

# include "Block.hpp" // Block クラスの定義があるヘッダファイルをインクルード
# include "common.hpp"
# include "Board.hpp" // Board クラスの定義があるヘッダファイルをインクルード
# include "Enemy.hpp" // Enemy クラスの定義があるヘッダファイルをインクルード
# include "Banner.hpp" // Enemy クラスの定義があるヘッダファイルをインクルード
# include "HPbar.cpp" // Enemy クラスの定義があるヘッダファイルをインクルード

// Data Manager の Deck を模倣したグローバル変数
// 実際には Data Manager クラス (DataManager.hpp) で定義し、ここからインクルードするのが望ましい

struct EnemyState
{
	String name;
	Texture texture;
	int32 hp = 0;
	int32 maxHp = 0;
	Array<EnemyAction> actionPattern; // 現在のターンで敵が何をするか
};

class Battle : public App::Scene
{
private:

	Board m_board; // 盤面の状態を管理する Board クラスのインスタンス
	Banner m_banner; // バナーの表示を管理する Banner クラスのインスタンス
	bool is_result = false;
	bool board_locked = false; // 盤面の操作がロックされているかどうか
	bool flag_exit = false; // 敵が逃走するか
	bool is_boss3 = false;
	int32 num_turn = 0; // ターン数
	int32 num_turn_start = -1; // 攻撃/防御のパターンの変化を管理(基本的には0のまま)
	int32 table_size; // 手札のサイズ
	Enemy m_enemyDB;
	EnemyState m_enemy; // 現在の敵の状態を保持する
	// 攻撃・防御の情報を保持するための変数
	int32 my_attack = 0;
	int32 my_defense = 0;
	int32 my_real_attack = 0;
	int32 ene_attack = 0; 
	int32 ene_defense = 0;
	int32 ene_real_attack = 0;
	int32 global_id = 0;
	int32 global_tmp_max = 0;


	enum class BattleAnimationState
	{
		Idle,               // 何もアニメーションしていない状態
		CombatEnemyEffect,       // 攻撃・防御演出中
		CombatMyEffect,       // 攻撃・防御演出中
		DiscardEffect,      // ブロックが捨て札に行く演出中
		CardDrawEffect,     // カードドロー演出中
		WinEffect,      // 勝利演出中
		GameOver    // ゲームオーバー表示中
	};
	// 現在のアニメーション状態
	BattleAnimationState m_currentAnimState = BattleAnimationState::Idle;

    // アニメーションの再生、エフェクトの描画など 。
	Stopwatch m_animeStopwatch; // 各アニメーションの時間を計測
	Duration m_currentAnimDuration; // 現在のアニメーションの全体時間

	
	// Battle Sceneでは、Data ManagerのDeck(vector<Block>)をコピーして使用状況を管理する
	// Array<Block> Deck_yama;     // 山札 (元のGlobalDeckのコピー) 
	// Array<Block> Deck_table;   // 手札 
	// Array<Block> Deck_gomi;     // 捨て札
	// Array<Block> Deck_board;
	// Array<Block> Deck;


	// Texture 
	Texture m_backgroundTexture; // 背景画像
	Texture m_myTexture; // カードのテクスチャ
	Texture m_yamahudaTexture; // 山札のテクスチャ
	Texture m_sutehudaTexture; // 手札のテクスチャ
	Texture m_buttonTexture; // ボタンのテクスチャ
	Texture m_effectTexture; // エフェクトのテクスチャ
	HPBar my_hpbar; // HPバーのインスタンス
	HPBar ene_hpbar; // HPバーのインスタンス
	Texture m_attackIcon;
	Texture m_defenceIcon;
	Rect m_button_hantei;
	Array<Rect> m_tehuda_hantei;

	double enemy_scale = 0.85;
	double my_angle = 0.0;
	int32 deck_width = 15;
	int32 my_damage_effect_cnt = 0;
	int32 ene_damage_effect_cnt = 0;
	int32 my_damage_max_cnt = 0;
	int32 ene_damage_max_cnt = 0;
	int32 my_effect_x = -1, my_effect_y = -1; // エフェクトの位置
	int32 ene_effect_x = -1, ene_effect_y = -1; // エフェクトの位置


	bool is_deck = false;
	int32 flag_once_draw = 0;
	double yamahuda_angle = 0.0;
	double sutehuda_angle = 0.0;
	// int32 table_id = 0;
	double tehuda_rate = 0.0;
	double tehuda_angle = 0.0;
	// Board Class が管理するとされる盤面の情報 (Battle Class 内では参照・操作に使う)
	// Board Classは、盤面の状態を管理し、ブロックの配置、回転、削除、盤面の描画などを行う 。
	// ここでは RectF の配列で仮定。
	int32 getTableSize() const;
	void setupEnemy();
	void updateTableDeck();
	void attack();
	// 各演出の更新関数 (private)
    // これらはupdate()から呼ばれ、アニメーションの状態を更新し、完了時に次の状態へ遷移させる
	void updateCombatEnemyEffect();
	void updateCombatMyEffect();
	void updateDiscardEffect();
	void updateCardDrawEffect();
	void updateWinEffect();

	// 各演出の描画関数 (private, const)
    // これらはdraw()から呼ばれ、現在のアニメーション状態に基づいて描画を行う
	void drawTableDeck() const;
	void drawDefault() const;
	void drawCombatEnemyEffect() const;
	void drawCombatMyEffect() const;
	void drawDiscardEffect() const;
	void drawCardDrawEffect() const;
	void drawWinEffect() const;


public:
	
	Battle(const InitData& init);
		
	// ~Battle(); 
	
	void update() override;
	
	void draw() const override;
};

#endif