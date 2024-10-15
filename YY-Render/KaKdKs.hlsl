struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
};

cbuffer cb0 : register(b0)
{
    float4x4 g_mWorldViewProj;
    float3 g_light;
    uint g_useDefaultDiffuse;
    float3 g_eyepos;
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

Texture2D tex_Ka : register(t0);
Texture2D tex_Kd : register(t1);
Texture2D tex_Ks : register(t2);
Texture2D tex_Ke : register(t3);
Texture2D tex_Kt : register(t4);
Texture2D tex_Ns : register(t5);
Texture2D tex_Ni : register(t6);
Texture2D tex_d : register(t7);
Texture2D tex_bump : register(t8);

//RasterizerOrderedTexture2D g_txMat : register(t0);
SamplerState g_sampler : register(s0);

PSInput VSMain(float3 position : POSITION, float2 texcoord : TEXCOORD, float3 normal : NORMAL)
{
    PSInput result;

    result.position = mul(float4(position, 1.0f), g_mWorldViewProj);
    result.uv = float2(texcoord.x, 1.0f - texcoord.y);
    result.normal = normal;
    return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    // calculate ambient
    float ambientStrength = 0.2;
    float3 ambient_color;
    if (map_Ka != 0)
    {
        ambient_color = tex_Ka.Sample(g_sampler, input.uv);
    }
    else
    {
        ambient_color = Ka;
    }
    float3 ambient = ambient_color * ambientStrength;
    // calculate diffuse
    float3 diffuse_color;
    if (g_useDefaultDiffuse == 1 || map_Kd != 0)
    {
        diffuse_color = tex_Kd.Sample(g_sampler, input.uv);
    }
    else
    {
        diffuse_color = Kd;
    }
    float3 lightDir = normalize(g_light);
    float diff = max(dot(input.normal, lightDir), 0.0f);
    float3 diffcolor = diff * diffuse_color;
    // specular
    float3 specular_color;
    if (map_Ks != 0)
    {
        specular_color = tex_Ks.Sample(g_sampler, input.uv);
    }
    else
    {
        specular_color = Ks;
    }
    float exp_value;
    if (map_Ns != 0)
    {
        exp_value = tex_Ns.Sample(g_sampler, input.uv);
    }
    else
    {
        exp_value = Ns;
    }
    exp_value = exp_value < 0.001f ? 0.001f : exp_value;
    float3 viewDir = normalize(g_eyepos - input.position.xyz);
    float3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(input.normal, halfwayDir), 0.0), exp_value);
    float3 specular = specular_color * spec;
    // emission
    //float3 emission_color;
    //if (map_Ke != 0)
    //{
    //    emission_color = tex_Ke.Sample(g_sampler, input.uv);
    //}
    //else
    //{
    //    emission_color = Ke;
    //}
    float3 final = ambient + diffcolor + specular;
    return float4(final, 1.0f);
}
