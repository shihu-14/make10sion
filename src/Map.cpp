#include "Map.hpp" // Map.hpp をインクルード

// MapPointTypeのメンバーを直接使えるようにする
using enum MapPointType;

// Mapクラスのコンストラクタの実装
Map::Map(const InitData& init) :
	App::Scene<GameState>(init), // 親クラスApp::Sceneのコンストラクタを呼び出す

	// 背景画像の読み込み
	m_background(GameConstants::BackgroundAssetPath),

	// 地点画像の読み込み
	m_pointEnemyImage(GameConstants::PointEnemyImagePath),
	m_pointTreasureImage(GameConstants::PointTreasureImagePath),
	m_pointEliteImage(GameConstants::PointEliteImagePath),
	m_pointBossImage(GameConstants::PointBossImagePath),
	m_pointShopImage(GameConstants::PointShopImagePath),
	m_pointEventImage(GameConstants::PointEventImagePath),

	// フォントの作成
	m_playerInfoFont(GameConstants::PlayerInfoFontSize),
	m_buttonFont(GameConstants::ButtonFontSize, Typeface::Bold),

	// BGMの読み込み
	m_mapBGM(GameConstants::MapBGMPath, Loop::Yes),

	// 画面下部ボタンの領域定義
	m_battleButtonRect(Arg::center(Window::Center().x - 200, Window::Height() - 150), 180, 60),
	m_restButtonRect(Arg::center(Window::Center().x, Window::Height() - 150), 180, 60),
	m_shopButtonRect(Arg::center(Window::Center().x + 200, Window::Height() - 150), 180, 60)
{
	// BGMの再生開始
	if (m_mapBGM.isValid() && !m_mapBGM.isPlaying()) {
		m_mapBGM.play();
	}

	// プレイヤーの初期ステータス
	m_playerScore = 0;
	m_playerFloor = 1; // 階層は1から数える

	// === 6つのマップ配置パターン (1～10層ブロック) の定義 ===
	// Arrayに直接MapPatternオブジェクトを初期化リストで格納します
	m_stages = {
		// --- パターン1 --- (
		MapPattern{
			// layerPointTypes (10層分の地点タイプ)
			{
				{ None, Enemy, None },     // Layer 0 (1層目): 固定されるが、ここにはパターン定義として入れておく
				{ Enemy, Event, Enemy },     // Layer 1 (2層目)
				{ Elite, Enemy, Enemy }, // Layer 2 (3層目)
				{ Enemy, Treasure, Event },    // Layer 3 (4層目)
				{ Elite, Enemy, None },    // Layer 4 (5層目)
				{ Enemy, Shop, Enemy },     // Layer 5 (6層目)
				{ Elite, Treasure, Enemy },     // Layer 6 (7層目)
				{ Shop, Enemy, Enemy },    // Layer 7 (8層目)
				{ Enemy, Elite, Event }, // Layer 8 (9層目)
				{ None, Boss, None }       // Layer 9 (10層目): 固定されるが、ここにはパターン定義として入れておく
			},
		// layerConnections (9層間分の接続情報)
		{
			// Layer 0->1 (1層->2層)
			{ {0,0}, {0,1}, {0,2} }, // 1層の0から2層の0, 1へ
			// Layer 1->2 (2層->3層)
			{ {0,1},{2,1} }, // 2層の0から3層の0へ, 2層の1から3層の2へ
			// Layer 2->3 (3層->4層)
			{ {1,0},{1,2} }, // 3層の0から4層の0へ
			// Layer 3->4 (4層->5層)
			{  {0,1},{2,1}}, // 4層の0から5層の0へ, 4層の1から5層の0,1へ
			// Layer 4->5 (5層->6層)
			{ {1,0},{1,2} },
			// Layer 5->6 (6層->7層)
			{ {0,1},{2,1} },
			// Layer 6->7 (7層->8層)
			{ {1,0},{1,2} },
			// Layer 7->8 (8層->9層)
			{  },
			// Layer 8->9 (9層->10層)
			{ {0,1},{2,1} }
		}
	}, // --- パターン1 終わり ---

		// --- パターン2 --- 
		MapPattern{
			{
				{ None,Enemy , None },
				{ Enemy, Elite, Event },
				{ Event, Enemy, Elite },
				{ Enemy, Elite, Shop },
				{ Elite, Shop, Enemy },
				{ Enemy, Elite, Event },
				{ Treasure, Elite, Enemy },
				{ Shop, Elite, Enemy },
				{ Enemy, Elite, Event },
				{ None, Boss, None }
			},
			{
			{ {0,0}, {0,1}, {0,2} },
			{ {0,1},{2,1} },
			{ {1,0},{1,2} },
			{  {0,1},{2,1}},
			{ {1,0},{1,2} },
			{ {0,1},{2,1} },
			{ {1,0},{1,2} },
			{  {0,1},{2,1}},
			{ {0,1},{2,1} }
			}
		}, // --- パターン2 終わり ---

		// --- パターン3 --- 
		MapPattern{
			{
				{ None,Enemy , None },
				{ Shop, Enemy, Event },
				{ Treasure, Enemy, Event },
				{ Event, Shop, Enemy },
				{ Treasure, Enemy, Elite },
				{ Enemy, Event, Shop },
				{ Elite, Treasure, Enemy },
				{ Shop, Enemy, Event },
				{ Enemy, Event, Treasure },
				{ None, Boss, None }
			},
			{
				{ {0,0}, {0,1}, {0,2} },
				{ {1,0},{1,2} },
			{ {0,1},{2,1} },
			{ {1,0},{1,2} },
			{  {0,1},{2,1}},
			{ {1,0},{1,2} },
			{ {0,1},{2,1} },
			{ {1,0},{1,2} },
			{ {0,1},{2,1} }
			}
		}, // --- パターン3 終わり ---

		// --- パターン4 --- まだできてないです
		MapPattern{
			{
				{ None,Enemy , None },
				{ Shop, Enemy, Event },
				{ Treasure, Enemy, Event },
				{ Event, Shop, Enemy },
				{ Treasure, Enemy, Elite },
				{ Enemy, Event, Shop },
				{ Elite, Treasure, Enemy },
				{ Shop, Enemy, Event },
				{ Enemy, Event, Treasure },
				{ None, Boss, None }
			},
			{
				{ {0,0}, {0,1}, {0,2} },
				{ {1,0},{1,2} },
			{ {0,1},{2,1} },
			{ {1,0},{1,2} },
			{  {0,1},{2,1}},
			{ {1,0},{1,2} },
			{ {0,1},{2,1} },
			{ {1,0},{1,2} },
			{ {0,1},{2,1} }
			}
		}, // --- パターン4 終わり ---

		// --- パターン5 --- (
		MapPattern{
			{
				{None,Enemy , None },
				{ Event, Enemy,Elite },
				{ Treasure, Elite, Enemy },
				{ Shop, Enemy, Elite },
				{ Event, Shop, Enemy },
				{ Treasure, Enemy, Elite },
				{ Shop, Elite, Enemy },
				{ Event, Enemy, Elite },
				{ Treasure, Elite, Enemy },
				{ None, Boss, None }
			},
			{
				{ {0,0}, {0,1}, {0,2} },
				{ {1,0},{1,2} },
			{ {0,1},{2,1} },
			{ {1,0},{1,2} },
			{  {0,1},{2,1}},
			{ {1,0},{1,2} },
			{ {0,1},{2,1} },
			{ {1,0},{1,2} },
			{ {0,1},{2,1} }
			}
		}, // --- パターン5 終わり ---

		// --- パターン6 --- 
		MapPattern{
			{
				{ None,Enemy , None  },
				{ Enemy, Event, Enemy },
				{ Shop, Enemy, Enemy },
				{ Enemy, Event, Treasure },
				{ Shop, Elite, Enemy},
				{ Event, Enemy, Treasure },
				{ Elite, Enemy, Elite },
				{ Shop, Elite, Enemy},
				{ Elite, Event, Elite },
				{None, Boss, None }
			},
			{
				{ {0,0}, {0,1}, {0,2} },
				{ {1,0},{1,2} },
			{ {0,1},{2,1} },
			{ {1,0},{1,2} },
			{  {0,1},{2,1}},
			{ {1,0},{1,2} },
			{ {0,1},{2,1} },
			{ {1,0},{1,2} },
			{ {0,1},{2,1} }
			}
		} // --- パターン6 終わり ---
	};

	// 選ばれたパターンから実際の30層マップを構築する
	BuildMapFromPatterns();

	// プレイヤーの初期位置と状態
	m_playerCurrentLayer = 0;
	m_playerCurrentPoint = 0;
	m_allMapPoints[m_playerCurrentLayer][m_playerCurrentPoint].isVisited = true;
	m_allMapPoints[m_playerCurrentLayer][m_playerCurrentPoint].isAccessible = true;

	// 初期アクセス可能地点を更新
	UpdateAccessiblePoints();
}

