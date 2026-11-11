struct VertexShaderOutput{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
    float3 worldPosition : TEXCOORD1;
    // インスタンスごとの色（Object3dInstanced.VS が設定。通常の Object3d.VS は白）。PS で gMaterial.color に掛ける
    float4 instanceColor : COLOR0;
};

// 実体化演出用の擬似乱数（0～1）
float Hash21(float2 p)
{
    return frac(sin(dot(p, float2(12.9898f, 78.233f))) * 43758.5453f);
}
