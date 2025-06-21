#ifndef MAP_HPP
#define MAP_HPP

#include <Siv3D.hpp>
#include <vector>
#include <string>
#include "common.hpp"

enum class MapPointType {
	Boss,
	Elite,
	Event,
	Shop,
	Enemy,
	Treasure,
	None,
};

struct Node {
	MapPointType type = MapPointType::None;
	bool isVisited = false;
	int NextLayerIndex = 0; // 次の層のインデックス (1:上層, 2:中層, 4:下層)
};

// Mapシーンを表すクラス
class Map : public App::Scene {
	//画像の読み込み
	std::vector<Texture> background_imgs;
	Texture enemy_icon_0 = Texture{ U"../../image/map_enemy_teki.png" };
	Texture enemy_icon_1 = Texture{ U"../../image/map_enemy_teki_1.png" };
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
	
	//Map情報
	std::vector<std::vector<Node>> map_nodes; // 各地点の情報を保持するノードの配列
	//Map情報(抽選用)
	std::vector<std::vector<std::vector<Node>>> map_nodes_source; // 抽選用のノードの配列
	//現在地
	int currentLayerIndex = 0; // 現在の層のインデックス (0から2)
	int mode = 0; // モード (0:待機, 1:移動中, 2:イベント発生中)
	


	
	// プレイヤーの現在位置
	int m_playerCurrentLayer = 0; // 現在いる層 (0から29)
	int m_playerCurrentPoint = 0; // 現在いる層内の地点インデックス (0から2)

	// ... (既存のイベント管理用のメンバー変数など) ...

	// ヘルパー関数 (Map.cppで実装)
	void BuildMapFromPatterns(); // 選ばれたパターンから実際の30層マップを構築する
	void UpdateAccessiblePoints(); // アクセス可能な地点を更新
	void HandleCurrentPointAction(MapPointType type); // クリックされた地点の種類に応じたアクション

public:
	Map(const InitData& init);
	~Map() = default;

	void update() override;
	void draw() const override;
};

#endif // MAP_HPP