// update() メソッドの実装
void Map::update() {
	m_currentAlpha = Min(m_currentAlpha + Scene::DeltaTime() * 0.5, 1.0);
	m_currentTimeInMap += Scene::DeltaTime(); // マップシーンでの経過時間を更新



	// 地点のクリック判定とシーン遷移
	m_selectedPointIndex.reset(); // 毎フレーム、マウスオーバー中の地点をリセット

	// 描画範囲の層のみクリック可能にする
	int startLayerForInteraction = Max(0, m_playerCurrentLayer - 1);
	int endLayerForInteraction = Min(29, m_playerCurrentLayer + 2);

	for (int layerIdx = startLayerForInteraction; layerIdx <= endLayerForInteraction; ++layerIdx) {
		for (int pointIdx = 0; pointIdx < m_allMapPoints[layerIdx].size(); ++pointIdx) {
			MapPoint& point = m_allMapPoints[layerIdx][pointIdx];

			// Noneタイプの地点は操作できない
			if (point.type == None) continue;

			// 地点画像と同じくらいのクリック可能な円形領域（画像サイズに合わせて調整）
			RectF clickableArea = Circle(point.drawPos, 25);

			if (point.isAccessible && clickableArea.mouseOver()) {
				m_selectedPointIndex = pointIdx; // マウスが乗っている地点のインデックスを記録
				Cursor::RequestStyle(CursorStyle::Hand); // カーソルを手の形に

				if (clickableArea.leftClicked()) {
					// 同じ層の地点を選択した場合（再選択）
					if (layerIdx == m_playerCurrentLayer) {
						HandleCurrentPointAction(point.type); // 休眠、ショップ、宝箱、再戦など
						// 同じ層の戦闘系地点を再選択した場合も戦闘シーンへ
						if (point.type == Enemy || point.type == Elite || point.type == Boss) {
							if (m_mapBGM.isValid() && m_mapBGM.isPlaying()) { m_mapBGM.stop(0.5s); }
							changeScene(GameState::Battle, 0.5s);
						}
					}
					// 次の層の地点を選択した場合（移動）
					else if (layerIdx == m_playerCurrentLayer + 1) {
						bool canMoveToNextLayer = false;

						int currentBlockNum = m_playerCurrentLayer / 10;
						int relativeLayerIdx = m_playerCurrentLayer % 10;

						// 接続の判定ロジック
						if (relativeLayerIdx == 0) { // 1層,11層,21層からの移動（単一点）
							// 常に0番目の地点から次の層の0番目の地点に繋がる
							canMoveToNextLayer = (m_playerCurrentPoint == 0 && pointIdx == 0);
						}
						else if (relativeLayerIdx == 9) { // 10層,20層,30層からは移動できない
							canMoveToNextLayer = false;
						}
						else { // 2～9層、12～19層、22～29層からの移動
							const MapPattern& currentPattern = m_stages[m_currentMapPattern[currentBlockNum]];
							// layerConnectionsは0-8層の定義なので、relativeLayerIdx-1でアクセス
							for (const auto& conn : currentPattern.layerConnections[relativeLayerIdx - 1]) {
								if (conn.sourceIndex == m_playerCurrentPoint && conn.targetIndex == pointIdx && conn.isConnected) {
									canMoveToNextLayer = true;
									break;
								}
							}
						}

						if (canMoveToNextLayer) {
							// プレイヤーの位置を更新し、訪問済みとする
							m_playerCurrentLayer = layerIdx;
							m_playerCurrentPoint = pointIdx;
							m_allMapPoints[m_playerCurrentLayer][m_playerCurrentPoint].isVisited = true;
							UpdateAccessiblePoints();
							System::Print(U"{}層 {}地点へ移動！".format(m_playerCurrentLayer + 1, m_playerCurrentPoint));

							// シーン遷移（新しい地点タイプに対応）
							if (m_mapBGM.isValid() && m_mapBGM.isPlaying()) { m_mapBGM.stop(0.5s); }

							if (point.type == Enemy || point.type == Elite || point.type == Boss) {
								changeScene(GameState::Battle, 0.5s);
							}
							else if (point.type == Shop) {
								changeScene(GameState::Shop, 0.5s);
							}
							else if (point.type == Event) {
								System::Print(U"イベントマスに止まりました！抽選を開始します。");
								int diceRoll = Random(0, 99); // 0から99までの乱数を生成 (合計100)

								if (diceRoll < 10) { // 0-9 (10%)
									System::Print(U"→ エネミーが出現！");
									changeScene(GameState::Battle, 0.5s); // エネミーは戦闘シーンへ
								}
								else if (diceRoll < 20) { // 10-19 (10%)
									System::Print(U"→ エリートが出現！");
									changeScene(GameState::Battle, 0.5s); // エリートも戦闘シーンへ
								}
								else if (diceRoll < 30) { // 20-29 (10%)
									System::Print(U"→ ショップが出現！");
									changeScene(GameState::Shop, 0.5s); // ショップシーンへ
								}
								else if (diceRoll < 40) { // 30-39 (10%)
									System::Print(U"→ 宝箱を発見！");
									HandleCurrentPointAction(MapPointType::Treasure); // 宝箱はここで処理（スコア加算など）
								}
								else { // 40-99 (60%)
									System::Print(U"→ 特殊イベントが発生！");
									changeScene(GameState::Event, 0.5s); // 「何かをもらう」イベントシーンへ
								}
							}
							else if (point.type == Treasure) {
								HandleCurrentPointAction(point.type); // 宝箱はマップ上で直接処理
							}
						}
						else {
							System::Print(U"その道はつながっていません！");
						}
					}
				}
			}
		}
	}

	// カーソルをデフォルトに戻す
	if (!m_selectedPointIndex.has_value()) { // どの地点にもマウスが乗っていない場合
		Cursor::RequestStyle(CursorStyle::Arrow);
	}

}

