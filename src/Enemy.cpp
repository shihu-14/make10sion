#include "Enemy.hpp"

Enemy::Enemy()
{
	loadEnemies();
}

void Enemy::loadEnemies()
{
	m_enemies.push_back(
		{
			.name = U"enemy_1",
			.texturePath = U"../image/enemy_1.png", // PNGファイルへのパス
			.maxHp = 50,
			.actionPattern = {
				{ .attack = 10, .defense = 0 }, // 1ターン目
				{ .attack = 0,  .defense = 8 }, // 2ターン目
				{ .attack = 12, .defense = 0 }, // 3ターン目 (以降、このパターンがループする)
			}
		});

	m_enemies.push_back(
		{
			.name = U"enemy_2",
			.texturePath = U"../image/enemy_2.png", // PNGファイルへのパス
			.maxHp = 80,
			.actionPattern = {
				{ .attack = 0,  .defense = 20 },
				{ .attack = 25, .defense = 0 },
				{ .attack = 0,  .defense = 20 },
				{ .attack = 25, .defense = 0 },
			}
		});
	
	m_enemies.push_back(
		{
			.name = U"enemy_3",
			.texturePath = U"../image/enemy_3.png", // PNGファイルへのパス
			.maxHp = 40,
			.actionPattern = {
				{ .attack = 5, .defense = 5 },
				{ .attack = 5, .defense = 5 },
				{ .attack = 20, .defense = 0 },
			}
		});
}

// データベースからランダムに1体の敵を選ぶ
const EnemyData& Enemy::getOneEnemy() const
{
	// 配列の中からランダムな要素を1つ返す(後で変更する)
	return m_enemies.choice();
}

// 指定された名前の敵を「倒した」状態にする関数
void Enemy::markAsDefeated(const String& enemyName)
{
	for (auto& enemy : m_enemies)
	{
		if (enemy.name == enemyName)
		{
			enemy.isDefeated = true;
			return; // 該当の敵を見つけたら処理を終了します
		}
	}
}
