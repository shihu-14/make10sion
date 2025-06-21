#pragma once
#include <Siv3D.hpp>

enum class MapPointType {
	Boss,
	Elite,
	Event,
	Shop,
	Enemy,
	Treasure,
	None,
};

struct MapPoint {
	MapPointType type = MapPointType::None; // この地点の種類（初期値はNone）
	bool isVisited = false;                 // プレイヤーがこの地点を訪問済みか？
	bool isAccessible = false;              // プレイヤーが現在この地点に移動可能か？

	// この地点がプレイヤーの操作でクリックされる時の「見た目の位置」です。
	// 実際に移動距離を計算するわけではなく、あくまで画面に描画する時の座標です。
	Vec2 drawPos = { 0, 0 };
};

// 各層間の接続情報を表す構造体
// ある層の地点から次の層の地点へ繋がるかどうか
struct ConnectionInfo {
	// 例: sourceIndex = 0, targetIndex = 1 なら、現在の層の0番目の地点から次の層の1番目の地点へ
	int sourceIndex; // 接続元の地点インデックス (0, 1, 2)
	int targetIndex; // 接続先の地点インデックス (0, 1, 2)
	bool isConnected = true; // この接続が有効か（線がつながっているか）
};

// マップの1～10層までの「配置パターン」を定義する構造体
struct MapPattern {
	// 各層の地点の種類と、その層から次の層への接続情報を含む
	// 層のインデックスは 0～9 に対応
	Array<Array<MapPointType>> layerPointTypes; // [層インデックス][地点インデックス] = 地点種類
	Array<Array<ConnectionInfo>> layerConnections; // [層インデックス][接続情報リスト]

	// コンストラクタでサイズを初期化
	MapPattern() {
		layerPointTypes.resize(10); // 10層
		layerConnections.resize(9); // 0層-1層から8層-9層への接続
		for (int i = 0; i < 10; ++i) {
			layerPointTypes[i].resize(3); // 各層に最大3地点
		}
	}
};

// Mapシーンを表すクラス
class Map : public App::Scene {
private:
	Texture m_background;
	Texture m_boss;
	Texture m_elite;
	Texture m_event;
	Texture m_shop;
	Texture m_enemy;
	Texture m_treasure;
	Font m_playerInfoFont;
	Font m_buttonFont;
	Audio m_mapBGM;

	double m_currentAlpha = 0.0;

	// マップ全体の情報
	// 10層ごとのマップパターンを格納する配列 (30層はこれらパターンを組み合わせる)
	Array<MapPattern> m_stageLayouts; // ここに6つのパターンを定義する

	// 現在の30層マップで、どのパターンが選ばれたかを保持
	Array<int> m_currentMapPattern; // サイズ3 (0-9層, 10-19層, 20-29層の各ブロックのパターンインデックス)

	// 各層の実際の地点データを保持
	Array<Array<MapPoint>> m_allMapPoints; // [層インデックス][地点インデックス] = 実際の地点データ (30層分)

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
namespace GameConstants {
	const FilePath BossImageFilePath = U"example/texture/boss.png";
	const FilePath EliteImageFilePath = U"example/texture/elite.png";
	const FilePath EventImageFilePath = U"example/texture/event.png";
	const FilePath ShopImageFilePath = U"example/texture/shop.png";
	const FilePath EnemyImageFilePath = U"example/texture/teki.png";
	const FilePath TreasureImageFilePath = U"example/texture/treasure.png";
}
