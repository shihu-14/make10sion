#ifndef Title_HPP
#define Title_HPP
#include <Siv3D.hpp> // OpenSiv3D v0.6.4
#include "common.hpp"

class Title : public App::Scene {
private:
	Texture m_background;  // 背景画像用のTextureオブジェクト
	Font m_font;
	RoundRect m_startButtonRect; // ボタンの矩形
	RoundRect m_endButtonRect;
	RectF m_demoButtonRect{ Scene::Width() - 320, Scene::Height() - 100, 280, 60 };
	Audio titleBGM{ U"../../audio/title_bgm.wav",Loop::Yes }; // タイトル画面のBGM
	Texture m_titlelogo; // タイトルロゴのテクスチャ
	Texture m_startButtonTexture;
	Texture m_startButtonTexture2;// スタートボタンのテクスチャ
	Texture m_endButtonTexture;
	Texture m_endButtonTexture2; // 終了ボタンのテクスチャ
	bool go_to_map = false; // マップシーンに移動するかどうかのフラグ
	uint64 timer = 0; // タイマー

	std::vector<Texture> fadeTextures; // フェード用のテクスチャ
public:
	Title(const InitData& init);
	~Title() = default;

	void update() override;
	void draw() const override;
};

#endif
