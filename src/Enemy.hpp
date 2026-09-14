#ifndef ENEMY_HPP
#define ENEMY_HPP
#include <Siv3D.hpp>


// 1ターンごとの敵の行動パターン
struct EnemyAction
{
	int32 attack = 0;//そのターンの攻撃値を表す．
	int32 defense = 0;//そのターンの防御値を表す．
};

// 敵一体ごとの静的なデータ（データベース用）
struct EnemyData
{
	String name;
	String texturePath;
	int32 type; // 敵の種類を通常0，エリート1，ボス2で表す．
	int32 layer; // 出現時期を序盤0，中盤1，終盤2で表す．
	int32 maxHp = 0;//敵の最大HPを表す．
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
