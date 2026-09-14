# make10sion / Arithmancer

数値と演算子のカードを盤面に並べ、式の結果を攻撃力・防御力へ変える C++ / OpenSiv3D 製のカードゲームです。カードの配置、数式の成立、敵の行動予測を一画面に集約し、短い判断の積み重ねで戦況を組み立てます。

## デモ

- [戦闘の一連の操作を見る](docs/movies/battle-flow.mp4) — 配られた手札を盤面へ配置し、`=` で攻防を確定してターンを終えるまで
- [戦闘中のデッキ表示を見る](docs/movies/deck-view.mp4) — 戦闘画面からデッキ内容を確認する操作

> 動画は無音の短尺 MP4 です。GitHub ではリンク先で再生できます。

## どんなゲームか

![タイトル画面](docs/images/title.png)

1. 数値・演算子・特殊効果を持つカードが手札に配られます。
2. カードを 7 × 6 の盤面へドラッグして、横一列の数式を組み立てます。
3. 上段は攻撃、下段は防御として集計され、`=` を押すと敵と同時に解決します。
4. 敵、ショップ、イベントなどのマップ上の選択を通じ、デッキと盤面を強化して先へ進みます。

数値を大きくするだけではなく、演算子の順序、行ごとの倍率、攻撃／防御の割り当て、手札と盤面の空きに応じて最適解が変わることを狙っています。

## 画面構成

### タイトル

ゲームの開始点です。最初の数秒で世界観と入力の導線が伝わるよう、戦闘背景と大きな開始ボタンを中心に構成しています。

### マップ — 次の強化方針を選ぶ

![マップ画面](docs/images/map.png)

敵・エリート・ショップ・イベント・宝箱・ボスから次の進行先を選びます。戦闘だけでなく、デッキを伸ばすか、盤面を解放するか、リスクの高い戦闘に進むかを判断する画面です。

### 戦闘 — カード配置と数式の解決

戦闘は、カードを配置している間に計算結果をフィードバックし、確定操作で敵味方の攻防を解決します。動画では、手札の配布から配置、解決、ターン終了までを確認できます。

- [戦闘の操作動画](docs/movies/battle-flow.mp4)
- [戦闘中のデッキ表示動画](docs/movies/deck-view.mp4)

### ショップ — デッキを再設計する

![ショップ画面](docs/images/shop.png)

カードとレリックを購入して、次の戦闘に向けたデッキを調整します。所持金・カードの役割・盤面との相性を同時に考える、ランの中長期的な判断ポイントです。

## 実装で重視した点

- **盤面計算をゲーム描画から分離** — `BoardCalculationRules` が数式の妥当性、演算子の優先順位、行ごとの集計を扱います。表示と入力から独立させ、計算規則を追跡・検証しやすくしています。
- **ドラッグ操作とカードの所有状態を一貫させる** — カードは山札・手札・盤面・捨て札のいずれか一つにだけ属し、ドラッグ中も状態遷移を明示します。盤面への配置、入れ替え、無効なドロップからの復帰で状態が食い違わないことを意識しています。
- **戦闘レイアウトを集約** — 戦闘画面の座標や表示ルールを `BattleLayoutRules` にまとめ、カード、敵、ステータス表示の調整を局所化しています。
- **Windows / macOS でゲーム内容を共有** — `src/`、`image/`、`audio/` は両 OS 共通です。OS ごとの差分は Visual Studio / Xcode のプロジェクト設定と Siv3D 実行環境に閉じています。

## ディレクトリ構成

```text
make10sion/
├── src/           # Windows / macOS 共通のゲームコード
├── image/         # 共通画像
├── audio/         # 共通音声
├── docs/
│   ├── images/    # README 用の静止画（title.png / map.png / shop.png）
│   └── movies/    # README 用の動画（battle-flow.mp4 / deck-view.mp4）
├── tests/         # 戦闘カード操作と macOS 実行補助のテスト
├── windows/       # Visual Studio プロジェクトと Windows 用 Siv3D 実行環境
├── macos/         # Xcode プロジェクトと macOS 用 Siv3D 実行環境
└── .vscode/       # macOS 用 Build / Run タスク
```

## 開発環境とビルド

Siv3D SDK 本体の `include` と `lib` はリポジトリに含まれていません。各 OS で OpenSiv3D v0.6.16 公式 SDK を用意してください。実行に必要な OS 別エンジンリソースと、Windows 版の SoundTouch ランタイム DLL は各 `App` フォルダに含まれます。

### Windows

必要なものは、Windows 10 / 11（64-bit）、Visual Studio 2022 の「C++ によるデスクトップ開発」、OpenSiv3D v0.6.16 SDK です。環境変数 `SIV3D_0_6_16` には公式 SDK のルートフォルダを指定します。

1. `windows/make10sion.sln` を Visual Studio 2022 で開きます。
2. 構成を `Debug | x64` または `Release | x64` にします。
3. 「ローカル Windows デバッガー」で実行します。

実行時の作業ディレクトリは `windows/App` です。ゲームコードは `../../image` と `../../audio` から共有素材を参照します。

### macOS

必要なものは、OpenSiv3D v0.6.16 macOS Project Templates、Xcode 14.3 以降と Command Line Tools、Visual Studio Code です。Siv3D v0.6.16 は Apple Silicon へネイティブ対応していないため、Apple Silicon Mac では Rosetta 2 を用いて `x86_64` としてビルド・実行します。

公式 macOS テンプレートを展開し、リポジトリが次の位置になるよう clone してください。Xcode プロジェクトはこの相対位置から SDK の `include` と `lib` を参照します。

```text
siv3d_v0.6.16_macOS/
├── include/
├── lib/
└── examples/
    └── make10sion/   # このリポジトリ
```

VS Code では次のタスクを使えます。

- `Siv3D: Build macOS`
- `Siv3D: Run macOS`
- `Siv3D: Run Midgame Debug`
- `Siv3D: Build and Run macOS`
- `Siv3D: Test Battle Card Interactions`

ターミナルから実行する場合は、リポジトリのルートで次を実行します。

```bash
./macos/build-debug.sh
./macos/run-debug.sh
```

`./macos/run-debug.sh --check` は、アプリを起動せずにビルド由来情報だけを検証します。
