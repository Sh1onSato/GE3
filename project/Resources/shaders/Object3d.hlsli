struct VertexShaderOutput{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
    float3 worldPosition : TEXCOORD1;
    // インスタンスごとの色（Object3dInstanced.VS が設定。通常の Object3d.VS は白）。PS で gMaterial.color に掛ける
    float4 instanceColor : COLOR0;
};

// 物ごとの材質（C++ の Material と同じ並び）。PS と VS（実体化の横ズレ）で共有する
struct Material
{
    float4 color;
    int enableLighting;
    float dissolveHeight;        // 実体化演出：このワールドY座標より上のピクセルを描かない
    float dissolveEdgeWidth;     // 実体化演出：境界の発光幅。0以下で演出無効
    float dissolveTime;          // 実体化演出：ノイズを動かすための経過時間[秒]
    float4x4 uvTransform;
    float4 emissive;             // 自分で光る色（rgb。モデルの発光に足す）
    float metallicOverride;      // 0 以上ならモデルの値の代わりに使う
    float roughnessOverride;
    int lightingModelOverride;   // -1 = モデルのまま、0 = 簡単な当たり方、1 = 金属・粗さを使う当たり方
    float dissolveBlockStrength; // 実体化演出：ブロック欠けの強さ（0～1）。全体が見えた時点で0にする
};

// 実体化演出用の擬似乱数（0～1）
float Hash21(float2 p)
{
    return frac(sin(dot(p, float2(12.9898f, 78.233f))) * 43758.5453f);
}
