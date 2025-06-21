#ifndef Title_HPP
#define Title_HPP
#include <Siv3D.hpp> // OpenSiv3D v0.6.4
#include "common.hpp"

class Title : public App::Scene {
private:
	//Write private functions or varables here.
	Texture m_background;  // 背景画像用のTextureオブジェクト
	Font m_font;
	RoundRect m_startButtonRect; // ボタンの矩形
	RoundRect m_endButtonRect;
	Audio m_titleBGM;
	Texture m_titlelogo; // タイトルロゴのテクスチャ
	Texture m_startButtonTexture;
	Texture m_startButtonTexture2;// スタートボタンのテクスチャ
	Texture m_endButtonTexture;
	Texture m_endButtonTexture2; // 終了ボタンのテクスチャ
public:
	Title(const InitData& init);
	//Write public functions here.
	~Title() = default;

	void update() override;
	void draw() const override;
};

#endif
