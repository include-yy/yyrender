struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
};

cbuffer cb0 : register(b0)
{
    float4x4 g_mWorldViewProj;
};

cbuffer cb1 : register(b1)
{
    float3 Ka;
    uint map_Ka;
    float3 Kd;
    uint map_Kd;
    float3 Ks;
    uint map_Ks;
    float3 Ke;
    uint map_Ke;
    float3 Kt;
    uint map_Kt;
    float Ns;
    uint map_Ns;
    float Ni;
    uint map_Ni;
    float3 Tf;
    float d;
    uint map_d;
    uint map_bump;
    uint illum;
    uint fallback;
};

Texture2D tex_Ke : register(t3);

//RasterizerOrderedTexture2D g_txMat : register(t0);
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
    if (map_Ke != 0)
    {
        return tex_Ke.Sample(g_sampler, input.uv);
    }
    else
    {
        return float4(Ke, 1.0f);
    }
}
