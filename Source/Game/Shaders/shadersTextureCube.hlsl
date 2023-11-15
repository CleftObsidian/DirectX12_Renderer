struct ModelViewProjection
{
    matrix MVP;
};
ConstantBuffer<ModelViewProjection> ModelViewProjectionCB : register(b0);

struct PSInput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD;
};

Texture2D g_texture : register(t0);
SamplerState g_sampler : register(s0);

PSInput VSMain(float4 position : POSITION, float4 texCoord : TEXCOORD)
{
    PSInput result;

    result.position = mul(ModelViewProjectionCB.MVP, position);
    result.texCoord = texCoord;

    return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return g_texture.Sample(g_sampler, input.texCoord);
}