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
	if (getData().Layer%10 == 0){
		map_nodes = map_nodes_source[Random(0, 5)]; // 0から5の範囲でランダムに選択
		getData().selected_nodes = map_nodes; // 選択されたノードを保存
	}else{
		map_nodes = getData().selected_nodes;
	}
	//Map生成完了！！

	banner.init(getData().money, getData().Layer); // バナーの初期化
}

// update() メソッドの実装
void Map::update() {
	deck_mode = banner.update(getData().Deck); // バナーの更新
	if (deck_mode)return;

	move_x = -Clamp(getData().Layer - 2, 0, 4) * 300; // マップの移動量を計算
	vector<tuple<Node, Circle, int>> next_node; // 次のノードとその位置
	if (map_nodes[getData().Layer % 10][getData().Index].NextLayerIndex & 1) {
		next_node.push_back(make_tuple(map_nodes[getData().Layer + 1][0], Circle{ move_x + 200 + (getData().Layer + 1) * 300, 420 + 0 * 265, 100.0 }, 0)); // 上層
	}
	if (map_nodes[getData().Layer % 10][getData().Index].NextLayerIndex & 2) {
		next_node.push_back(make_tuple(map_nodes[getData().Layer + 1][1], Circle{ move_x + 200 + (getData().Layer + 1) * 300, 420 + 1 * 265, 100.0 }, 1)); // 中層
	}
	if (map_nodes[getData().Layer % 10][getData().Index].NextLayerIndex & 4) {
		next_node.push_back(make_tuple(map_nodes[getData().Layer + 1][2], Circle{ move_x + 200 + (getData().Layer + 1) * 300, 420 + 2 * 265, 100.0 }, 2)); // 下層
	}
	hovered_index = -1;
	for (const auto& [node, circle, index] : next_node) {
		bool is_hovered = circle.mouseOver(); // ノードがホバーされているかどうかをチェック
		if (is_hovered) {
			Cursor::RequestStyle(CursorStyle::Hand); // カーソルスタイルを更新
			hovered_index = index; // ホバーされているノードのインデックスを保存
		}
		//クリック！！
		if (is_hovered && MouseL.down()) {
			getData().Layer++; // 次の層に移動
			getData().Index = index; // インデックスをセット
			if (node.type == MapPointType::Shop) {
				changeScene(State::Shop, 2s); // ショップに移動
			} else if (node.type == MapPointType::Boss) {
				getData().enemy = 2; // ボスの敵IDをセット
				changeScene(State::Battle, 2s); // ボス戦に移動
			} else if (node.type == MapPointType::Event) {
				//changeScene(State::Event, 2s); // イベントに移動
			} else if (node.type == MapPointType::Elite) {
				getData().enemy = 1; // エリートの敵IDをセット
				changeScene(State::Battle, 2s); // エリート戦に移動
			} else if (node.type == MapPointType::Enemy) {
				getData().enemy = 0; // 通常の敵IDをセット
				changeScene(State::Battle, 2s); // 通常戦闘に移動
			} else if (node.type == MapPointType::Treasure) {
				//changeScene(State::Battle, 2s); // 宝箱を開けるための戦闘に移動
			}
		}
	}

}

// draw() メソッドの実装
void Map::draw() const {
	if (deck_mode) {
		banner.draw(); // デッキモードのバナーを描画
		return;
	}
	// 現在の層に応じた背景画像を描画
	background_imgs.at((getData().Layer) / 10).draw();

	// 現在の層のノードを描画
	for (int i = 0; i < 10; i++) {
		for (int j = 0; j < 3; j++) {
			const Node& node = map_nodes[i][j];
			bool NextVisit = (getData().Layer % 10 + 1 == i) && map_nodes[getData().Layer % 10][getData().Index].NextLayerIndex & (1 << j); // 次の層のインデックスを取得
			bool hovered = NextVisit && (hovered_index == j); // ホバーされているかどうかをチェック
			Texture icon;
			switch (node.type) {
			case Boss:
				icon = hovered ? boss_icon_1 : boss_icon; // ホバーされている場合は特別なアイコンを使用
				break;
			case Elite:
				icon = hovered ? elite_icon_1 : elite_icon; // ホバーされている場合は特別なアイコンを使用
				break;
			case Event:
				icon = hovered ? event_icon_1 : event_icon; // ホバーされている場合は特別なアイコンを使用
				break;
			case Shop:
				icon = hovered ? shop_icon_1 : shop_icon; // ホバーされている場合は特別なアイコンを使用
				break;
			case Enemy:
				icon = hovered ? enemy_icon_1 : enemy_icon; // ホバーされている場合は特別なアイコンを使用
				break;
			case Treasure:
				icon = hovered ? treasure_icon_1 : treasure_icon; // ホバーされている場合は特別なアイコンを使用
				break;
			default:
				continue; // Noneの場合は何もしない
			}
			{
				double node_alpha = (getData().Layer%10 < i) ? 0.0 : 0.6; // 現在の層より上の層は半透明
				if ((node_alpha == 0.6) && (getData().Layer%10 == i) && (getData().Index == j)) {
					node_alpha = 0.0;
				}
				const ScopedColorMul2D colorMul{ ColorF{ 1.0 - node_alpha, 1.0 - node_alpha, 1.0 - node_alpha } };
				double size = 1.0 + (NextVisit ? Periodic::Sine1_1(1.5s) * 0.14 + 0.05 : 0.0); // 次の訪問地点はサイズが変化
				if (hovered) size = 1.2;
				// アイコンを描画
				icon.scaled(size).drawAt(move_x + 200 + i * 300, 420 + j * 265); // 適当な位置に描画
				// 矢印を描画
				for (int k = 0; k < 3; k++) {
					if (map_nodes[i][j].NextLayerIndex & (1 << k)) {
						double angle = atan2(100 * (k - j), 75.0);
						double distance = sqrt(170 * 170 * (j - k) * (j - k) + 90.0 * 90.0) / 100;
						arrow_icon.scaled(0.25 * distance, 0.25).rotated(angle).drawAt(move_x + 350.0 + i * 300, 440.0 + 132.5 * (j + k));
					}
				}
			}
		}
	}
	//TODO:主人公ちゃんの描画

	// バナーの描画
	banner.draw();
}

