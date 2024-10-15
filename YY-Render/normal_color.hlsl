struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

cbuffer cb0 : register(b0)
{
    float4x4 g_mWorldViewProj;
};

PSInput VSMain(float3 position : POSITION, float2 texcoord : TEXCOORD, float3 normal : NORMAL)
{
    PSInput result;

    result.position = mul(float4(position, 1.0f), g_mWorldViewProj);
    result.color = float4(normal, 1.0f);

    return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return input.color;
}
