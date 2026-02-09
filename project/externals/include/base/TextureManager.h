#pragma once
#include <string>
#include<d3d12.h>
#include<dxgi1_6.h>
#include<wrl.h> 
#include"externals/DirectXTex/DirectXTex.h"
#include"DirectXCommon.h"
#include"StringUtility.h"

using namespace StringUtility;

class TextureManager{
public:
	static TextureManager* GetInstance();
	// 初期化
	void Initialize();
	/// <summary>
	/// テクスチャファイルの読み込み
	/// </summary>
	/// <param name="filePath">テクスチャファイルのパス</param>
	void LoadTexture(const std::string& filePath, DirectXCommon* dxCommon);

	uint32_t GetTextureIndexByFilePath(const std::string& filePath);
	D3D12_GPU_DESCRIPTOR_HANDLE GetSrvHandleGPU(uint32_t textureIndex);

	void Finalize();

	Microsoft::WRL::ComPtr<ID3D12Resource> CreateTextureResource(ID3D12Device* device, const DirectX::TexMetadata& metadata);
	const DirectX::TexMetadata& GetMetadata(uint32_t textureIndex);
	//Imgui用の変数
	static uint32_t kSRVIndexTop;
private:
	DirectXCommon* dxCommon;

	static TextureManager* instance;
	TextureManager() = default;
	~TextureManager() = default;
	TextureManager(TextureManager&) = delete;
	TextureManager& operator = (TextureManager&) = delete;

	struct TextureDate {
		std::string filePath;
		DirectX::TexMetadata metadata;
		Microsoft::WRL::ComPtr<ID3D12Resource> resource;
		D3D12_CPU_DESCRIPTOR_HANDLE srvHandleCPU;
		D3D12_GPU_DESCRIPTOR_HANDLE srvHandleGPU;
	};
	std::vector<TextureDate> textureDatas;


	
};

