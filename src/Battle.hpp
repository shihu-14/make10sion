#ifndef BATTLE_HPP
#define BATTLE_HPP

#include "common.hpp"
#include "Board.hpp"
#include "Enemy.hpp"
#include "Banner.hpp"
#include "HPBar.hpp"
#include "BattleCombat.hpp"
#include "BattleLayoutRules.hpp"
#include "EnemyIntentRules.hpp"

class Battle : public App::Scene {
private:
	// 配る → 置く → 自分の攻撃 → 敵の攻撃 → 捨てる，で1ターン．
	enum class Phase { Drawing, Planning, PlayerAttack, EnemyAttack, Discarding, Victory, Defeat };
	// デッキと設定は，戦闘の進み具合とは別に開く．
	enum class Overlay { None, Deck, Settings };
	enum class VolumeSlider { None, Bgm, Se };
	enum class DealPhase { RotatePile, Cards, ReturnPile };

	struct EnemyState {
		Texture texture;
		int32 hp = 0;
		int32 maxHp = 0;
		int32 type = 0;
		Array<EnemyAction> actions;
	};

	Board m_board;
	Banner m_banner;
	Enemy m_enemyDB;
	EnemyState m_enemy;
	EnemyIntentRules::TurnState m_enemyIntent;
	GameStateRules::BattleDeckState m_deck;
	// 位置や回転を変えるのはこのコピー．所持デッキには戻さない．
	std::vector<Block> m_cards;
	Phase m_phase = Phase::Drawing;
	Overlay m_overlay = Overlay::None;
	VolumeSlider m_activeVolumeSlider = VolumeSlider::None;
	BattleCardRules::PointerInputOwner m_pointerOwner = BattleCardRules::PointerInputOwner::None;
	bool m_transitionStarted = false;
	bool m_blocksPlayerDamage = false;
	// 最初の配札が終わったところを0ターン目にする
	int32 m_turn = -1;
	// 敵によっては，行動の途中から別の周期に切り替わる．
	int32 m_turnStart = 0;
	int32 m_actionCycle = 1;
	int32 m_handLimit = 0;
	int32 m_cardCount = 0;
	int32 m_reward = 0;

	// 盤面から決まった値と，演出で減っていく表示の値を分けておく．
	BattleCombat::CombatValues m_playerStats;
	BattleCombat::CombatValues m_enemyStats;
	BattleCombat::CombatValues m_playerDisplay;
	BattleCombat::CombatValues m_enemyDisplay;
	// 「=」を押した時点の結果．敵の攻撃が終わるまで使う．
	BattleCombat::CombatResult m_turnResult;
	BattleCombat::AttackAnimation m_attack;
	Point m_hitPosition{ -1, -1 };
	double m_playerAttackPulseTime = BattleLayoutRules::CombatStatPulseDuration;
	double m_playerDefensePulseTime = BattleLayoutRules::CombatStatPulseDuration;
	double m_enemyBaseScale = 1.0;
	double m_enemyHitScale = 1.0;
	double m_enemyAlpha = 1.0;
	double m_playerAngle = 0.0;
	// 配札・捨て札・勝敗の演出に使う．攻撃の時間は m_attack 側．
	Stopwatch m_phaseTimer;

	// 戦闘画面で使う画像と音
	Texture m_backgroundTexture;
	Texture m_playerTexture;
	Texture m_drawPileTexture;
	Texture m_discardPileTexture;
	Texture m_attackButtonTexture;
	Texture m_effectTexture;
	Texture m_attackIcon;
	Texture m_defenseIcon;
	Texture m_rewardTexture;
	Texture m_settingsBackgroundTexture;
	Texture m_settingsBackButtonTexture;
	HPBar m_playerHpBar;
	HPBar m_enemyHpBar;
	Font m_rewardFont;
	Font m_numFont;
	Font m_combatFont;
	Audio m_battleBgm{ U"../../audio/battle_bgm.wav", Loop::Yes };
	Audio m_drawCardSound{ U"../../audio/draw_card.mp3" };
	Audio m_dragCardSound{ U"../../audio/drag_card.mp3" };
	Audio m_attackSound{ U"../../audio/attack.mp3" };
	// 報酬を出すときは，背後の戦闘画面だけをぼかす．
	RenderTexture m_sceneBuffer;
	RenderTexture m_blurBuffer;

	// 描き始めた枚数．手札へ移した枚数とは別．
	int32 m_dealtCardCount = 0;
	DealPhase m_dealPhase = DealPhase::RotatePile;
	double m_drawPileAngle = 0.0;
	double m_discardPileAngle = 0.0;
	struct DiscardMotion {
		int32 cardId = -1;
		Vec2 start;
		bool detached = false;
		bool complete = false;
	};
	std::vector<DiscardMotion> m_discardMotions;
	bool m_discardStarted = false;

	int32 handLimit() const;
	Point handPosition(int32 slot) const;
	int32 hoveredHandCard(Point cursor) const;
	void setupEnemy(int32 type, int32 act);

	void beginDraw();
	void beginPlanning();
	void beginPlayerAttack();
	void beginEnemyAttack();
	void beginDiscard();
	void updateDraw();
	void updatePlanning(const BoardInputFrame& input);
	void updateAttack(double deltaSeconds);
	void updateDiscard();
	void updateVictory();
	void updateDefeat();

	void updateEnemyIntent();
	void refreshCombatPreview();
	bool updateOverlay(const BoardInputFrame& input);
	bool updateDeckButton(const BoardInputFrame& input, bool allowOpen);
	void updateCardInput(const BoardInputFrame& input, int32 hoveredCardId);
	void updateSettings(const BoardInputFrame& input);
	void applyAudioSettings();
	void prepareDrawPile();
	bool moveCard(int32 cardId, GameStateRules::CardZone source, GameStateRules::CardZone destination);
	void applyBoardChanges();
	const std::vector<int32>& cards(GameStateRules::CardZone zone) const;

	void drawHand() const;
	void drawScene() const;
	void drawAttack() const;
	void drawVictory() const;
	void drawSettings() const;

public:
	Battle(const InitData& init);
	void update() override;
	void draw() const override;
};

#endif
