#pragma once
#include "WinApp.h"
#include "DirectXCommon.h"
#include "SrvManager.h"
#include "Input.h"
#include "TextureManager.h"
#include "SpriteCommon.h"
#include "Object3dCommon.h"
#include "ParticleCommon.h"
#include "SceneManager.h"

class Framework {
public:
	virtual ~Framework() = default;

	static Framework* GetInstance() { return instance; }

	// 実行
	void Run();

	// 初期化
	virtual void Initialize();

	// 終了
	virtual void Finalize();

	// 更新
	virtual void Update();

	// 描画（派生クラスで実装）
	virtual void Draw() = 0;

	// 終了フラグのゲッター
	virtual bool IsEndRequest() { return endRequest; }

	// 各種ゲッター
	WinApp* GetWinApp() const { return winApp; }
	DirectXCommon* GetDxCommon() const { return dxCommon; }
	SrvManager* GetSrvManager() const { return srvManager; }
	Input* GetInput() const { return input; }
	SpriteCommon* GetSpriteCommon() const { return spriteCommon; }
	Object3dCommon* GetObject3dCommon() const { return object3dCommon; }
	ParticleCommon* GetParticleCommon() const { return particleCommon; }

protected:
	WinApp* winApp = nullptr;
	DirectXCommon* dxCommon = nullptr;
	SrvManager* srvManager = nullptr;
	Input* input = nullptr;
	
	// これらはゲーム側で使う可能性が高いので protected に配置
	SpriteCommon* spriteCommon = nullptr;
	Object3dCommon* object3dCommon = nullptr;
	ParticleCommon* particleCommon = nullptr;
	SceneManager* sceneManager = nullptr;

	bool endRequest = false;

	static Framework* instance;
};