// draw() メソッドの実装
void Map::draw() const {
	m_background.draw();

	// プレイヤー情報描画
	m_playerInfoFont(U"スコア: {}", m_playerScore).draw(20, 20, Palette::White);
	m_playerInfoFont(U"階層: {}F", m_playerCurrentLayer + 1).draw(20, 70, Palette::White);
	m_playerInfoFont(U"現在の地点: {}".format(m_playerCurrentPoint)).draw(20, 120, Palette::Cyan);

	// 描画範囲の計算
	int startLayerForDrawing = Max(0, m_playerCurrentLayer - 1);
	int endLayerForDrawing = Min(29, m_playerCurrentLayer + 2); // 30層なので最大29

	// --- 接続線の描画 ---
	for (int layerIdx = startLayerForDrawing; layerIdx <= endLayerForDrawing; ++layerIdx) {
		if (layerIdx < 29) { // 最終層の手前まで
			int currentBlockNum = layerIdx / 10;
			int relativeLayerIdx = layerIdx % 10;

			// 接続は、(1層,11層,21層) または (10層,20層,30層) の単一点層には存在しない
			// かつ、そのブロックの最終層 (9層) からの接続も存在しない
			if (relativeLayerIdx > 0 && relativeLayerIdx < 9) { // 2～9層、12～19層、22～29層の場合
				const MapPattern& currentPattern = m_stages[m_currentMapPattern[currentBlockNum]];
				// layerConnectionsは0-8層の定義なので、relativeLayerIdx-1でアクセス
				for (const auto& conn : currentPattern.layerConnections[relativeLayerIdx - 1]) {
					if (conn.isConnected) {
						Vec2 startPos = m_allMapPoints[layerIdx][conn.sourceIndex].drawPos;
						Vec2 endPos = m_allMapPoints[layerIdx + 1][conn.targetIndex].drawPos;
						Line(startPos, endPos).draw(3, Palette::Green.withAlpha(m_currentAlpha));
					}
					else {
						Vec2 startPos = m_allMapPoints[layerIdx][conn.sourceIndex].drawPos;
						Vec2 endPos = m_allMapPoints[layerIdx + 1][conn.targetIndex].drawPos;
						Line(startPos, endPos).draw(1, Palette::Gray.withAlpha(m_currentAlpha * 0.5));
					}
				}
			}
		}
	}

	// --- 各層の地点の描画（画像を使用） ---
	for (int layerIdx = startLayerForDrawing; layerIdx <= endLayerForDrawing; ++layerIdx) {
		for (int pointIdx = 0; pointIdx < m_allMapPoints[layerIdx].size(); ++pointIdx) {
			const MapPoint& point = m_allMapPoints[layerIdx][pointIdx];

			if (point.type == None) continue; // Noneタイプの地点は描画しない

			// 描画する画像を選択（6種類に対応）
			const Texture* pointImage = nullptr;
			if (point.type == Enemy) { pointImage = &m_pointEnemyImage; }
			else if (point.type == Treasure) { pointImage = &m_pointTreasureImage; }
			else if (point.type == Elite) { pointImage = &m_pointEliteImage; }
			else if (point.type == Boss) { pointImage = &m_pointBossImage; }
			else if (point.type == Shop) { pointImage = &m_pointShopImage; }
			else if (point.type == Event) { pointImage = &m_pointEventImage; }

			if (pointImage && pointImage->isValid()) {
				pointImage->drawAt(point.drawPos, ColorF(1.0, m_currentAlpha)); // 透明度を適用
			}

			// プレイヤーが現在いる地点にマークを表示する
			if (layerIdx == m_playerCurrentLayer && pointIdx == m_playerCurrentPoint) {
				Circle(point.drawPos, 20).drawFrame(4, Palette::Yellow.withAlpha(m_currentAlpha)); // 太い黄色い枠
			}
			// アクセス可能な地点に強調表示
			else if (point.isAccessible) {
				Circle(point.drawPos, 22).drawFrame(2, Palette::Cyan.withAlpha(m_currentAlpha * 0.7));
			}
			// 訪問済みだがアクセス不可能（過去の地点）は半透明で表示
			else if (point.isVisited) {
				if (pointImage && pointImage->isValid()) {
					pointImage->drawAt(point.drawPos, ColorF(0.5, m_currentAlpha));
				}
			}
		}
	}

	RectF(Scene::Size()).draw(ColorF(0.0, 0.0, 0.0, 1.0 - m_currentAlpha)); // フェードイン演出
}

