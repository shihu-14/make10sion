#include "Map.hpp"
using namespace std;

// MapPointTypeのメンバーを直接使えるようにする
using enum MapPointType;

// Mapクラスのコンストラクタの実装
Map::Map(const InitData& init) :
	App::Scene(init),
	background_imgs(3),
	map_nodes(30, vector<Node>(3)), // 30層、各層に3地点のノードを初期化
	map_nodes_source(6, vector<vector<Node>>(10, vector<Node>(3))) // 6つのパターン、10層、各層に3地点のノードを初期化
{
	//背景画像の読み込み
	for (int i = 0; i < 3; i++)
		background_imgs.at(i) = Texture{ Unicode::Widen("../../image/map_haikei_" + to_string(i + 1) + "sou.png") };

	// マップパターン生成(手動)
	map_nodes_source = {
		// 1
		{
			// 1層
			{
				Node{None, false, 0}, // なし
				Node{Enemy, false, 7}, // 1層目の敵
				Node{None, false, 0} // なし
			},
		// 2層
		{
			Node{Enemy, false, 3}, // 敵
			Node{Event, false, 2}, // イベント
			Node{Enemy, false, 6} // 敵
		},
		// 3層
		{
			Node{Elite, false, 1}, // エリート
			Node{Enemy, false, 7}, // 敵
			Node{Enemy, false, 4} // 敵
		},
		// 4層
		{
			Node{Enemy, false, 3}, // 敵
			Node{Treasure, false, 2}, // 宝箱
			Node{Event, false, 6} // イベント
		},
		// 5層
		{
			Node{Elite, false, 1}, // エリート
			Node{Shop, false, 7}, // ショップ
			Node{Enemy, false, 4} // 敵
		},
		// 6層
		{
			Node{Enemy, false, 3}, // 敵
			Node{Event, false, 2}, // イベント
			Node{Enemy, false, 6} // 敵
		},
		// 7層
		{
			Node{Elite, false,3 }, // エリート
			Node{Treasure, false,6 }, // 宝箱
			Node{Enemy, false,4 } // 敵
		},
		// 8層
		{
			Node{Shop , false,1 }, // ショップ
			Node{Enemy, false,2 }, // 敵
			Node{Enemy, false,4 } // 敵
		},
		// 9層
		{
			Node{Enemy, false, 2}, // 敵
			Node{Elite, false, 2}, // エリート
			Node{Event, false, 2} // イベント
		},
		// 10層
		{
			Node{None, false, 0}, // なし
			Node{Boss, false, 0}, // ボス
			Node{None, false, 0} // なし
		}
	},
		// Map 2
{
	// 1層
	{
		Node{None, false, 0},
		Node{Enemy, false, 7},
		Node{None, false, 0}
	},
		// 2層
		{
			Node{Enemy, false, 3},
			Node{Elite, false, 2},
			Node{Enemy, false, 6}
		},
		// 3層
		{
			Node{Event, false, 3},
			Node{Enemy, false, 2},
			Node{Elite, false, 6}
		},
		// 4層
		{
			Node{Enemy, false, 3},
			Node{Elite, false, 2},
			Node{Treasure, false, 6}
		},
		// 5層
		{
			Node{Elite, false, 3},
			Node{Shop, false, 2},
			Node{Enemy, false, 6}
		},
		// 6層
		{
			Node{Enemy, false, 1},
			Node{Elite, false, 7},
			Node{Event, false, 4}
		},
		// 7層
		{
			Node{Treasure, false, 3},
			Node{Elite, false, 2},
			Node{Enemy, false, 6}
		},
		// 8層
		{
			Node{Shop, false, 3},
			Node{Elite, false, 2},
			Node{Enemy, false, 6}
		},
		// 9層
		{
			Node{Enemy, false, 2},
			Node{Elite, false, 2},
			Node{Event, false, 2}
		},
		// 10層
		{
			Node{None, false, 0},
			Node{Boss, false, 0},
			Node{None, false, 0}
		}
	},// Map 3
	{
		// 1層
		{
			Node{None, false, 0},
			Node{Enemy, false, 7},
			Node{None, false, 0}
		},
		// 2層
		{
			Node{Shop, false, 3},
			Node{Enemy, false, 2},
			Node{Event, false, 6}
		},
		// 3層
		{
			Node{Treasure, false, 3},
			Node{Enemy, false, 2},
			Node{Event, false, 6}
		},
		// 4層
		{
			Node{Event, false, 3},
			Node{Shop, false, 2},
			Node{Enemy, false, 6}
		},
		// 5層
		{
			Node{Treasure, false, 3},
			Node{Enemy, false, 2},
			Node{Elite, false, 6}
		},
		// 6層
		{
			Node{Enemy, false, 3},
			Node{Event, false, 2},
			Node{Shop, false, 6}
		},
		// 7層
		{
			Node{Elite, false, 3},
			Node{Treasure, false, 2},
			Node{Enemy, false, 6}
		},
		// 8層
		{
			Node{Shop, false, 3},
			Node{Enemy, false, 2},
			Node{Event, false, 6}
		},
		// 9層
		{
			Node{Enemy, false, 2},
			Node{Event, false, 2},
			Node{Treasure, false, 2}
		},
		// 10層
		{
			Node{None, false, 0},
			Node{Boss, false, 0},
			Node{None, false, 0}
		}
	},// Map 4
{
	// 1層
	{
		Node{None, false, 0},
		Node{Enemy, false, 7},
		Node{None, false, 0}
	},
		// 2層
		{
			Node{Enemy, false, 3},
			Node{Enemy, false, 2},
			Node{Event, false, 6}
		},
		// 3層
		{
			Node{Elite, false, 3},
			Node{Enemy, false, 2},
			Node{Shop, false, 6}
		},
		// 4層
		{
			Node{Enemy, false, 1},
			Node{Event, false, 5},
			Node{Treasure, false, 4}
		},
		// 5層
		{
			Node{Elite, false, 2},
			Node{None, false, 0},
			Node{Event, false, 2}
		},
		// 6層
		{
			Node{None, false, 0},
			Node{Shop, false, 7},
			Node{None, false, 0}
		},
		// 7層
		{
			Node{Enemy, false, 1},
			Node{Treasure, false, 2},
			Node{Enemy, false, 4}
		},
		// 8層
		{
			Node{Elite, false, 3},
			Node{Enemy, false, 2},
			Node{Shop, false, 6}
		},
		// 9層
		{
			Node{Enemy, false, 2},
			Node{Elite, false, 2},
			Node{Event, false, 2}
		},
		// 10層
		{
			Node{None, false, 0},
			Node{Boss, false, 0},
			Node{None, false, 0}
		}
	},
		// Map 5
	{
		// 1層
		{
			Node{None, false, 0},
			Node{Enemy, false, 7},
			Node{None, false, 0}
		},
		// 2層
		{
			Node{Event, false, 3},
			Node{Enemy, false, 2},
			Node{Elite, false, 6}
		},
		// 3層
		{
			Node{Treasure, false, 3},
			Node{Elite, false, 2},
			Node{Enemy, false, 6}
		},
		// 4層
		{
			Node{Shop, false, 3},
			Node{Enemy, false, 2},
			Node{Elite, false, 6}
		},
		// 5層
		{
			Node{Event, false, 3},
			Node{Shop, false, 2},
			Node{Enemy, false, 6}
		},
		// 6層
		{
			Node{Treasure, false, 3},
			Node{Enemy, false, 2},
			Node{Elite, false, 6}
		},
		// 7層
		{
			Node{Shop, false, 3},
			Node{Elite, false, 2},
			Node{Enemy, false, 6}
		},
		// 8層
		{
			Node{Event, false, 3},
			Node{Enemy, false, 2},
			Node{Elite, false, 6}
		},
		// 9層
		{
			Node{Treasure, false, 2},
			Node{Elite, false, 2},
			Node{Enemy, false, 2}
		},
		// 10層
		{
			Node{None, false, 0},
			Node{Boss, false, 0},
			Node{None, false, 0}
		}
	},// Map 6
{
	// 1層
	{
		Node{None, false, 0},
		Node{Enemy, false, 7},
		Node{None, false, 0}
	},
		// 2層
		{
			Node{Enemy, false, 3},
			Node{Event, false, 2},
			Node{Enemy, false, 6}
		},
		// 3層
		{
			Node{Shop, false, 3},
			Node{Enemy, false, 2},
			Node{Enemy, false, 6}
		},
		// 4層
		{
			Node{Enemy, false, 3},
			Node{Event, false, 2},
			Node{Treasure, false, 6}
		},
		// 5層
		{
			Node{Shop, false, 3},
			Node{Elite, false, 2},
			Node{Enemy, false, 6}
		},
		// 6層
		{
			Node{Event, false, 3},
			Node{Enemy, false, 2},
			Node{Treasure, false, 6}
		},
		// 7層
		{
			Node{Elite, false, 3},
			Node{Enemy, false, 2},
			Node{Elite, false, 6}
		},
		// 8層
		{
			Node{Shop, false, 1},
			Node{Elite, false, 5},
			Node{Enemy, false, 4}
		},
		// 9層
		{
			Node{Elite, false, 2},
			Node{Event, false, 2},
			Node{Elite, false, 2}
		},
		// 10層
		{
			Node{None, false, 0},
			Node{Boss, false, 0},
			Node{None, false, 0}
		}
	}
	};

	//抽選スタート！
	for (int i = 0;i < 3;i++){
		std::vector<std::vector<Node>> selected_nodes(10, std::vector<Node>(3)); // 10層、各層に3地点のノードを初期化
		selected_nodes = map_nodes_source[Random(0, 5)]; // 0から5の範囲でランダムに選択
		map_nodes.insert(map_nodes.begin() + i * 10, selected_nodes.begin(), selected_nodes.end());
	}
	//Map生成完了！！

}

// update() メソッドの実装
void Map::update() {
	

}

// draw() メソッドの実装
void Map::draw() const {
	// 現在の層に応じた背景画像を描画
	background_imgs.at(getData().Layer / 10).draw();

	// 現在の層のノードを描画
	for (int i = 0; i < 10; i++) {
		for (int j = 0; j < 3; j++) {
			const Node& node = map_nodes[i][j];
			Texture icon;

			switch (node.type) {
			case Boss:
				icon = boss_icon;
				break;
			case Elite:
				icon = elite_icon;
				break;
			case Event:
				icon = event_icon;
				break;
			case Shop:
				icon = shop_icon;
				break;
			case Enemy:
				icon = enemy_icon_0;
				break;
			case Treasure:
				icon = treasure_icon;
				break;
			default:
				continue; // Noneの場合は何もしない
			}

			// アイコンを描画
			icon.drawAt(100 + j * 50, 100 + i * 50); // 適当な位置に描画
		}
	}

}

