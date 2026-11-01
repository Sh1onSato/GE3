struct VertexShaderOutput{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
    float3 worldPosition : TEXCOORD1;
};

// VS（横ズレ）とPS（切り取り・発光）の両方で読むため共通ヘッダに置く
struct Material
{
    float4 color;
    int enableLighting;
    float dissolveHeight;    // 実体化演出：このワールドY座標より上のピクセルを描かない
    float dissolveEdgeWidth; // 実体化演出：境界の発光幅。0以下で演出無効
    float dissolveTime;      // 実体化演出：ノイズを動かすための経過時間[秒]
    float4x4 uvTransform;
    float dissolveBlockStrength; // 実体化演出：ブロック欠けの強さ（0～1）。全体が見えた時点で0にする
};

// 実体化演出用の擬似乱数（0～1）
float Hash21(float2 p)
{
    return frac(sin(dot(p, float2(12.9898f, 78.233f))) * 43758.5453f);
}