// ヘルパー関数（Mapクラスのプライベートメソッドとして定義）
// Map.cppのどこかに実装してください
void Map::UpdateAccessiblePoints() {
	// まず全ての地点のアクセス可能性をfalseにする
	for (auto& layer : m_allMapPoints) {
		for (auto& point : layer) {
			point.isAccessible = false;
		}
	}

	// 現在プレイヤーがいる地点は常にアクセス可能
	m_allMapPoints[m_playerCurrentLayer][m_playerCurrentPoint].isAccessible = true;

	// 現在の層から次の層への接続をたどり、アクセス可能にする
	if (m_playerCurrentLayer < 29) { // 最終層(29)でなければ
		int currentBlockNum = m_playerCurrentLayer / 10;
		int relativeLayerIdx = m_playerCurrentLayer % 10;

		// 次の層の地点をアクセス可能にするロジック
		if (relativeLayerIdx == 0) { // 1層,11層,21層からの移動（単一点）
			// 常に0番目の地点から次の層の0番目の地点に繋がる
			m_allMapPoints[m_playerCurrentLayer + 1][0].isAccessible = true;
		}
		else if (relativeLayerIdx == 9) { // 10層,20層,30層からの移動はない
			// ここからは移動できないので、何もアクセス可能にしない
		}
		else { // 2～9層、12～19層、22～29層からの移動
			const MapPattern& currentPattern = m_stages[m_currentMapPattern[currentBlockNum]];
			// layerConnectionsは0-8層の定義なので、relativeLayerIdx-1でアクセス
			for (const auto& conn : currentPattern.layerConnections[relativeLayerIdx - 1]) {
				if (conn.isConnected && conn.sourceIndex == m_playerCurrentPoint && conn.targetIndex < m_allMapPoints[m_playerCurrentLayer + 1].size()) {
					m_allMapPoints[m_playerCurrentLayer + 1][conn.targetIndex].isAccessible = true;
				}
			}
		}
	}
}
// Map.cpp 内の BuildMapFromPatterns() 関数

