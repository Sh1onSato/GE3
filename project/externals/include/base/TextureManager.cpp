#include "TextureManager.h"

TextureManager* TextureManager::instance = nullptr;

TextureManager* TextureManager::GetInstance() {
	if (instance == nullptr) {
		instance = new TextureManager();
	}
	return instance;
}

void TextureManager::Initialize() {
	textureDatas.reserve(DirectXCommon::kMaxSrvCount);
}

void TextureManager::LoadTexture(const std::string& filePath, DirectXCommon* dxCommon){
	this->dxCommon = dxCommon;
	// 読み込み済みテクスチャを検索
	auto it = std::find_if(
		textureDatas.begin(),
		textureDatas.end(),
		[&](TextureDate& textureData) {
			return textureData.filePath == filePath;
		}
	);
	if(it!= textureDatas.end()){
		// 見つかった場合は何もしない
		return;
	}

	assert(textureDatas.size() + kSRVIndexTop < DirectXCommon::kMaxSrvCount);

	// テクスチャの読み込み
	DirectX::ScratchImage image{};
	std::wstring filePathW = ConvertString(filePath);
	HRESULT hr = DirectX::LoadFromWICFile(filePathW.c_str(), DirectX::WIC_FLAGS_FORCE_SRGB, nullptr, image);
	assert(SUCCEEDED(hr));
	// ミニマップの作成
	DirectX::ScratchImage mipChain{};
	hr = DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(), image.GetMetadata(), DirectX::TEX_FILTER_SRGB, 0, mipChain);
	assert(SUCCEEDED(hr));

	// テクスチャデータを追加
	textureDatas.resize(textureDatas.size() + 1);
	// 追加したテクスチャデータの参照を取得
	TextureDate& textureData = textureDatas.back();

	textureData.filePath = filePath;
	textureData.metadata = mipChain.GetMetadata();
	textureData.resource = CreateTextureResource(dxCommon->GetDevice(), textureData.metadata);

	uint32_t srvIndex = static_cast<uint32_t>(textureDatas.size() - 1) + kSRVIndexTop;

	textureData.srvHandleCPU = dxCommon->GetSrvCPUHandle(srvIndex);
	textureData.srvHandleGPU = dxCommon->GetSrvGPUHandle(srvIndex);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = textureData.metadata.format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	if (textureData.metadata.dimension == DirectX::TEX_DIMENSION_TEXTURE2D) {
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = static_cast<UINT>(textureData.metadata.mipLevels);
	}
	else {
		assert(0);
	}
	// SRVの作成
	dxCommon->GetDevice()->CreateShaderResourceView(textureData.resource.Get(), &srvDesc, textureData.srvHandleCPU);
	// テクスチャデータのアップロード
	Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource = dxCommon->UploadTextureData(textureData.resource.Get(),mipChain,dxCommon);


	// コマンドリストを閉じて実行、アップロード完了を待機
	hr = dxCommon->GetCommandList()->Close();
	assert(SUCCEEDED(hr));
	ID3D12CommandList* commandListsForUpload[] = { dxCommon->GetCommandList() };
	dxCommon->GetCommandQueue()->ExecuteCommandLists(1, commandListsForUpload);
	dxCommon->WaitForGpu();

	// コマンドリストとアロケーターをリセットして、メインループで再度使用できるようにする
	hr = dxCommon->GetCommandAllocator()->Reset(); assert(SUCCEEDED(hr));
	hr = dxCommon->GetCommandList()->Reset(dxCommon->GetCommandAllocator(), nullptr); assert(SUCCEEDED(hr));

}

uint32_t TextureManager::GetTextureIndexByFilePath(const std::string& filePath){
	auto it = std::find_if(
		textureDatas.begin(),
		textureDatas.end(),
		[&](TextureDate& textureData) {
			return textureData.filePath == filePath;
		}
	);
	if(it!= textureDatas.end()) {
		uint32_t textureIndex = static_cast<uint32_t>(std::distance(textureDatas.begin(), it));
		return textureIndex;
	}
	assert(0);
	return 0;
}

D3D12_GPU_DESCRIPTOR_HANDLE TextureManager::GetSrvHandleGPU(uint32_t textureIndex){
	assert(textureIndex < textureDatas.size());

	TextureDate& textureData = textureDatas[textureIndex];
	return textureData.srvHandleGPU;
}

void TextureManager::Finalize() {
	delete instance;
	instance = nullptr;
}


Microsoft::WRL::ComPtr<ID3D12Resource> TextureManager::CreateTextureResource(ID3D12Device* device, const DirectX::TexMetadata& metadata) {
	// テクスチャのリソースを作成
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Width = UINT(metadata.width);
	resourceDesc.Height = UINT(metadata.height);
	resourceDesc.MipLevels = UINT(metadata.mipLevels);
	resourceDesc.DepthOrArraySize = UINT(metadata.arraySize);
	resourceDesc.Format = metadata.format;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION(metadata.dimension);

	// 利用するHeepの設定
	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

	// Resourceの生成
	Microsoft::WRL::ComPtr <ID3D12Resource> resource = nullptr;
	HRESULT hr = device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&resource));
	assert(SUCCEEDED(hr));
	return resource;
}

const DirectX::TexMetadata& TextureManager::GetMetadata(uint32_t textureIndex){
	assert(textureIndex < textureDatas.size());
	
	TextureDate& textureData = textureDatas[textureIndex];
	return textureData.metadata;
}

uint32_t TextureManager::kSRVIndexTop = 1;