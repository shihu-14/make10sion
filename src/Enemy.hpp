#include <Siv3D.hpp>


// 1ターンごとの敵の行動パターン
struct EnemyAction
{
	int32 attack = 0;
	int32 defense = 0;
};

// 敵一体ごとの静的なデータ（データベース用）
struct EnemyData
{
	String name;
	String texturePath;
	int32 maxHp = 0;
	Array<EnemyAction> actionPattern; // ターンごとの行動パターン
    bool isDefeated = false;
};

// ゲームに登場するすべての敵データを管理するクラス
class Enemy
{
private:
	Array<EnemyData> m_enemies;
	void loadEnemies(); // 敵データをロードする

public:
	Enemy();
    void markAsDefeated(const String& enemyName); // 指定された敵を「倒した」状態にす
	const EnemyData& getOneEnemy(bool is_boss) const; // データベースからランダムな敵データを1体返す
};
