#pragma once
#include "Calculation.h"
#include "ParticleManager.h"
#include <string>
#include <random>

/// <summary>
/// パーティクル発生源
/// </summary>
class ParticleEmitter {
public:
    struct EmitterSetting {
        Vector3 position;       // 発生中心座標
        Vector3 minVelocity;    // 最小初速
        Vector3 maxVelocity;    // 最大初速
        Vector4 color;          // 色
        float lifeTime;         // 寿命
        uint32_t count;         // 一度に出す数
        float frequency;        // 発生間隔（秒）
    };

    void Initialize(const std::string& name, ParticleManager* manager, const EmitterSetting& setting);
    void Update();

    // ゲッター・セッター
    const std::string& GetName() const { return name; }
    void SetPosition(const Vector3& pos) { setting.position = pos; }

    const EmitterSetting& GetSetting() const { return setting; }
    void SetSetting(const EmitterSetting& newSetting) { setting = newSetting; }

private:
    std::string name;
    ParticleManager* manager = nullptr;
    EmitterSetting setting;

    float timer = 0.0f;

    // 乱数生成器
    std::mt19937 randomEngine;
};
