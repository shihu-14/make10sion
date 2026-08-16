#include "Map.hpp"
#include <algorithm>
using namespace std;

// MapPointTypeのメンバーを直接使えるようにする
using enum MapPointType;

// Mapクラスのコンストラクタの実装
Map::Map(const InitData& init) :
	App::Scene(init),
	background_imgs(3),
	map_nodes(10, vector<Node>(3)), // 現在区間の10層、各層に3地点のノードを初期化
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
				Node{None, 0}, // なし
				Node{Enemy, 7}, // 1層目の敵
				Node{None, 0} // なし
			},
		// 2層
		{
			Node{Enemy, 3}, // 敵
			Node{Event, 2}, // イベント
			Node{Enemy, 6} // 敵
		},
		// 3層
		{
			Node{Elite, 1}, // エリート
			Node{Enemy, 7}, // 敵
			Node{Enemy, 4} // 敵
		},
		// 4層
		{
			Node{Enemy, 3}, // 敵
			Node{Treasure, 2}, // 宝箱
			Node{Event, 6} // イベント
		},
		// 5層
		{
			Node{Elite, 1}, // エリート
			Node{Shop, 7}, // ショップ
			Node{Enemy, 4} // 敵
		},
		// 6層
		{
			Node{Enemy, 3}, // 敵
			Node{Event, 2}, // イベント
			Node{Enemy, 6} // 敵
		},
		// 7層
		{
			Node{Elite,3 }, // エリート
			Node{Treasure,6 }, // 宝箱
			Node{Enemy,4 } // 敵
		},
		// 8層
		{
			Node{Shop ,1 }, // ショップ
			Node{Enemy,2 }, // 敵
			Node{Enemy,4 } // 敵
		},
		// 9層
		{
			Node{Enemy, 2}, // 敵
			Node{Elite, 2}, // エリート
			Node{Event, 2} // イベント
		},
		// 10層
		{
			Node{None, 0}, // なし
			Node{Boss, 0}, // ボス
			Node{None, 0} // なし
		}
	},
		// Map 2
{
	// 1層
	{
		Node{None, 0},
		Node{Enemy, 7},
		Node{None, 0}
	},
		// 2層
		{
			Node{Enemy, 3},
			Node{Elite, 2},
			Node{Enemy, 6}
		},
		// 3層
		{
			Node{Event, 3},
			Node{Enemy, 2},
			Node{Elite, 6}
		},
		// 4層
		{
			Node{Enemy, 3},
			Node{Elite, 2},
			Node{Treasure, 6}
		},
		// 5層
		{
			Node{Elite, 3},
			Node{Shop, 2},
			Node{Enemy, 6}
		},
		// 6層
		{
			Node{Enemy, 1},
			Node{Elite, 7},
			Node{Event, 4}
		},
		// 7層
		{
			Node{Treasure, 3},
			Node{Elite, 2},
			Node{Enemy, 6}
		},
		// 8層
		{
			Node{Shop, 3},
			Node{Elite, 2},
			Node{Enemy, 6}
		},
		// 9層
		{
			Node{Enemy, 2},
			Node{Elite, 2},
			Node{Event, 2}
		},
		// 10層
		{
			Node{None, 0},
			Node{Boss, 0},
			Node{None, 0}
		}
	},// Map 3
	{
		// 1層
		{
			Node{None, 0},
			Node{Enemy, 7},
			Node{None, 0}
		},
		// 2層
		{
			Node{Shop, 3},
			Node{Enemy, 2},
			Node{Event, 6}
		},
		// 3層
		{
			Node{Treasure, 3},
			Node{Enemy, 2},
			Node{Event, 6}
		},
		// 4層
		{
			Node{Event, 3},
			Node{Shop, 2},
			Node{Enemy, 6}
		},
		// 5層
		{
			Node{Treasure, 3},
			Node{Enemy, 2},
			Node{Elite, 6}
		},
		// 6層
		{
			Node{Enemy, 3},
			Node{Event, 2},
			Node{Shop, 6}
		},
		// 7層
		{
			Node{Elite, 3},
			Node{Treasure, 2},
			Node{Enemy, 6}
		},
		// 8層
		{
			Node{Shop, 3},
			Node{Enemy, 2},
			Node{Event, 6}
		},
		// 9層
		{
			Node{Enemy, 2},
			Node{Event, 2},
			Node{Treasure, 2}
		},
		// 10層
		{
			Node{None, 0},
			Node{Boss, 0},
			Node{None, 0}
		}
	},// Map 4
{
	// 1層
	{
		Node{None, 0},
		Node{Enemy, 7},
		Node{None, 0}
	},
		// 2層
		{
			Node{Enemy, 3},
			Node{Enemy, 2},
			Node{Event, 6}
		},
		// 3層
		{
			Node{Elite, 3},
			Node{Enemy, 2},
			Node{Shop, 6}
		},
		// 4層
		{
			Node{Enemy, 1},
			Node{Event, 5},
			Node{Treasure, 4}
		},
		// 5層
		{
			Node{Elite, 2},
			Node{None, 0},
			Node{Event, 2}
		},
		// 6層
		{
			Node{None, 0},
			Node{Shop, 7},
			Node{None, 0}
		},
		// 7層
		{
			Node{Enemy, 1},
			Node{Treasure, 2},
			Node{Enemy, 4}
		},
		// 8層
		{
			Node{Elite, 3},
			Node{Enemy, 2},
			Node{Shop, 6}
		},
		// 9層
		{
			Node{Enemy, 2},
			Node{Elite, 2},
			Node{Event, 2}
		},
		// 10層
		{
			Node{None, 0},
			Node{Boss, 0},
			Node{None, 0}
		}
	},
		// Map 5
	{
		// 1層
		{
			Node{None, 0},
			Node{Enemy, 7},
			Node{None, 0}
		},
		// 2層
		{
			Node{Event, 3},
			Node{Enemy, 2},
			Node{Elite, 6}
		},
		// 3層
		{
			Node{Treasure, 3},
			Node{Elite, 2},
			Node{Enemy, 6}
		},
		// 4層
		{
			Node{Shop, 3},
			Node{Enemy, 2},
			Node{Elite, 6}
		},
		// 5層
		{
			Node{Event, 3},
			Node{Shop, 2},
			Node{Enemy, 6}
		},
		// 6層
		{
			Node{Treasure, 3},
			Node{Enemy, 2},
			Node{Elite, 6}
		},
		// 7層
		{
			Node{Shop, 3},
			Node{Elite, 2},
			Node{Enemy, 6}
		},
		// 8層
		{
			Node{Event, 3},
			Node{Enemy, 2},
			Node{Elite, 6}
		},
		// 9層
		{
			Node{Treasure, 2},
			Node{Elite, 2},
			Node{Enemy, 2}
		},
		// 10層
		{
			Node{None, 0},
			Node{Boss, 0},
			Node{None, 0}
		}
	},// Map 6
{
	// 1層
	{
		Node{None, 0},
		Node{Enemy, 7},
		Node{None, 0}
	},
		// 2層
		{
			Node{Enemy, 3},
			Node{Event, 2},
			Node{Enemy, 6}
		},
		// 3層
		{
			Node{Shop, 3},
			Node{Enemy, 2},
			Node{Enemy, 6}
		},
		// 4層
		{
			Node{Enemy, 3},
			Node{Event, 2},
			Node{Treasure, 6}
		},
		// 5層
		{
			Node{Shop, 3},
			Node{Elite, 2},
			Node{Enemy, 6}
		},
		// 6層
		{
			Node{Event, 3},
			Node{Enemy, 2},
			Node{Treasure, 6}
		},
		// 7層
		{
			Node{Elite, 3},
			Node{Enemy, 2},
			Node{Elite, 6}
		},
		// 8層
		{
			Node{Shop, 1},
			Node{Elite, 5},
			Node{Enemy, 4}
		},
		// 9層
		{
			Node{Elite, 2},
			Node{Event, 2},
			Node{Elite, 2}
		},
		// 10層
		{
			Node{None, 0},
			Node{Boss, 0},
			Node{None, 0}
		}
	}
	};
	//抽選スタート！
	const int32 current_act = GameStateRules::ActIndex(getData().Layer);
	const bool valid_saved_map = (getData().selected_nodes.size() == 10)
		&& std::all_of(getData().selected_nodes.begin(), getData().selected_nodes.end(),
			[](const auto& row) { return row.size() == 3; });
	if ((getData().selected_map_act != current_act) || !valid_saved_map) {
		map_nodes = map_nodes_source[Random(0, 5)]; // 0から5の範囲でランダムに選択
		getData().selected_nodes = map_nodes; // 選択されたノードを保存
		getData().selected_map_act = current_act;
	} else {
		map_nodes = getData().selected_nodes;
	}
	getData().Index = Clamp(getData().Index, 0, 2);
	//Map生成完了！！

}

