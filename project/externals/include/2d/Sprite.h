#pragma once
#include"Structs.h"
#include <d3d12.h>
#include <wrl.h>
class SpriteCommon;
class DirectXCommon;

class Sprite {
public:
	// 初期化
	void Initialize(SpriteCommon* spriteCommon,uint32_t textureIndex);
	//更新
	void Update();
	//描画
	void Draw();

	void ImGui(); // デバッグ用UIを表示する関数を追加
private:
	SpriteCommon* spriteCommon = nullptr;
	DirectXCommon* dxCommon = nullptr;
	Calculation calculation;

	Microsoft::WRL::ComPtr <ID3D12Resource> vertexResourceSprite;
	Microsoft::WRL::ComPtr<ID3D12Resource> materialResourceSprite;
	Material* materialData = nullptr;
	Transform uvTransformSprite{ {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} };
	VertexData* VertexDataSprite = nullptr;
	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferViewSprite;
	//Sprite用のTransformationMatrix用のリソースを作成
	Microsoft::WRL::ComPtr <ID3D12Resource> transformationMatrixResourceSprite;
	TransformationMatrix* transformetionMatrixDataSprite;
	// スプライト自身の座標・回転・サイズ
	Transform transform{ { 1.0f, 1.0f, 1.0f } , { 0.0f, 0.0f, 0.0f } , { 0.0f, 0.0f, 0.0f } };

	Microsoft::WRL::ComPtr < ID3D12Resource> indexResourceSprite;
	D3D12_INDEX_BUFFER_VIEW indexBuffViewSprite{};
	uint32_t* indexDataSprite = nullptr;
};

