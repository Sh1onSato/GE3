#include "BitmapText.h"
#include "Sprite.h"
#include "SpriteCommon.h"
#include "TextureManager.h"

void BitmapText::Initialize(SpriteCommon* spriteCommon, size_t maxChars) {
    this->spriteCommon = spriteCommon;

    // "digitFont"アトラスが未生成なら生成する（複数のBitmapTextから呼ばれても内部でガードされる）
    TextureManager::GetInstance()->CreateInternalDigitFontTexture();

    charSprites.clear();
    charSprites.reserve(maxChars);
    for (size_t i = 0; i < maxChars; ++i) {
        auto sprite = std::make_unique<Sprite>();
        sprite->Initialize(spriteCommon, "digitFont");
        sprite->SetTexSize({ kGlyphCellWidth, kGlyphCellHeight });
        sprite->SetSize({ kGlyphCellWidth * scale, kGlyphCellHeight * scale });
        sprite->SetColor(color);
        charSprites.push_back(std::move(sprite));
    }
}

int BitmapText::GlyphIndex(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c == '.') return 10;
    if (c == '-') return 11;
    switch (c) {
        case 'W': return 13;
        case 'I': return 14;
        case 'N': return 15;
        case 'L': return 16;
        case 'O': return 17;
        case 'S': return 18;
        case 'E': return 19;
        case 'Q': return 20;
        case 'U': return 21;
        case 'T': return 22;
        case 'A': return 23;
        case 'B': return 24;
        case 'C': return 25;
        case 'D': return 26;
        case 'F': return 27;
        case 'G': return 28;
        case 'H': return 29;
        case 'J': return 30;
        case 'K': return 31;
        case 'M': return 32;
        case 'P': return 33;
        case 'R': return 34;
        case 'V': return 35;
        case 'X': return 36;
        case 'Y': return 37;
        case 'Z': return 38;
        case '>': return 39;
    }
    return kBlankGlyphIndex;
}

void BitmapText::SetText(const std::string& text) {
    // 位置/倍率変更時に再配置できるよう、直近テキストを保持しておく
    lastText = text;
    Relayout();
}

void BitmapText::Relayout() {
    size_t count = (lastText.size() < charSprites.size()) ? lastText.size() : charSprites.size();
    for (size_t i = 0; i < count; ++i) {
        int glyph = GlyphIndex(lastText[i]);
        Sprite* sprite = charSprites[i].get();
        sprite->SetTexLeftTop({ kGlyphCellWidth * static_cast<float>(glyph), 0.0f });
        sprite->SetPosition({ position.x + kGlyphCellWidth * scale * static_cast<float>(i), position.y });
    }
    visibleCount = count;
}

void BitmapText::SetPosition(const Vector2& pos) {
    position = pos;
    Relayout();
}

void BitmapText::SetScale(float newScale) {
    scale = newScale;
    for (auto& sprite : charSprites) {
        sprite->SetSize({ kGlyphCellWidth * scale, kGlyphCellHeight * scale });
    }
    Relayout();
}

float BitmapText::GetWidth() const {
    return static_cast<float>(visibleCount) * kGlyphCellWidth * scale;
}

void BitmapText::SetColor(const Vector4& newColor) {
    color = newColor;
    for (auto& sprite : charSprites) {
        sprite->SetColor(color);
    }
}

void BitmapText::Update() {
    for (size_t i = 0; i < visibleCount; ++i) {
        charSprites[i]->Update();
    }
}

void BitmapText::Draw() {
    for (size_t i = 0; i < visibleCount; ++i) {
        charSprites[i]->Draw();
    }
}
