#ifndef MAP_HPP
#define MAP_HPP

#include <Siv3D.hpp>
#include <vector>
#include <string>
#include "common.hpp"
#include "Banner.hpp"

// Mapシーンを表すクラス
class Map : public App::Scene {
	//画像の読み込み
	std::vector<Texture> background_imgs;
	Texture enemy_icon = Texture{ U"../../image/map_teki.png" };
	Texture enemy_icon_1 = Texture{ U"../../image/map_teki_1.png" };
	Texture elite_icon = Texture{ U"../../image/map_elite.png" };
	Texture elite_icon_1 = Texture{ U"../../image/map_elite_1.png" };
	Texture boss_icon = Texture{ U"../../image/map_boss.png" };
	Texture boss_icon_1 = Texture{ U"../../image/map_boss_1.png" };
	Texture event_icon = Texture{ U"../../image/map_event.png" };
	Texture event_icon_1 = Texture{ U"../../image/map_event_1.png" };
	Texture shop_icon = Texture{ U"../../image/map_shop.png" };
	Texture shop_icon_1 = Texture{ U"../../image/map_shop_1.png" };
	Texture treasure_icon = Texture{ U"../../image/map_treasure.png" };
	Texture treasure_icon_1 = Texture{ U"../../image/map_treasure_1.png" };
	Texture arrow_icon = Texture{ U"../../image/map_arrow.png" };
	
	//Map情報
	std::vector<std::vector<Node>> map_nodes; // 各地点の情報を保持するノードの配列
	//Map情報(抽選用)
	std::vector<std::vector<std::vector<Node>>> map_nodes_source; // 抽選用のノードの配列

	int hovered_index = -1; // ホバーしているノードのインデックス
	//Mapの移動
	int move_x = 0;
	//移動先決定後

	Banner banner; // バナー表示用
	bool deck_mode = false; // デッキモードのフラグ
public:
	Map(const InitData& init);

	void update() override;
	void draw() const override;
};

#endif // MAP_HPP