// update() メソッドの実装
void Map::update() {
	deck_mode = banner.update(getData().Deck); // バナーの更新
	if (deck_mode)return;

	const int32 floor_in_act = GameStateRules::FloorInAct(getData().Layer);
	move_x = -Clamp(floor_in_act - 2, 0, 4) * 300; // マップの移動量を計算
	vector<tuple<Node, Circle, int>> next_node; // 次のノードとその位置
	if ((floor_in_act + 1 < static_cast<int32>(map_nodes.size()))
		&& (map_nodes[floor_in_act][getData().Index].NextLayerIndex & 1)) {
		next_node.push_back(make_tuple(map_nodes[floor_in_act + 1][0], Circle{ move_x + 200 + (floor_in_act + 1) * 300, 420 + 0 * 265, 100.0 }, 0)); // 上層
	}
	if ((floor_in_act + 1 < static_cast<int32>(map_nodes.size()))
		&& (map_nodes[floor_in_act][getData().Index].NextLayerIndex & 2)) {
		next_node.push_back(make_tuple(map_nodes[floor_in_act + 1][1], Circle{ move_x + 200 + (floor_in_act + 1) * 300, 420 + 1 * 265, 100.0 }, 1)); // 中層
	}
	if ((floor_in_act + 1 < static_cast<int32>(map_nodes.size()))
		&& (map_nodes[floor_in_act][getData().Index].NextLayerIndex & 4)) {
		next_node.push_back(make_tuple(map_nodes[floor_in_act + 1][2], Circle{ move_x + 200 + (floor_in_act + 1) * 300, 420 + 2 * 265, 100.0 }, 2)); // 下層
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
				shop_se.play(); // ショップのSEを再生
				changeScene(State::Shop, 2s); // ショップに移動
			} else if (node.type == MapPointType::Boss) {
				battle_se.play(); // ボス戦のSEを再生
				getData().enemy = 2; // ボスの敵IDをセット
				changeScene(State::Battle, 2s); // ボス戦に移動
			} else if (node.type == MapPointType::Event) {
				event_se.play();
				changeScene(State::Event, 2s);
			} else if (node.type == MapPointType::Elite) {
				battle_se.play(); // エリート戦のSEを再生
				getData().enemy = 1; // エリートの敵IDをセット
				changeScene(State::Battle, 2s); // エリート戦に移動
			} else if (node.type == MapPointType::Enemy) {
				battle_se.play(); // 通常戦闘のSEを再生
				getData().enemy = 0; // 通常の敵IDをセット
				changeScene(State::Battle, 2s); // 通常戦闘に移動
			} else if (node.type == MapPointType::Treasure) {
				treasure_se.play(); // 宝箱のSEを再生
				changeScene(State::Shop, 2s); // 宝箱を開けるための戦闘に移動
			}
		}
	}

}

