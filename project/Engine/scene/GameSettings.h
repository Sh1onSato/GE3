#pragma once

// タイトル画面で調整し、GameScene開始時に引き継ぐ設定値のシングルトン。
// シーンをまたいで値を保持するための最小限のプロセス内メモリ保持（ファイル永続化はしない）。
class GameSettings {
public:
	static GameSettings* GetInstance();

	float GetMouseSensitivity() const { return mouseSensitivity; }
	void SetMouseSensitivity(float value) { mouseSensitivity = value; }

	float GetEyeHeight() const { return eyeHeight; }
	void SetEyeHeight(float value) { eyeHeight = value; }

	// GameSceneのFPS Debugパネルと同じ範囲に揃える
	static constexpr float kMouseSensitivityMin = 0.0001f;
	static constexpr float kMouseSensitivityMax = 0.01f;
	static constexpr float kEyeHeightMin = 0.1f;
	static constexpr float kEyeHeightMax = 3.0f;

private:
	GameSettings() = default;
	~GameSettings() = default;
	GameSettings(const GameSettings&) = delete;
	GameSettings& operator=(const GameSettings&) = delete;

	float mouseSensitivity = 0.002f;
	float eyeHeight = 1.5f;

	static GameSettings* instance;
};
