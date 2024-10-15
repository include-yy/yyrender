struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv       : TEXCOORD;
};

cbuffer cb0 : register(b0)
{
    float4x4 g_mWorldViewProj;
};

Texture2D g_txMat : register(t1);
SamplerState g_sampler : register(s0);

PSInput VSMain(float3 position : POSITION, float2 texcoord : TEXCOORD, float3 normal : NORMAL)
{
    PSInput result;
    result.position = mul(float4(position, 1.0f), g_mWorldViewProj);
    result.uv = float2(texcoord.x, 1.0f - texcoord.y);
    return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return g_txMat.Sample(g_sampler, input.uv);
}
