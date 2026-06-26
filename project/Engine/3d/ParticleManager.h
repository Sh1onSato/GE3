#pragma once
#include "Calculation.h"
#include "Structs.h"
#include "ParticleCommon.h"
#include "Camera.h"
#include <vector>
#include <list>
#include <wrl.h>

/// <summary>
/// 個々のパーティクル（粒）のデータ
/// </summary>
struct Particle {
    Transform transform;
    Vector3 velocity;
    Vector4 color;
    float lifeTime; // 寿命（秒）
    float currentTime; // 現在の経過時間
};

/// <summary>
/// GPUに送る用のパーティクルデータ
/// </summary>
struct ParticleForGPU {
    Matrix4x4 WVP;
    Matrix4x4 World;
    Vector4 color;
};

/// <summary>
/// パーティクルマネージャー
/// </summary>
class ParticleManager {
public:
    // 1つのマネージャーで管理できる最大数
    static const uint32_t kMaxParticles = 50000;

    void Initialize(ParticleCommon* common, uint32_t textureIndex, ParticleDrawType drawType = ParticleDrawType::kTriangle);
    void Update();
    void Draw();

    // パーティクルの追加
    void Emit(const Vector3& position, const Vector3& velocity, const Vector4& color, float lifeTime);

    // ゲッター
    ParticleDrawType GetDrawType() const { return drawType; }

private:
    ParticleCommon* common = nullptr;
    uint32_t textureIndex = 0;
    ParticleDrawType drawType = ParticleDrawType::kTriangle;

    // 現在のアクティブなパーティクル数
    uint32_t instanceCount = 0;

    // パーティクルのリスト
    std::list<Particle> particles;

    // インスタンシング用のリソース
    Microsoft::WRL::ComPtr<ID3D12Resource> instancingResource;
    ParticleForGPU* instancingData = nullptr;

    // 頂点データ (パーティクルはただの板ポリ)
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
};
