#pragma once
#include "DirectXCommon.h"
#include "SrvManager.h"
#include <wrl.h>
#include <d3d12.h>
#include <dxcapi.h>

/// <summary>
/// パーティクル共通部
/// </summary>
class ParticleCommon {
public:
    void Initialize(DirectXCommon* dxCommon, SrvManager* srvManager);

    // 描画前処理
    void PreDraw();

    // ゲッター
    DirectXCommon* GetDxCommon() const { return dxCommon; }

private:
    // ルートシグネチャの作成
    void CreateRootSignature();
    // グラフィックスパイプラインの作成
    void CreateGraphicsPipeline();

private:
    DirectXCommon* dxCommon = nullptr;
    SrvManager* srvManager = nullptr;

    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState;

    Microsoft::WRL::ComPtr<IDxcUtils> dxcUtils = nullptr;
    Microsoft::WRL::ComPtr<IDxcCompiler3> dxcCompiler = nullptr;
    Microsoft::WRL::ComPtr<IDxcIncludeHandler> includeHandler = nullptr;
};
