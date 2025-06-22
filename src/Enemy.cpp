#include "Enemy.hpp"

Enemy::Enemy()
{
	loadEnemies();
}

// ゲームに登場するすべての敵データを定義する
void Enemy::loadEnemies()
{
	// ----------------------------------------
	// 通常敵 (序盤)
	// ----------------------------------------
	m_enemies.push_back(
		{
			.name = U"enemy_1",
			.texturePath = U"../image/enemy_1.png",
			.maxHp = 20,
			.actionPattern = {
				{ .attack = 8, .defense = 2 },
				{ .attack = 8, .defense = 2 },
				{ .attack = 8, .defense = 2 },
				{ .attack = 8, .defense = 2 },
				{ .attack = 8, .defense = 2 },
			}
		});

	m_enemies.push_back(
		{
			.name = U"enemy_2",
			.texturePath = U"../image/enemy_2.png",
			.maxHp = 30,
			.actionPattern = {
				{ .attack = 0, .defense = 0 },
				{ .attack = 9, .defense = 2 },
				{ .attack = 10, .defense = 2 },
				{ .attack = 11, .defense = 2 },
				{ .attack = 12, .defense = 2 },
			}
		});
	
	m_enemies.push_back(
		{
			.name = U"enemy_3",
			.texturePath = U"../image/enemy_3.png",
			.maxHp = 30,
			.actionPattern = {
				{ .attack = 14, .defense = 0 },
				{ .attack = 12, .defense = 0 },
				{ .attack = 10, .defense = 0 },
				{ .attack = 8,  .defense = 0 },
				{ .attack = 0,  .defense = 0 },
			}
		});

	// ----------------------------------------
	// 通常敵 (中盤)
	// ----------------------------------------
	m_enemies.push_back(
		{
			.name = U"enemy_4",
			.texturePath = U"../image/enemy_4.png",
			.maxHp = 60,
			.actionPattern = {
				{ .attack = 0,  .defense = 15 },
				{ .attack = 40, .defense = 0 },
				{ .attack = 0,  .defense = 15 },
				{ .attack = 40, .defense = 0 },
				{ .attack = 0,  .defense = 15 },
			}
		});

	m_enemies.push_back(
		{
			.name = U"enemy_5",
			.texturePath = U"../image/enemy_5.png",
			.maxHp = 60,
			.actionPattern = {
				{ .attack = 0,   .defense = 20 },
				{ .attack = 0,   .defense = 20 },
				{ .attack = 0,   .defense = 20 },
				{ .attack = 0,   .defense = 20 },
				{ .attack = 100, .defense = 20 },
			}
		});
	
	m_enemies.push_back(
		{
			.name = U"enemy_6",
			.texturePath = U"../image/enemy_6.png",
			.maxHp = 70,
			.actionPattern = {
				{ .attack = 16, .defense = 10 },
				{ .attack = 16, .defense = 10 },
				{ .attack = 16, .defense = 10 },
				{ .attack = 16, .defense = 10 },
				{ .attack = 16, .defense = 10 },
			}
		});

	// ----------------------------------------
	// 通常敵 (終盤)
	// ----------------------------------------
	m_enemies.push_back(
		{
			.name = U"enemy_7",
			.texturePath = U"../image/enemy_7.png",
			.maxHp = 90,
			.actionPattern = {
				{ .attack = 20, .defense = 15 },
				{ .attack = 30, .defense = 15 },
				{ .attack = 40, .defense = 15 },
				{ .attack = 50, .defense = 15 },
				{ .attack = 60, .defense = 15 },
			}
		});

	m_enemies.push_back(
		{
			.name = U"enemy_8",
			.texturePath = U"../image/enemy_8.png",
			.maxHp = 80,
			.actionPattern = {
				{ .attack = 60, .defense = 12 },
				{ .attack = 50, .defense = 12 },
				{ .attack = 40, .defense = 12 },
				{ .attack = 30, .defense = 12 },
				{ .attack = 20, .defense = 12 },
			}
		});

	m_enemies.push_back(
		{
			.name = U"enemy_9",
			.texturePath = U"../image/enemy_9.png",
			.maxHp = 90,
			.actionPattern = {
				{ .attack = 60, .defense = 8 },
				{ .attack = 0,  .defense = 20 },
				{ .attack = 60, .defense = 8 },
				{ .attack = 0,  .defense = 20 },
				{ .attack = 60, .defense = 8 },
			}
		});
	
	// ----------------------------------------
	// エリート敵
	// ----------------------------------------
	m_enemies.push_back(
		{
			.name = U"enemy_10",
			.texturePath = U"../image/enemy_10.png",
			.maxHp = 60,
			.actionPattern = {
				{ .attack = -10, .defense = 3 }, // 3+2*N
				{ .attack = -10, .defense = 3 },
				{ .attack = -10, .defense = 3 },
				{ .attack = -10, .defense = 3 },
				{ .attack = -10, .defense = 3 },
			}
		});

	m_enemies.push_back(
		{
			.name = U"enemy_11",
			.texturePath = U"../image/enemy_11.png",
			.maxHp = 70,
			.actionPattern = {
				{ .attack = 0,   .defense = 10 },
				{ .attack = 0,   .defense = 10 },
				{ .attack = 0,   .defense = 10 },
				{ .attack = -11, .defense = 0  }, // 行動変化のトリガー
                { .attack = 20,   .defense = 0 },
				{ .attack = 9,   .defense = 0 },
				{ .attack = 9,   .defense = 0 },
				{ .attack = 9, .defense = 0  }, // 行動変化のトリガー
			}
		});

	m_enemies.push_back(
		{
			.name = U"enemy_12",
			.texturePath = U"../image/enemy_12.png",
			.maxHp = 100,
			.actionPattern = {
				{ .attack = -12, .defense = 15 }, // 60-4*N
				{ .attack = -12, .defense = 15 },
				{ .attack = -12, .defense = 15 },
				{ .attack = -12, .defense = 15 },
				{ .attack = -12, .defense = 15 },
			}
		});

	m_enemies.push_back(
		{
			.name = U"enemy_13",
			.texturePath = U"../image/enemy_13.png",
			.maxHp = 80,
			.actionPattern = {
				{ .attack = -13, .defense = 15 }, // 40 + 30マネー強奪
				{ .attack = -13, .defense = 15 },
				{ .attack = -13, .defense = 15 },
				{ .attack = -13, .defense = 15 },
				{ .attack = -14, .defense = -14}, // 逃走
			}
		});
	
	m_enemies.push_back(
		{
			.name = U"enemy_14",
			.texturePath = U"../image/enemy_14.png",
			.maxHp = 150,
			.actionPattern = {
				{ .attack = -15, .defense = 14 }, // 10+14*N
				{ .attack = -15, .defense = 14 },
				{ .attack = -15, .defense = 14 },
				{ .attack = -15, .defense = 14 },
				{ .attack = -15, .defense = 14 },
			}
		});
	
	m_enemies.push_back(
		{
			.name = U"enemy_15",
			.texturePath = U"../image/enemy_15.png",
			.maxHp = 160,
			.actionPattern = {
				{ .attack = 120, .defense = 0 },
				{ .attack = 0,   .defense = 30 },
				{ .attack = 120, .defense = 0 },
				{ .attack = 0,   .defense = 30 },
				{ .attack = 120, .defense = 0 },
			}
		});

    // ========================================
	// ボス敵
	// ========================================
	m_enemies.push_back(
		{
			.name = U"boss_1",
			.texturePath = U"../image/boss_1.png",
			.maxHp = 100,
			.actionPattern = {
				{ .attack = 30, .defense = 0 },
				{ .attack = 30, .defense = 0 },
				{ .attack = 30, .defense = 0 },
				{ .attack = 30, .defense = 0 },
				{ .attack = -16, .defense = 0 },
				{ .attack = -18, .defense = 15 },
				{ .attack = -18, .defense = 15 },
				{ .attack = -18, .defense = 15 },
				{ .attack = -18, .defense = 15 },
				{ .attack = -18, .defense = 15 },
			}
		});
	m_enemies.push_back(
		{
			.name = U"boss_2",
			.texturePath = U"../image/boss_2.png",
			.maxHp = 100,
			.actionPattern = {
				{ .attack = 6, .defense = 15 },
				{ .attack = 5, .defense = 15 },
				{ .attack = 6, .defense = 15 },
				{ .attack = 5, .defense = 15 },
				{ .attack = 6, .defense = 15 },
				{ .attack = 60, .defense = 0 },
			}
		});
	m_enemies.push_back(
		{
			.name = U"boss_3",
			.texturePath = U"../image/boss_3.png",
			.maxHp = 200,
			.actionPattern = {
				{ .attack = 40, .defense = 15 },
				{ .attack = 40, .defense = 15 },
				{ .attack = 40, .defense = 15 },
				{ .attack = 40, .defense = 15 },
				{ .attack = -17, .defense = 15 },
			}
		});
	m_enemies.push_back(
		{
			.name = U"boss_4",
			.texturePath = U"../image/boss_4.png",
			.maxHp = 200,
			.actionPattern = {
				{ .attack = 60, .defense = 0 },
				{ .attack = 60, .defense = 0 },
				{ .attack = 60, .defense = 0 },
				{ .attack = 60, .defense = 0 },
				{ .attack = -16, .defense = 0 },
				{ .attack = -19, .defense = 20 },
				{ .attack = -19, .defense = 20 },
				{ .attack = -19, .defense = 20 },
				{ .attack = -19, .defense = 20 },
				{ .attack = -19, .defense = 20 },
			}
		});
	m_enemies.push_back(
		{
			.name = U"boss_5",
			.texturePath = U"../image/boss_5.png",
			.maxHp = 300,
			.actionPattern = {
				{ .attack = 12, .defense = 30 },
				{ .attack = 10, .defense = 30 },
				{ .attack = 12, .defense = 30 },
				{ .attack = 10, .defense = 30 },
				{ .attack = 12, .defense = 30 },
				{ .attack = 150, .defense = 0 },
			}
		});
	m_enemies.push_back(
		{
			.name = U"boss_6",
			.texturePath = U"../image/boss_6.png",
			.maxHp = 300,
			.actionPattern = {
				{ .attack = 60, .defense = 20 },
				{ .attack = 60, .defense = 20 },
				{ .attack = 60, .defense = 20 },
				{ .attack = 60, .defense = 20 },
				{ .attack = -17, .defense = 20 },
			}
		});
}

// データベースからランダムに1体の敵を選ぶ
const EnemyData& Enemy::getOneEnemy(bool is_boss) const
{
	// 敵データの数を取得
    if (is_boss)
    {
		// ボス敵のデータを取得
		return m_enemies[Random(15, 20)]; // 通常敵は5体
	}
	else
	{
		// 通常敵のデータを取得
		return m_enemies[Random(0, 14)]; // ボス敵は6体
    }
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
