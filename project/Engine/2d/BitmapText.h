#pragma once
#include "Structs.h"
#include <memory>
#include <string>
#include <vector>

class Sprite;
class SpriteCommon;

// 数字("0"-"9")・"."・"-"・アルファベット(A-Z、大文字小文字問わず)を表示できる軽量なビットマップフォント描画クラス。
// TextureManagerが内部生成する"digitFont"アトラス（5x7ドット、1pxパディング入り7x9セル）を、
// Spriteのプールを並べてUV切り出しすることで表示する（ImGuiに依存しないため、Releaseビルドでも使える）。
class BitmapText {
public:
    // maxChars: このインスタンスが表示できる最大文字数（プール数）
    void Initialize(SpriteCommon* spriteCommon, size_t maxChars);
    // "0"-"9" "." "-" "A"-"Z"（小文字は大文字にフォールバック）以外の文字は空白グリフとして扱う
    void SetText(const std::string& text);
    void SetPosition(const Vector2& pos);
    // 表示倍率を変更する（スプライトサイズも更新し、直近テキストを再配置する）
    void SetScale(float scale);
    // 直近のSetTextで表示した文字列の横幅(px)。中央揃え・右揃えの計算用
    float GetWidth() const;
    void SetColor(const Vector4& color);
    // 描画レイヤー（大きいほど手前。Initialize前に呼んでも反映される）
    void SetLayer(int layer);

    void Update();
    void Draw();

private:
    // グリフアトラス上の1セルのサイズ（TextureManager::CreateInternalDigitFontTextureと一致させる）
    static constexpr float kGlyphCellWidth = 7.0f;
    static constexpr float kGlyphCellHeight = 9.0f;
    static constexpr int kBlankGlyphIndex = 12;

    // 対応表引き。非対応文字は空白グリフのインデックスを返す
    static int GlyphIndex(char c);
    // lastText・position・scaleから各スプライトの位置/UVを再計算する
    void Relayout();

    SpriteCommon* spriteCommon = nullptr;
    std::vector<std::unique_ptr<Sprite>> charSprites; // maxChars分を事前生成しておくプール
    Vector2 position{ 0.0f, 0.0f };
    Vector4 color{ 1.0f, 1.0f, 1.0f, 1.0f };
    std::string lastText;    // 直近のSetTextの文字列（SetPosition/SetScale時の再配置用）
    int layer = 0;           // 全文字スプライトに適用する描画レイヤー
    float scale = 2.0f;      // 1グリフあたりの表示倍率
    size_t visibleCount = 0; // 直近のSetTextで実際に使われた文字数（この数だけUpdate/Drawする）
};
