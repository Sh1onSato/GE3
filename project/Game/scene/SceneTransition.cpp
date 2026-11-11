#include "SceneTransition.h"
#include "Framework.h"
#include "GameTime.h"
#include "Screen.h"
#include "ScreenEffectController.h"
#include "Easing.h"
#include <algorithm>
#include <cmath>
#include <random>

void SceneTransition::Initialize(ScreenEffectController* newScreenEffect) {
	screenEffect = newScreenEffect;
	// 画面の大きさはゲーム座標（Screen のゲーム解像度）で測る
	const Vector2 gameSize = Screen::GetGameSize();
	overlay = std::make_unique<Sprite>();
	overlay->Initialize(Framework::GetInstance()->GetSpriteCommon(), "white");
	overlay->SetPosition({ 0.0f, 0.0f });
	overlay->SetSize(gameSize);
	overlay->SetColor({ 0.0f, 0.0f, 0.0f, 0.0f });

	// タイルは画面サイズを切り上げで割り、隙間が出ないようにする
	tileWidth = std::ceil(gameSize.x / kTileColumns);
	tileHeight = std::ceil(gameSize.y / kTileRows);
	tiles.clear();
	tileScales.assign((size_t)kTileColumns * kTileRows, 0.0f);
	tiles.reserve((size_t)kTileColumns * kTileRows);
	for (int row = 0; row < kTileRows; ++row) {
		for (int col = 0; col < kTileColumns; ++col) {
			auto tile = std::make_unique<Sprite>();
			tile->Initialize(Framework::GetInstance()->GetSpriteCommon(), "white");
			tile->SetAnchorPoint({ 0.5f, 0.5f });
			tile->SetPosition({ (col + 0.5f) * tileWidth, (row + 0.5f) * tileHeight });
			tile->SetSize({ 0.0f, 0.0f });
			tile->SetColor({ 0.0f, 0.0f, 0.0f, 1.0f });
			tiles.push_back(std::move(tile));
		}
	}
	RegenerateTileDelays();

	state = State::None;
	timer = 0.0f;
}

void SceneTransition::Start() {
	state = State::FadeOut;
	timer = 0.0f;
	glitchTime = 0.0f;
	RegenerateTileDelays();
}

void SceneTransition::StartFromBlack() {
	state = State::FadeIn;
	timer = 0.0f;
	glitchTime = 0.0f;
	RegenerateTileDelays();
	UpdateVisual();
}

void SceneTransition::BeginFadeIn() {
	state = State::FadeIn;
	timer = 0.0f;
	UpdateVisual();
}

void SceneTransition::Update() {
	// ポーズ中の timeScale に影響されないよう、unscaled の経過時間で進める
	const float deltaTime = Time::GetUnscaledDeltaTime();
	switch (state) {
	case State::None:
		return;
	case State::FadeOut:
		glitchTime += deltaTime;
		timer += deltaTime;
		if (timer >= params.fadeOutDuration) {
			state = State::Hold;
			timer = 0.0f;
		}
		break;
	case State::Hold:
		// 差し替え待ち。BeginFadeIn()が呼ばれるまでHoldのまま（IsReadyToSwitchで判定される）
		glitchTime += deltaTime;
		timer += deltaTime;
		break;
	case State::FadeIn:
		glitchTime += deltaTime;
		timer += deltaTime;
		if (timer >= params.fadeInDuration) {
			state = State::None;
			timer = 0.0f;
		}
		break;
	}

	UpdateVisual();
}

void SceneTransition::Draw() {
	if (state == State::None) {
		return;
	}
	// スプライトは積むだけ（実際の描画は SceneManager が Flush する）
	if (params.style == TransitionStyle::Tiles) {
		// Hold中は隙間対策の全画面幕のみ。それ以外はサイズのあるタイルだけ描く
		if (state == State::Hold) {
			overlay->Draw();
			return;
		}
		for (size_t i = 0; i < tiles.size(); ++i) {
			if (tileScales[i] > 0.0f) {
				tiles[i]->Draw();
			}
		}
		return;
	}
	overlay->Draw();
}

float SceneTransition::CalcProgress() const {
	switch (state) {
	case State::FadeOut:
		return std::clamp(timer / params.fadeOutDuration, 0.0f, 1.0f);
	case State::Hold:
		return 1.0f;
	case State::FadeIn:
		return 1.0f - std::clamp(timer / params.fadeInDuration, 0.0f, 1.0f);
	default:
		return 0.0f;
	}
}

void SceneTransition::UpdateVisual() {
	const float progress = CalcProgress();
	switch (params.style) {
	case TransitionStyle::Fade:
		UpdateFadeVisual(progress);
		break;
	case TransitionStyle::Tiles:
		UpdateTilesVisual(progress);
		break;
	}
	UpdateGlitch(progress);
}

