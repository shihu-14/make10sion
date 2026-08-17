# make10sion

OpenSiv3D v0.6.16で開発しているゲームです。ゲームコードと素材はWindowsとmacOSで共有し、OS固有のプロジェクトとSiv3D実行環境だけを分離しています。

## ディレクトリ構成

```text
make10sion/
├── src/       # Windows / macOS共通のゲームコード
├── image/     # 共通画像
├── audio/     # 共通音声
├── windows/   # Visual StudioプロジェクトとWindows用Siv3D実行環境
├── macos/     # XcodeプロジェクトとmacOS用Siv3D実行環境
└── .vscode/   # VS CodeのmacOS用Build / Runタスク
```

Siv3D SDK本体の`include`と`lib`はこのリポジトリに含まれていません。それぞれのOSでOpenSiv3D v0.6.16公式SDKを用意してください。アプリ実行に必要なOS別エンジンリソースとWindows版SoundTouchランタイムDLLは、各`App`フォルダに含めています。

## Windows

### 必要なもの

- Windows 10または11（64-bit）
- Visual Studio 2022（「C++によるデスクトップ開発」）
- OpenSiv3D v0.6.16 SDK
- Windows環境変数`SIV3D_0_6_16`（公式SDKのルートフォルダを指定）

### ビルドと実行

1. `windows/make10sion.sln`をVisual Studio 2022で開きます。
2. 構成を`Debug | x64`または`Release | x64`にします。
3. Visual Studioの「ローカルWindowsデバッガー」で実行します。

実行時の作業ディレクトリは`windows/App`です。この位置を基準に、ゲームコードの`../../image`と`../../audio`がルートの共有素材を参照します。

## macOS

### 必要なもの

- 公式のOpenSiv3D v0.6.16 macOS Project Templates
- Xcode 14.3以降とCommand Line Tools
- Visual Studio Code
- Apple Silicon MacではRosetta 2

Siv3D v0.6.16はApple Siliconへネイティブ対応していないため、Apple Silicon Macでは`x86_64`アプリとしてビルドし、Rosettaで実行します。

Rosetta 2が未導入の場合は、ターミナルで次を実行します。

```bash
softwareupdate --install-rosetta --agree-to-license
```

### cloneする場所

公式macOSテンプレートを展開し、このリポジトリが次の位置になるようにcloneしてください。

```text
siv3d_v0.6.16_macOS/
├── include/
├── lib/
└── examples/
    └── make10sion/   # このリポジトリ
```

例:

```bash
cd /path/to/siv3d_v0.6.16_macOS/examples
git clone https://github.com/shihu-14/make10sion.git
cd make10sion
code .
```

この配置を前提に、`macos/make10sion.xcodeproj`が公式SDKの`include`と`lib`を相対パスで参照します。SDK本体をリポジトリ内へコピーする必要はありません。

### VS Codeからビルド・実行

VS Codeでリポジトリのルートフォルダを開き、`Command + Shift + B`を押します。既定タスク`Siv3D: Build macOS`はclean buildだけを実行し、アプリを自動では起動しません。

ビルド成功後、`macos/App/make10sion-build-provenance.txt`へworktree、コミットSHA、実行ファイルのSHA-256とUUIDを記録します。`Siv3D: Run macOS`はこの記録が現在のworktreeと一致する場合だけアプリを起動します。

個別に実行する場合は、VS Codeの「Terminal」→「Run Task...」から次のタスクを選択します。

- `Siv3D: Build macOS`
- `Siv3D: Run macOS`
- `Siv3D: Build and Run macOS`
- `Siv3D: Test Battle Card Interactions`

ターミナルから同じビルドを行う場合は次を実行します。

```bash
./macos/build-debug.sh
```

実行コマンドは次のとおりです。

```bash
./macos/run-debug.sh
```

アプリを起動せず由来情報だけを検証する場合は`./macos/run-debug.sh --check`を使用できます。

Xcodeの画面を普段開く必要はありません。VS Codeのタスクが、Xcodeに付属する`xcodebuild`とmacOS SDKをバックエンドとして使用します。
