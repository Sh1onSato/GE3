#pragma once
#include"DirectXCommon.h"
#include "Structs.h"
#include <wrl.h>
#include <d3d12.h>
#include"Calculation.h"
#include <dxcapi.h>
#include "SrvManager.h"

// Object3d描画時のブレンドモード
enum class Object3dBlendMode {
    kNormal,   // 通常（不透明、深度書き込みあり）
    kAdditive, // 加算合成（発光表現用、深度書き込みなし）
};

class Object3dCommon{
public:
	void Initialize(DirectXCommon* dxCommon, SrvManager* srvManager);

	void PreDraw(Object3dBlendMode blendMode = Object3dBlendMode::kNormal);

    // シャドウマップ生成パス（ライト視点の深度のみ描画）
    void PreDrawShadow();
    void PostDrawShadow();

    DirectXCommon* GetDxCommon() const { return dxCommon; }

    PointLight& GetPointLightData() { return *pointLightData; }
    SpotLight& GetSpotLightData() { return *spotLightData; }

    Matrix4x4 GetLightViewProjection() const { return shadowData->lightViewProjection; }
    float& GetShadowBias() { return shadowData->bias; }

    // シャドウマップの解像度（正方形）
    static const uint32_t kShadowMapSize = 4096;
private:
    // ルートシグネチャの作成
    void CreateRootSignature();
    // グラフィックスパイプラインの作成
    void CreateGraphicsPipeline();
    // シャドウ用ルートシグネチャの作成（b0のみ・VSのみの最小構成）
    void CreateShadowRootSignature();
    // シャドウ用パイプライン（深度専用）の作成
    void CreateShadowPipeline();
    // ライト視点の正射影ビュープロジェクション行列を再計算（シャドウパス開始前に呼ぶ）
    void UpdateLightViewProjection();

private:
    DirectXCommon* dxCommon = nullptr;
    SrvManager* srvManager = nullptr;

    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineStateAdditive;

    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignatureShadow;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineStateShadow;


    Microsoft::WRL::ComPtr <IDxcUtils> dxcUtils = nullptr;
    Microsoft::WRL::ComPtr <IDxcCompiler3> dxcCompiler = nullptr;
    Microsoft::WRL::ComPtr <IDxcIncludeHandler> includeHandler = nullptr;
    Microsoft::WRL::ComPtr <IDxcBlob> vertexShaderBlob = nullptr;
    Microsoft::WRL::ComPtr < IDxcBlob> pixelShaderBlob = nullptr;
    Microsoft::WRL::ComPtr <IDxcBlob> shadowVertexShaderBlob = nullptr;

    // ライト用のリソース
    Microsoft::WRL::ComPtr<ID3D12Resource> directionalLightResource;
    DirectionalLight* directionalLightData = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> pointLightResource;
    PointLight* pointLightData = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> spotLightResource;
    SpotLight* spotLightData = nullptr;

    // シャドウマップ用のリソース
    Microsoft::WRL::ComPtr<ID3D12Resource> shadowMapResource;
    Microsoft::WRL::ComPtr<ID3D12Resource> shadowDataResource;
    ShadowData* shadowData = nullptr;
    uint32_t shadowMapSrvIndex = 0;
};

