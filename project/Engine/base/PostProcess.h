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

    // 更新（showDebugUI: trueの間だけImGui調整パネルを表示する。Pキーでのポーズ状態と連動させる想定）
    void Update(bool showDebugUI);

    // 結果を画面に表示する
    void Draw();

    // グレースケール表示のON/OFFを切り替える（キー入力等から呼び出す想定）
    void ToggleGrayscale() { grayscaleEnabled = !grayscaleEnabled; }
    // セピア表示のON/OFFを切り替える（キー入力等から呼び出す想定。グレースケールと同時ONの場合はセピアを優先）
    void ToggleSepia() { sepiaEnabled = !sepiaEnabled; }
    // 画面全体ぼかしのON/OFFを切り替える（キー入力等から呼び出す想定。ブルームとは独立して重ねがけ可能）
    void ToggleBlur() { blurEnabled = !blurEnabled; }

private:
    // RTVインデックス（rtvDescriptorHeapの並び）
    static constexpr uint32_t kSceneRtvIndex = 2;
    static constexpr uint32_t kBrightnessRtvIndex = 3;
    static constexpr uint32_t kBlurHRtvIndex = 4;
    static constexpr uint32_t kBlurVRtvIndex = 5;
    static constexpr uint32_t kFullBlurHRtvIndex = 6;
    static constexpr uint32_t kFullBlurVRtvIndex = 7;

    // ブルーム処理は負荷軽減のため半解像度で行う
    static constexpr uint32_t kBloomWidth = WinApp::KclientWidth / 2;
    static constexpr uint32_t kBloomHeight = WinApp::KclientHeight / 2;

    void CreateGraphicsPipeline();
    void CreateExtractPipeline();
    void CreateBlurPipeline();

    // 指定した幅・高さのテクスチャ1枚を作成するヘルパー（輝度抽出/ブラーHV/全画面ぼかし共通の手順を集約）
    Microsoft::WRL::ComPtr<ID3D12Resource> CreateBloomTexture(uint32_t width, uint32_t height, uint32_t rtvIndex, uint32_t& outSrvIndex);

    void DrawBrightnessExtractPass();
    void DrawGaussianBlurPass(bool horizontal, ID3D12Resource* srcResource, uint32_t srcSrvIndex, ID3D12Resource* dstResource, uint32_t dstRtvIndex,
        D3D12_VIEWPORT viewport, D3D12_RECT scissorRect, float texelWidth, float texelHeight, float strength);
    void DrawCompositePass();
    // 画面全体ぼかしパス（blurEnabled時にシーンテクスチャを水平→垂直の2パスでぼかし、fullBlurVResourceに結果を出す）
    void DrawFullScreenBlurPass();

    DirectXCommon* dxCommon = nullptr;
    SrvManager* srvManager = nullptr;

    // オフスクリーン描画用のリソース（フル解像度シーン）
    Microsoft::WRL::ComPtr<ID3D12Resource> resource;
    uint32_t rtvIndex = kSceneRtvIndex;
    uint32_t srvIndex = 0;

    // 輝度抽出結果（半解像度）
    Microsoft::WRL::ComPtr<ID3D12Resource> brightnessResource;
    uint32_t brightnessSrvIndex = 0;

    // 水平ブラー結果（半解像度）
    Microsoft::WRL::ComPtr<ID3D12Resource> blurHResource;
    uint32_t blurHSrvIndex = 0;

    // 垂直ブラー結果（半解像度）＝最終ブルームテクスチャ
    Microsoft::WRL::ComPtr<ID3D12Resource> blurVResource;
    uint32_t blurVSrvIndex = 0;

    // 画面全体ぼかし用・水平ブラー結果（フル解像度）
    Microsoft::WRL::ComPtr<ID3D12Resource> fullBlurHResource;
    uint32_t fullBlurHSrvIndex = 0;

    // 画面全体ぼかし用・垂直ブラー結果（フル解像度）＝最終ぼかしテクスチャ
    Microsoft::WRL::ComPtr<ID3D12Resource> fullBlurVResource;
    uint32_t fullBlurVSrvIndex = 0;

    // 輝度抽出パイプライン
    Microsoft::WRL::ComPtr<ID3D12RootSignature> extractRootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> extractPipelineState;

    // ガウシアンブラーパイプライン（水平/垂直共通）
    Microsoft::WRL::ComPtr<ID3D12RootSignature> blurRootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> blurPipelineState;

    // 合成パイプライン（シーン + ブルーム）
    Microsoft::WRL::ComPtr<ID3D12RootSignature> compositeRootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> compositePipelineState;

    // 半解像度パス用のビューポート・シザー矩形
    D3D12_VIEWPORT bloomViewport{};
    D3D12_RECT bloomScissorRect{};

    // ImGuiで調整するブルームパラメータ
    float bloomThreshold = 1.0f;
    float bloomIntensity = 1.0f;
    float blurStrength = 1.0f;
    bool grayscaleEnabled = false; // 合成パスで画面全体をグレースケール化するかどうか
    bool sepiaEnabled = false;     // 合成パスで画面全体をセピア調にするかどうか（グレースケールより優先）

    // 画面全体ぼかし（ブルームとは独立管理。Bキーでトグル）
    bool blurEnabled = false;
    float screenBlurStrength = 2.0f;

    // 頂点データ (画面全体を覆う板ポリ用)
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};

    // クリアカラー（背景色）
    const float clearColor[4] = { 0.1f, 0.25f, 0.5f, 1.0f };
};
