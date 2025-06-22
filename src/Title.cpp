#include "Title.hpp"  
using namespace std;  

Title::Title(const InitData& init) : IScene(init), 
m_background(U"../../image/haikei_sentou.png"), 
m_titlelogo(U"../../image/title_logo.png"),
m_startButtonTexture(U"../../image/title_start_0.png"),
m_startButtonTexture2(U"../../image/title_start_1.png"),
m_endButtonTexture(U"../../image/title_end_0.png"),
m_endButtonTexture2(U"../../image/title_end_1.png"),
m_startButtonRect(Arg::center = Vec2(Scene::Center().x+20 , Scene::Height() - 450), 420, 100, 20), // 修正: RoundRect の正しいコンストラクタを使用
m_endButtonRect(Arg::center = Vec2(Scene::Center().x +20, Scene::Height()-250 ), 420, 100, 20), // 修正: RoundRect の正しいコンストラクタを使用
m_font(30, Typeface::Bold),
m_titleBGM(U"example/audio/game_bgm.mp3", Loop::Yes)
{
        m_titleBGM.play();
    }

void Title::update() {  
    if (m_startButtonRect.mouseOver())
    {   
        Cursor::RequestStyle(CursorStyle::Hand);
        if (m_startButtonRect.leftClicked()) { // マウス左ボタンがクリックされた瞬間
            // マップシーンへ遷移
            changeScene(State::Map, 0.5s);
        }
    }
    if (m_endButtonRect.mouseOver())
    {
        Cursor::RequestStyle(CursorStyle::Hand);
        if (m_endButtonRect.leftClicked()) { // マウス左ボタンがクリックされた瞬間
         
            System::Exit();
        }
    }
}  

void Title::draw() const {
    m_background.draw(0, 0);

	m_titlelogo.drawAt(Scene::Center().x, Scene::Center().y - 100); // タイトルロゴを画面中央に配置
    if (m_startButtonRect.mouseOver()) {
        // マウスがボタンの上にいる場合、ホバー時の画像を描画
        m_startButtonTexture2.drawAt(Scene::Center().x, Scene::Center().y); // m_startButtonRectの領域に合わせて画像を引き伸ばし
    }
    else {
        // マウスがボタンの上にいない場合、通常時の画像を描画
        m_startButtonTexture.drawAt(Scene::Center().x, Scene::Center().y); // m_startButtonRectの領域に合わせて画像を引き伸ばし
    }
    if(m_endButtonRect.mouseOver()) {
        // マウスがボタンの上にいる場合、ホバー時の画像を描画
        m_endButtonTexture2.drawAt(Scene::Center().x, Scene::Center().y); // m_endButtonRectの領域に合わせて画像を引き伸ばし
    }
    else {
        // マウスがボタンの上にいない場合、通常時の画像を描画
        m_endButtonTexture.drawAt(Scene::Center().x, Scene::Center().y); // m_endButtonRectの領域に合わせて画像を引き伸ばし
	}

}
