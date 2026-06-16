#pragma once
#include"Structs.h"
#include"Calculation.h"
#include<string>
#include"DirectXCommon.h"
#include"TextureManager.h"

class Model{
public:
public:
    // ロードとリソース作成を一度に行う関数
    void Initialize(DirectXCommon* dxCommon, const std::string& directoryPath, const std::string& filename);
    void Draw(ID3D12GraphicsCommandList* commandList);

    // Getter
    const D3D12_VERTEX_BUFFER_VIEW& GetVertexBufferView() const { return vertexBufferView; }
    UINT GetVertexCount() const { return (UINT)modelData.vertices.size(); }

private:

private:
    // 既存のメンバ関数はprivateにして、Initializeから呼ぶのが一般的です
    MaterialData LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename);
    ModelData LoadObjFile(const std::string& directoryPath, const std::string& filename);

    ModelData modelData;
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
};

