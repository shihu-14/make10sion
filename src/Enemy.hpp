#ifndef ENEMY_HPP
#define ENEMY_HPP
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
	int32 type; // 敵の種類（通常敵0、エリート敵1、ボス敵2）
	int32 layer; // 序盤0、中盤1、終盤2
	int32 maxHp = 0;
	Array<EnemyAction> actionPattern; // ターンごとの行動パターン
};

// ゲームに登場するすべての敵データを管理するクラス
class Enemy
{
private:
	Array<EnemyData> m_enemies;
	void loadEnemies(); // 敵データをロードする

public:
	Enemy();
	const EnemyData& getOneEnemy(int32 type, int32 act) const; // データベースからランダムな敵データを1体返す
};
#endif