void Map::BuildMapFromPatterns() {
	m_allMapPoints.resize(30);

	m_currentMapPattern.resize(3);
	m_currentMapPattern[0] = Random(0, (int)m_stages.size() - 1);
	m_currentMapPattern[1] = Random(0, (int)m_stages.size() - 1);
	m_currentMapPattern[2] = Random(0, (int)m_stages.size() - 1);

	double baseMapX = 150.0;
	double layerSpacingX = 100.0;
	double baseMapY = 150.0;
	double pointSpacingY = (Scene::Height() - baseMapY * 2) / 2.0;

	for (int layerIdx = 0; layerIdx < 30; ++layerIdx) {
		m_allMapPoints[layerIdx].resize(3);

		int blockNum = layerIdx / 10;
		int relativeLayerIdx = layerIdx % 10;

		const MapPattern& currentPattern = m_stages[m_currentMapPattern[blockNum]];

		// === 地点タイプの割り当て === (ここは変更なし)
		if (relativeLayerIdx == 0) {
			m_allMapPoints[layerIdx][0].type = Enemy;
			m_allMapPoints[layerIdx][1].type = None;
			m_allMapPoints[layerIdx][2].type = None;
		}
		else if (relativeLayerIdx == 9) {
			m_allMapPoints[layerIdx][0].type = Boss;
			m_allMapPoints[layerIdx][1].type = None;
			m_allMapPoints[layerIdx][2].type = None;
		}
		else {
			for (int pointIdx = 0; pointIdx < 3; ++pointIdx) {
				m_allMapPoints[layerIdx][pointIdx].type = currentPattern.layerPointTypes[relativeLayerIdx][pointIdx];
			}
		}

		// === 各地点の描画座標の設定 === (変更なし)
		double currentLayerX = baseMapX + layerIdx * layerSpacingX;
		m_allMapPoints[layerIdx][0].drawPos = { currentLayerX, baseMapY + pointSpacingY * 0 };
		m_allMapPoints[layerIdx][1].drawPos = { currentLayerX, baseMapY + pointSpacingY * 1 };
		m_allMapPoints[layerIdx][2].drawPos = { currentLayerX, baseMapY + pointSpacingY * 2 };

		// 初期状態を設定 (変更なし)
		for (int pointIdx = 0; pointIdx < 3; ++pointIdx) {
			m_allMapPoints[layerIdx][pointIdx].isVisited = false;
			m_allMapPoints[layerIdx][pointIdx].isAccessible = false;
		}
	}
}

// 現在選択している地点に応じた行動を処理する関数
// （これは、クリックされた地点が現在のステージの地点だった場合）
void Map::HandleCurrentPointAction(MapPointType type) {
	if (type == Shop) {

		// changeScene(GameState::Shop, 0.5s); // 必要なら専用シーンへ
	}
	else if (type == Treasure) {

		// 宝箱は一度開けたらもう報酬なしにするなどの処理も追加可能
		// m_allMapPoints[m_playerCurrentLayer][m_playerCurrentPoint].isVisited = true;
	}
	else if (type == Event) {

		// changeScene(GameState::Event, 0.5s); // 再度イベントシーンへ遷移など
	}
}
