// PostProcess のぼかし（ボックス型。横か縦の一方向）。横 → 縦の2回で、四角い範囲をぼかしたのと同じになる
// GaussianBlur.PS.hlsl と同じ cbuffer・タップ数・ルートシグネチャにしてあり、PostProcess は PSO だけ切り替えて使う。
// ガウシアン（中心ほど重みが大きい）と違い、全タップを均等な重みで平均するので、輪郭がはっきり残ったままざらっとぼける
struct VertexShaderOutput {
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

Texture2D<float4> gTexture : register(t0);
SamplerState gLinearSampler : register(s0);

cbuffer BlurParams : register(b0) {
    float2 gStep; // 1 回にずらす量（UV）。横なら (半径 / 幅, 0)、縦なら (0, 半径 / 高さ)
};

static const int kTapCount = 9; // 中心 + 左右4つずつ

float4 main(VertexShaderOutput input) : SV_TARGET {
    float3 result = gTexture.Sample(gLinearSampler, input.texcoord).rgb;
    for (int i = 1; i <= 4; ++i) {
        float2 offset = gStep * float(i);
        result += gTexture.Sample(gLinearSampler, input.texcoord + offset).rgb;
        result += gTexture.Sample(gLinearSampler, input.texcoord - offset).rgb;
    }
    return float4(result / float(kTapCount), 1.0f);
}