// draw() メソッドの実装
void Map::draw() const {
	if (deck_mode) {
		banner.draw(getData().money, getData().Layer, getData().leric); // デッキモードのバナーを描画
		return;
	}
	// 現在の層に応じた背景画像を描画
	const int32 act = GameStateRules::ActIndex(getData().Layer);
	const int32 floor_in_act = GameStateRules::FloorInAct(getData().Layer);
	background_imgs.at(act).draw();

	// 現在の層のノードを描画
	for (int i = 0; i < 10; i++) {
		for (int j = 0; j < 3; j++) {
			const Node& node = map_nodes[i][j];
			bool NextVisit = (floor_in_act + 1 == i) && map_nodes[floor_in_act][getData().Index].NextLayerIndex & (1 << j); // 次の層のインデックスを取得
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
				double node_alpha = (floor_in_act < i) ? 0.0 : 0.6; // 現在の層より上の層は半透明
				if ((node_alpha == 0.6) && (floor_in_act == i) && (getData().Index == j)) {
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
	player_icon.drawAt(move_x + 200 + floor_in_act * 300, 420 + getData().Index * 265);

	// バナーの描画
	banner.draw(getData().money, getData().Layer, getData().leric);
}

void Map::drawFadeOut(double t) const {
	draw();
	const double progress = EaseInOutExpo(t);
	loading_icon.draw(-1920 * Math::Lerp(1.0, 0.0, progress), 0);
}
