#ifndef BATTLE_HPP
#define BATTLE_HPP

# include "Block.hpp" // Block クラスの定義があるヘッダファイルをインクルード
# include "common.hpp"
# include "Board.hpp" // Board クラスの定義があるヘッダファイルをインクルード
# include "Enemy.hpp" // Enemy クラスの定義があるヘッダファイルをインクルード
# include "Banner.hpp" // Enemy クラスの定義があるヘッダファイルをインクルード
# include "BattleCardRules.hpp"
# include "BattleLayoutRules.hpp"
# include "EnemyIntentRules.hpp"
#include "HPBar.hpp"
#include "BattleDamageRules.hpp"

// Data Manager の Deck を模倣したグローバル変数
// 実際には Data Manager クラス (DataManager.hpp) で定義し、ここからインクルードするのが望ましい

struct EnemyState
{
	String name;
	Texture texture;
	int32 type = 0;
	int32 hp = 0;
	int32 maxHp = 0;
	Array<EnemyAction> actionPattern; // 現在のターンで敵が何をするか
};

class Battle : public App::Scene
{
private:

	Board m_board; // 盤面の状態を管理する Board クラスのインスタンス
	Banner m_banner; // バナーの表示を管理する Banner クラスのインスタンス
	bool is_deck = false; // デッキ画面にいるかどうか
	bool is_board_locked = false; // 盤面の操作がロックされているかどうか
	bool is_exit = false; // 敵が逃走するか
	bool is_boss3 = false;
	bool is_settings_open = false;
	enum class VolumeSlider { None, Bgm, Se };
	VolumeSlider m_activeVolumeSlider = VolumeSlider::None;
	EnemyIntentRules::TurnState m_enemyIntentState;
	bool is_scene_transition_started = false;
	BattleCardRules::PointerInputOwner m_pointerInputOwner = BattleCardRules::PointerInputOwner::None;
	uint64 m_frameNumber = 0;
	int32 now_turn = 0; // ターン数
	int32 turn_start = 0; // 攻撃/防御のパターンの変化を管理(基本的には0のまま)
	int32 action_cycle = 1; // 敵の行動パターンのサイクル
	int32 table_max_size; // 手札のサイズ
	int32 deck_size = 0;
	Enemy m_enemyDB;
	EnemyState m_enemy; // 現在の敵の状態を保持する
	// 攻撃・防御の情報を保持するための変数
	int32 my_attack = 0;
	int32 my_defense = 0;
	int32 my_real_attack = 0;
	int32 my_attack_effect = 0;
	int32 ene_attack = 0;
	int32 ene_defense = 0;
	int32 ene_real_attack = 0;
	int32 ene_attack_effect = 0;
	double m_playerAttackStatPulseElapsed = BattleLayoutRules::CombatStatPulseDuration;
	double m_playerDefenseStatPulseElapsed = BattleLayoutRules::CombatStatPulseDuration;
	BattleLayoutRules::PlayerCombatStatPulseChanges m_playerCombatStatPulseChanges;
	int32 my_defense_effect = 0; // 自分の防御力を減らすエフェクトのための変数
	int32 ene_defense_effect = 0; // 敵の防御力のへらすエフェクトのための変数
	int32 reward_money = 0; // 報酬の金額
	std::vector<Block> m_cards;
	GameStateRules::BattleDeckState m_deckState;


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
	Texture m_reward_money; // 報酬のテクスチャ
	// Array<Rect> m_tehuda_hantei; // 手札の判定
	Font m_rewardFont; // 報酬のフォント
	Font m_numFont; // 攻撃・防御の数字のフォント
	Font m_combatFont; // 攻撃・防御値専用のフォント

	const Audio battle_bgm{ U"../../audio/battle_bgm.wav" , Loop::Yes };
	const Audio draw_card_se{ U"../../audio/draw_card.mp3" , Loop::No };
	const Audio drag_card_se{ U"../../audio/drag_card.mp3" , Loop::No };
	const Audio attack_se{ U"../../audio/attack.mp3" , Loop::No };

	// 味方・敵にダメージが入るまでの演出のための変数
	Vec2 my_attack_icon_pos;
	Vec2 ene_attack_icon_pos;
	Vec2 my_attack_icon_start;
	Vec2 ene_attack_icon_start;
	Vec2 my_attack_icon_end;
	Vec2 ene_attack_icon_end;
	double my_attack_icon_scale = 1.0;
	double ene_attack_icon_scale = 1.0;
	int32 my_attack_type = 0;
	int32 ene_attack_type = 0;

	// damage_effectの演出のための変数
	double enemy_base_scale = 1.0;
	double enemy_scale_multiplier = 1.0;
	double my_angle = 0.0;
	int32 my_damage_effect_cnt = 0;
	int32 ene_damage_effect_cnt = 0;
	std::vector<int32> m_playerDamageHits;
	std::vector<int32> m_enemyDamageHits;
	int32 my_effect_x = -1, my_effect_y = -1; // エフェクトの位置
	int32 ene_effect_x = -1, ene_effect_y = -1; // エフェクトの位置
	int32 flag_once_draw = 0; // 一回だけ描画させるための制御変数

	// wineffectのための変数
	bool is_gamewin = false;
	double enemy_image_alpha = 1.0; // 敵を倒した際のフェードアウト演出のための変数
	RenderTexture m_combatSceneBuffer; // 戦闘画面全体を描き込むためのレンダーターゲット
	RenderTexture m_blurInternalBuffer;

	int32 table_id = 0; // 手札のID
	bool m_handDealComplete = false;
	double yamahuda_angle = 0.0;
	double sutehuda_angle = 0.0;
	struct DiscardCardMotion {
		int32 card_id = -1;
		Vec2 start;
		bool detached = false;
		bool complete = false;
	};
	std::vector<DiscardCardMotion> m_discardCardMotions;
	bool m_discardCardMotionsInitialized = false;

	// コンストラクタで呼ばれる関数
	int32 getTableSize() const;
	Point GetHandPosition(int32 slot) const;
	Rect GetAttackButtonRect() const;
	void setupEnemy(int32 type, int32 layer);

	// これらはupdate()から呼ばれ、アニメーションの状態を更新し、完了時に次の状態へ遷移させる
	void getEnemyInfo();
	void attack();
	void updateTableDeck();
	void PrepareDrawPileForTurn();
	bool MoveCard(int32 card_id, GameStateRules::CardZone expected, GameStateRules::CardZone destination);
	bool ApplyBoardZoneChanges();
	const std::vector<int32>& Cards(GameStateRules::CardZone zone) const;
	void updateCombatEnemyEffect();
	void updateCombatMyEffect();
	void updateDiscardEffect();
	void updateCardDrawEffect();
	void updateWinEffect();
	void updateGameOverEffect();
	void updateSettingsOverlay(const BoardInputFrame& input);
	void ApplyAudioSettings() const;
	void AssertCardOwnership(const char* context) const;
	// void finish();

	// これらはdraw()から呼ばれ、現在のアニメーション状態に基づいて描画を行う
	// void drawTableDeck() const;
	void drawHandCards() const;
	bool drawDefault() const;
	void drawCombatEnemyEffect() const;
	void drawCombatMyEffect() const;
	void drawDiscardEffect() const;
	void drawCardDrawEffect() const;
	void drawWinEffect() const;
	void drawSettingsOverlay() const;


public:

	Battle(const InitData& init);

	// ~Battle(); 

	void update() override;

	void draw() const override;
};

#endif
