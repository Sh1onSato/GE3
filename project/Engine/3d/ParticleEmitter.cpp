#include "ParticleEmitter.h"
#include <random>

void ParticleEmitter::Initialize(const std::string& name, ParticleManager* manager, const EmitterSetting& setting) {
    this->name = name;
    this->manager = manager;
    this->setting = setting;
    this->timer = 0.0f;

    // 乱数生成器の初期化
    std::random_device seed_gen;
    randomEngine.seed(seed_gen());
}

void ParticleEmitter::Update() {
    timer += 1.0f / 60.0f; // 暫定 60FPS

    if (timer >= setting.frequency) {
        timer -= setting.frequency;

        // 最小値と最大値が逆転していたら補正する
        Vector3 minV = {
            (std::min)(setting.minVelocity.x, setting.maxVelocity.x),
            (std::min)(setting.minVelocity.y, setting.maxVelocity.y),
            (std::min)(setting.minVelocity.z, setting.maxVelocity.z)
        };
        Vector3 maxV = {
            (std::max)(setting.minVelocity.x, setting.maxVelocity.x),
            (std::max)(setting.minVelocity.y, setting.maxVelocity.y),
            (std::max)(setting.minVelocity.z, setting.maxVelocity.z)
        };

        // 乱数生成器の準備
        std::uniform_real_distribution<float> distV_X(minV.x, maxV.x);
        std::uniform_real_distribution<float> distV_Y(minV.y, maxV.y);
        std::uniform_real_distribution<float> distV_Z(minV.z, maxV.z);

        // 指定された数だけ放出
        for (uint32_t i = 0; i < setting.count; ++i) {
            Vector3 velocity = { distV_X(randomEngine), distV_Y(randomEngine), distV_Z(randomEngine) };
            manager->Emit(setting.position, velocity, setting.color, setting.lifeTime);
        }
    }
}
