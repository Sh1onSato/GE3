#pragma once
#include "DirectXCommon.h"
#include "SrvManager.h"
#include "Structs.h"
#include <wrl.h>
#include <memory>

/// <summary>
/// ポストエフェクト（オフスクリーンレンダリング）管理クラス
/// </summary>
class PostProcess {
public:
    void Initialize(DirectXCommon* dxCommon, SrvManager* srvManager);
    
    // 描画開始（これ以降の描画はテクスチャに行われる）
    void PreDraw();
    // 描画終了（これ以降の描画は画面に行われる）
    void PostDraw();

    // 更新
    void Update();

    // 結果を画面に表示する
    void Draw();

private:
    void CreateGraphicsPipeline();

    DirectXCommon* dxCommon = nullptr;
    SrvManager* srvManager = nullptr;

    // オフスクリーン描画用のリソース
    Microsoft::WRL::ComPtr<ID3D12Resource> resource;
    uint32_t rtvIndex = 0;
    uint32_t srvIndex = 0;

    // 専用パイプライン
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState;

    // 頂点データ (画面全体を覆う板ポリ用)
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};

    // クリアカラー（背景色）
    const float clearColor[4] = { 0.1f, 0.25f, 0.5f, 1.0f };
};
