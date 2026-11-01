#pragma once
#include <algorithm>

/// <summary>
/// イージング関数集。t(0～1)を受け取り、カーブをかけた0～1を返す。
/// フェード・UIアニメ・カメラ演出など、時間で変化する値の「変わり方」を決めるのに使う。
/// </summary>
class Easing {
public:
	// ゆっくり始まり、最後に一気に変化する
	static float EaseInCubic(float t) {
		t = Clamp01(t);
		return t * t * t;
	}
	// 最初に一気に変化し、最後はゆっくり収まる
	static float EaseOutCubic(float t) {
		t = 1.0f - Clamp01(t);
		return 1.0f - t * t * t;
	}
	// 始まりと終わりがゆっくり、中間が速い
	static float EaseInOutCubic(float t) {
		t = Clamp01(t);
		if (t < 0.5f) {
			return 4.0f * t * t * t;
		}
		float u = -2.0f * t + 2.0f;
		return 1.0f - u * u * u / 2.0f;
	}
	// EaseInOutより穏やかなS字カーブ
	static float SmoothStep(float t) {
		t = Clamp01(t);
		return t * t * (3.0f - 2.0f * t);
	}

private:
	static float Clamp01(float t) { return std::clamp(t, 0.0f, 1.0f); }
};
