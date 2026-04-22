#pragma once
#include <string>
#include<d3d12.h>
#include<dxgi1_6.h>
#include<wrl.h> 
#include"externals/DirectXTex/DirectXTex.h"
#include"DirectXCommon.h"
#include"StringUtility.h"
#include"SrvManager.h"

using namespace StringUtility;

class TextureManager{
public:
	static TextureManager* GetInstance();
	// 初期化
	void Initialize(DirectXCommon* dxCommon, SrvManager* srvManager);
	/// <summary>
	/// テクスチャファイルの読み込み
	/// </summary>
	/// <param name="filePath">テクスチャファイルのパス</param>
	void LoadTexture(const std::string& filePath);

	uint32_t GetTextureIndexByFilePath(const std::string& filePath);
	D3D12_GPU_DESCRIPTOR_HANDLE GetSrvHandleGPU(uint32_t textureIndex);

	void Finalize();

	Microsoft::WRL::ComPtr<ID3D12Resource> CreateTextureResource(ID3D12Device* device, const DirectX::TexMetadata& metadata);
	const DirectX::TexMetadata& GetMetadata(uint32_t textureIndex);
	
private:
	DirectXCommon* dxCommon = nullptr;
	SrvManager* srvManager = nullptr;

	static TextureManager* instance;
	TextureManager() = default;
	~TextureManager() = default;
	TextureManager(TextureManager&) = delete;
	TextureManager& operator = (TextureManager&) = delete;

	struct TextureData {
		std::string filePath;
		DirectX::TexMetadata metadata;
		Microsoft::WRL::ComPtr<ID3D12Resource> resource;
		uint32_t srvIndex;
	};
	std::vector<TextureData> textureDatas;


	
};

