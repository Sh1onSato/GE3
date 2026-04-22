#pragma once
#include"DirectXCommon.h"
#include "Structs.h"
#include <wrl.h>
#include <d3d12.h>
#include"Calculation.h"
#include <dxcapi.h>
#include "SrvManager.h"

class Object3dCommon{
public:
	void Initialize(DirectXCommon* dxCommon, SrvManager* srvManager);

	void PreDraw();

    DirectXCommon* GetDxCommon() const { return dxCommon; }
private:
    // ルートシグネチャの作成
    void CreateRootSignature();
    // グラフィックスパイプラインの作成
    void CreateGraphicsPipeline();

private:
    DirectXCommon* dxCommon = nullptr;
    SrvManager* srvManager = nullptr;
	Calculation calculation;

    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState;


    Microsoft::WRL::ComPtr <IDxcUtils> dxcUtils = nullptr;
    Microsoft::WRL::ComPtr <IDxcCompiler3> dxcCompiler = nullptr;
    Microsoft::WRL::ComPtr <IDxcIncludeHandler> includeHandler = nullptr;
    Microsoft::WRL::ComPtr <IDxcBlob> vertexShaderBlob = nullptr;
    Microsoft::WRL::ComPtr < IDxcBlob> pixelShaderBlob = nullptr;

    // ライト用のリソース
    Microsoft::WRL::ComPtr<ID3D12Resource> directionalLightResource;
    DirectionalLight* directionalLightData = nullptr;
};

