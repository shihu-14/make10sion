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
	bool is_result = false; // result画面にいるかどうか
	bool is_deck = false; // デッキ画面にいるかどうか
	bool is_board_locked = false; // 盤面の操作がロックされているかどうか
	bool is_exit = false; // 敵が逃走するか
	bool is_boss3 = false;
	int32 now_turn = 0; // ターン数
	int32 num_turn_start = -1; // 攻撃/防御のパターンの変化を管理(基本的には0のまま)
	int32 table_max_size; // 手札のサイズ
	int32 deck_size = 0;
	Enemy m_enemyDB;
	EnemyState m_enemy; // 現在の敵の状態を保持する
	// 攻撃・防御の情報を保持するための変数
	int32 my_attack = 0;
	int32 my_defense = 0;
	int32 my_real_attack = 0;
	int32 ene_attack = 0;
	int32 ene_defense = 0;
	int32 ene_real_attack = 0;
	int32 my_per_real_attack = 0;
	int32 my_res_real_attack = 0;
	int32 ene_per_real_attack = 0;
	int32 ene_res_real_attack = 0;


	enum class BattleAnimationState
	{
		Idle,               // 何もアニメーションしていない状態
		CombatEnemyEffect,       // 敵への攻撃・防御演出中
		CombatMyEffect,       // 自分への攻撃・防御演出中
		DiscardEffect,      // 手札が捨て札に行く演出中
		CardDrawEffect,     // 山札からのカードドロー演出中
		WinEffect,      // 勝利演出中
		GameOver    // ゲームオーバー表示中
	};
	// 現在のアニメーション状態
	BattleAnimationState m_currentAnimState = BattleAnimationState::Idle;

	// アニメーションの時間を制御するための変数
	Stopwatch m_animeStopwatch;
	Duration m_currentAnimDuration;


	// デッキの状態を管理する変数
	Array<int> Deck_id; // grobalのdeckの配列indexを管理
	Array<int> Deck_yama;     // 山札 (元のGlobalDeckのコピー) 
	Array<int> Deck_table;   // 手札 
	Array<int> Deck_gomi;     // 捨て札
	Array<int> Deck_board;

	// Texture 
	Texture m_backgroundTexture; // 背景画像
	Texture m_myTexture; // プレイヤーのテクスチャ
	Texture m_yamahudaTexture; // 山札のテクスチャ
	Texture m_sutehudaTexture; // 捨て札のテクスチャ
	Texture m_buttonTexture; // =ボタンのテクスチャ
	Texture m_effectTexture; // 攻撃エフェクトのテクスチャ
	HPBar my_hpbar; // 自分のHPバーのインスタンス
	HPBar ene_hpbar; // 敵のHPバーのインスタンス
	Texture m_attackIcon;
	Texture m_defenceIcon;
	Rect m_button_hantei; // =ボタンの判定
	// Array<Rect> m_tehuda_hantei; // 手札の判定


	double enemy_scale = 0.85;
	double my_angle = 0.0;
	int32 damage_effect_width = 8;
	int32 my_damage_effect_cnt = 0;
	int32 ene_damage_effect_cnt = 0;
	int32 my_damage_max_cnt = 0;
	int32 ene_damage_max_cnt = 0;
	int32 my_effect_x = -1, my_effect_y = -1; // エフェクトの位置
	int32 ene_effect_x = -1, ene_effect_y = -1; // エフェクトの位置
	int32 flag_once_draw = 0; // 一回だけ描画させるための制御変数

	int32 table_id = 0; // 手札のID
	double yamahuda_angle = 0.0;
	double sutehuda_angle = 0.0;
	double tehuda_rate = 0.0;
	double tehuda_angle = 0.0;

	// コンストラクタで呼ばれる関数
	int32 getTableSize() const;
	void setupEnemy(bool is_boss);

	// これらはupdate()から呼ばれ、アニメーションの状態を更新し、完了時に次の状態へ遷移させる
	void attack();
	void updateTableDeck();
	void updateCombatEnemyEffect();
	void updateCombatMyEffect();
	void updateDiscardEffect();
	void updateCardDrawEffect();
	void updateWinEffect();
	// void finish();

	// これらはdraw()から呼ばれ、現在のアニメーション状態に基づいて描画を行う
	void drawTableDeck() const;
	void drawDefault() const;
	void drawCombatEnemyEffect() const;
	void drawCombatMyEffect() const;
	void drawDiscardEffect() const;
	void drawCardDrawEffect() const;
	// void drawWinEffect() const;


public:

	Battle(const InitData& init);

	// ~Battle(); 

	void update() override;

	void draw() const override;
};

#endif