void SceneTransition::UpdateGlitch(float progress) {
	float intensity = 0.0f;
	if (params.glitchStrength > 0.0f) {
		switch (state) {
		case State::FadeOut:
			intensity = Easing::InCubic(Easing::Clamp01(progress)); // 暗転が進むほど急に激しくなる
			break;
		case State::Hold:
			intensity = 1.0f;
			break;
		case State::FadeIn:
			// progressは1→0。経過割合(0→1)のEaseInCubicを引く：序盤は乱れが残り、後半で一気に収束する
			intensity = 1.0f - Easing::InCubic(Easing::Clamp01(1.0f - progress));
			break;
		default:
			break;
		}
		intensity *= params.glitchStrength;
	}
	// 強度0でも毎回送る（遷移終了時・glitchStrength=0の遷移で前回の値が残らないように）
	if (screenEffect) {
		screenEffect->SetTransitionGlitch(intensity, glitchTime);
	}
}

void SceneTransition::UpdateFadeVisual(float progress) {
	// 暗転はEaseIn（最後にストンと落ちる）、明転はEaseOut（すぐ見えてから余韻）
	float alpha = 0.0f;
	switch (state) {
	case State::FadeOut:
		alpha = Easing::InCubic(Easing::Clamp01(progress));
		break;
	case State::Hold:
		alpha = 1.0f;
		break;
	case State::FadeIn:
		// progressは1→0。EaseOutCubicには経過割合(0→1)を渡す
		alpha = 1.0f - Easing::OutCubic(Easing::Clamp01(1.0f - progress));
		break;
	default:
		break;
	}
	overlay->SetColor({ params.color.x, params.color.y, params.color.z, alpha });
	overlay->Update();
}

void SceneTransition::UpdateTilesVisual(float progress) {
	// Hold中は隙間対策として全画面の幕で埋める
	overlay->SetColor({ params.color.x, params.color.y, params.color.z, 1.0f });
	overlay->Update();

	const bool fadingOut = (state != State::FadeIn);
	// タイルの進行度p：暗転中は暗転の進み具合、明転中は明転の経過割合(0→1)
	const float p = fadingOut ? progress : 1.0f - progress;
	for (size_t i = 0; i < tiles.size(); ++i) {
		const float local = std::clamp((p - tileDelays[i] * kTileSpread) / (1.0f - kTileSpread), 0.0f, 1.0f);
		ApplyTile(i, local, fadingOut);
	}
}

void SceneTransition::ApplyTile(size_t index, float local, bool fadingOut) {
	Sprite& tile = *tiles[index];
	float scale = 0.0f;
	Vector4 color = { params.color.x, params.color.y, params.color.z, 1.0f };
	if (fadingOut) {
		scale = Easing::OutCubic(Easing::Clamp01(local));
		// 前半はアクセント色、そこから幕の色へ線形に補間
		if (local < kTileAccentPhase) {
			color = { kTileAccentColor.x, kTileAccentColor.y, kTileAccentColor.z, 1.0f };
		} else {
			const float t = (local - kTileAccentPhase) / (1.0f - kTileAccentPhase);
			color.x = kTileAccentColor.x + (params.color.x - kTileAccentColor.x) * t;
			color.y = kTileAccentColor.y + (params.color.y - kTileAccentColor.y) * t;
			color.z = kTileAccentColor.z + (params.color.z - kTileAccentColor.z) * t;
		}
	} else {
		scale = 1.0f - Easing::InCubic(Easing::Clamp01(local));
	}
	tileScales[index] = scale;
	tile.SetSize({ tileWidth * scale, tileHeight * scale });
	tile.SetColor(color);
	tile.Update();
}

void SceneTransition::RegenerateTileDelays() {
	// 遷移ごとに作り直す。ランダム＋中心→外側へのゆるい偏り
	std::mt19937 rng{ std::random_device{}() };
	std::uniform_real_distribution<float> dist(0.0f, 1.0f);
	tileDelays.assign((size_t)kTileColumns * kTileRows, 0.0f);
	const float maxDistance = std::sqrt(2.0f); // 角のタイルが1になるよう正規化
	for (int row = 0; row < kTileRows; ++row) {
		for (int col = 0; col < kTileColumns; ++col) {
			// 画面中心からの距離を-1～1の正規化座標で測る（縦横比の影響を消す）
			const float nx = ((col + 0.5f) / kTileColumns) * 2.0f - 1.0f;
			const float ny = ((row + 0.5f) / kTileRows) * 2.0f - 1.0f;
			const float sweep = (std::min)(std::sqrt(nx * nx + ny * ny) / maxDistance, 1.0f);
			tileDelays[(size_t)row * kTileColumns + col] = sweep * kTileSweepWeight + dist(rng) * (1.0f - kTileSweepWeight);
		}
	}
}
