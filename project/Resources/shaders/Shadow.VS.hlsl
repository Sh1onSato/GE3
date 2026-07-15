// シャドウマップ生成用の深度専用頂点シェーダー
// Object3d.VS.hlslと同じ頂点レイアウト（POSITION/TEXCOORD/NORMAL）を使い回すが、POSITIONのみ使用する。
// PSは存在しない（深度のみを書き込むパス）。

struct ShadowTransform {
    float4x4 wvp;
};
ConstantBuffer<ShadowTransform> gShadowTransform : register(b0);

struct VertexShaderInput {
    float4 position : POSITION;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
};

float4 main(VertexShaderInput input) : SV_POSITION {
    return mul(input.position, gShadowTransform.wvp);
}
