#include "object3d.hlsli"

struct TransformationMatrix{
    float4x4 WVP;
    float4x4 World;
};
ConstantBuffer<TransformationMatrix> gTransformationMatrix : register(b1);
ConstantBuffer<Material> gMaterial : register(b0);

// (3) 実体化中の横ズレグリッチ
// ノイズの切り替え頻度[回/秒]、帯の細かさ（ワールド1mあたりの本数）、帯がずれる確率、最大ズレ量（画面幅に対する比率、NDC単位）
// ※頂点単位でずらすため、高さ方向に細かく分割されたモデルでないと帯状にずれない（cube.objでは箱が歪むだけ）
static const float kGlitchNoiseRate = 15.0f;
static const float kGlitchBandDensity = 3.0f;
static const float kGlitchBandRate = 0.03f; // 常時ではなく「たまにガクッとずれる」味付け程度に抑える（0.06・幅0.03+頻度0.04と見比べてこれに決定）
static const float kGlitchShiftMax = 0.04f;

struct VertexShaderInput{
    float4 position : POSITION;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
};

VertexShaderOutput main(VertexShaderInput input){
    VertexShaderOutput output;
    output.position = mul(input.position, gTransformationMatrix.WVP);
    output.texcoord = input.texcoord;
    output.normal = normalize(mul(float4(input.normal, 0.0f), gTransformationMatrix.World).xyz);
    output.worldPosition = mul(input.position, gTransformationMatrix.World).xyz;

    // 実体化中のみ：ワールドYで帯に分け、選ばれた帯の頂点を画面上で横にずらす
    // （worldPositionはずらさないので、PS側の切り取り・発光の位置は元の形のまま）
    if (gMaterial.dissolveEdgeWidth > 0.0f) {
        float noiseStep = floor(gMaterial.dissolveTime * kGlitchNoiseRate);
        float band = floor(output.worldPosition.y * kGlitchBandDensity);
        if (Hash21(float2(band, noiseStep)) < kGlitchBandRate) {
            float shift = (Hash21(float2(band, noiseStep + 7.0f)) * 2.0f - 1.0f) * kGlitchShiftMax;
            output.position.x += shift * output.position.w; // クリップ座標でwを掛けると、距離によらず画面上で一定量ずれる
        }
    }
    return output;
}
