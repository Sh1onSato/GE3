#pragma once
#include"Structs.h"
#include <d3d12.h>
#include <wrl.h>
class SpriteCommon;
class DirectXCommon;

class Sprite {
public:
	// 初期化
	void Initialize(SpriteCommon* spriteCommon,std::string textureFilePath);
	//更新
	void Update();
	//描画
	void Draw();

	void ImGui(); // デバッグ用UIを表示する関数を追加
	// --- 外部から数値を取得するための関数 (Getter) ---
	const Vector2& GetAnchorPoint() const { return anchorPoint; }
	const bool& GetIsFlipX() const { return isFlipX; }
	const bool& GetIsFlipY() const { return isFlipY; }
	const Vector2& GetTexLeftTop() const { return texLeftTop; }
	const Vector2& GetTexSize() const { return texSize; }

	// --- 外部から数値をいじるための関数 (Setter) ---
	void SetPosition(const Vector2& pos) { transform.translate = { pos.x, pos.y, 0.0f }; }
	void SetSize(const Vector2& size) { this->size = size; }
	void SetRotation(float rotation) { transform.rotate.z = rotation; }
	void SetAnchorPoint(const Vector2& anchorPoint) { this->anchorPoint = anchorPoint; }
	void SetIsFlipX(bool isFlipX) { this->isFlipX = isFlipX; }
	void SetIsFlipY(bool isFlipY) { this->isFlipY = isFlipY; }
	void SetTexLeftTop(const Vector2& texLeftTop) { this->texLeftTop = texLeftTop; }
	void SetTexSize(const Vector2& texSize) { this->texSize = texSize; }
private:
	SpriteCommon* spriteCommon = nullptr;
	DirectXCommon* dxCommon = nullptr;

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

	// --- 数値を直接打たずに変数で管理する ---
	Vector2 size = { 640.0f, 360.0f };
	// 画面解像度も変数にしておくと、あとで変更が楽
	Vector2 screenResolution = { 1280.0f, 720.0f };
	// テクスチャ番号
	uint32_t textureIndex = 0;
	// デバッグ用にファイル名を保存しておく変数
	std::string name;

	Vector2 anchorPoint = { 0.0f, 0.0f }; 

	// 左右フリップ
	bool isFlipX = false;
	// 上下フリップ
	bool isFlipY = false;

	// テクスチャ左上座標
	Vector2 texLeftTop = { 0.0f, 0.0f };
	// テクスチャの切り出し
	Vector2 texSize = { 100.0f, 100.0f };
	// テクスチャサイズをイメージに合わせる
	void AdjustTextureSize();
};

