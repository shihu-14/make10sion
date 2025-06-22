#include "Title.hpp"  
using namespace std;

Title::Title(const InitData& init) : IScene(init),
m_background(U"../../image/haikei_sentou.png"),
m_titlelogo(U"../../image/title_logo.png"),
m_startButtonTexture(U"../../image/title_start_0.png"),
m_startButtonTexture2(U"../../image/title_start_1.png"),
m_endButtonTexture(U"../../image/title_end_0.png"),
m_endButtonTexture2(U"../../image/title_end_1.png"),
m_startButtonRect(Arg::center = Vec2(Scene::Center().x + 20, Scene::Height() - 450), 420, 100, 20), // 修正: RoundRect の正しいコンストラクタを使用
m_endButtonRect(Arg::center = Vec2(Scene::Center().x + 20, Scene::Height() - 250), 420, 100, 20), // 修正: RoundRect の正しいコンストラクタを使用
m_font(30, Typeface::Bold),
fadeTextures(3),
m_titleBGM(U"example/audio/game_bgm.mp3", Loop::Yes)
{
    m_titleBGM.play();
    for (int i = 0; i < 3; i++)
        fadeTextures[i] = Texture{ U"../../image/tyu-toriarumae_" + ToString(i + 1) + U".png" };
}

void Title::update() {
    if (go_to_map) {
        if (Time::GetMillisec() - timer > 9000) { // 6000ミリ秒待つ
            changeScene(State::Map, 0.5s); // マップシーンへ遷移
        }
        return;
    }
    if (m_startButtonRect.mouseOver())
    {
        Cursor::RequestStyle(CursorStyle::Hand);
        if (m_startButtonRect.leftClicked()) { // マウス左ボタンがクリックされた瞬間
            // マップシーンへ遷移
            //changeScene(State::Map, 0.5s);
            go_to_map = true; // マップシーンに移動するフラグを立てる
            timer = Time::GetMillisec();
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
    m_background.scaled(0.5).draw(0, 0);

    m_titlelogo.drawAt(Scene::Center().x, Scene::Center().y - 100); // タイトルロゴを画面中央に配置

    if (go_to_map) {
        int time = Time::GetMillisec() - timer;
        double animation = 0.0;
        if (time < 1000){
            const double t = static_cast<double>(time) / 1000.0; // 0.0から1.0の範囲に正規化
            const double progress = EaseInOutExpo(t);
            animation = 1080 * Math::Lerp(0.0, 1.0, progress); // 1080は画面の高さ
        }else if (time < 3000){
            animation = 1080; // 2秒目以降はアニメーションを固定
        }else if (time < 4000){
            const double t = static_cast<double>(time - 3000) / 1000.0; // 0.0から1.0の範囲に正規化
            const double progress = EaseInOutExpo(t);
            animation = 1080 + 1080 * Math::Lerp(0.0, 1.0, progress); // 1080は画面の高さ
        }else if (time < 6000){
            animation = 2160;
        }else if (time < 7000){
            const double t = static_cast<double>(time - 6000) / 1000.0; // 0.0から1.0の範囲に正規化
            const double progress = EaseInOutExpo(t);
            animation = 2160 + 1080 * Math::Lerp(0.0, 1.0, progress); // 1080は画面の高さ
        }else{
            animation = 3240; // 画面の高さの2倍
        }

        for (int i = 0; i < fadeTextures.size(); i++) {
            fadeTextures[i].draw(0, (i + 1) * 1080 - animation);
        }
        return;
    }

    if (m_startButtonRect.mouseOver()) {
        // マウスがボタンの上にいる場合、ホバー時の画像を描画
        m_startButtonTexture2.drawAt(Scene::Center().x, Scene::Center().y); // m_startButtonRectの領域に合わせて画像を引き伸ばし
    } else {
        // マウスがボタンの上にいない場合、通常時の画像を描画
        m_startButtonTexture.drawAt(Scene::Center().x, Scene::Center().y); // m_startButtonRectの領域に合わせて画像を引き伸ばし
    }
    if (m_endButtonRect.mouseOver()) {
        // マウスがボタンの上にいる場合、ホバー時の画像を描画
        m_endButtonTexture2.drawAt(Scene::Center().x, Scene::Center().y); // m_endButtonRectの領域に合わせて画像を引き伸ばし
    } else {
        // マウスがボタンの上にいない場合、通常時の画像を描画
        m_endButtonTexture.drawAt(Scene::Center().x, Scene::Center().y); // m_endButtonRectの領域に合わせて画像を引き伸ばし
    }

}
