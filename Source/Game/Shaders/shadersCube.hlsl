struct ModelViewProjection
{
    matrix MVP;
};
ConstantBuffer<ModelViewProjection> ModelViewProjectionCB : register(b0);

struct VertexPosColor
{
    float3 Position : POSITION;
    float3 Color : COLOR;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

PSInput VSMain(VertexPosColor IN)
{
    PSInput result;

    result.position = mul(ModelViewProjectionCB.MVP, float4(IN.Position, 1.0f));
    result.color = float4(IN.Color, 1.0f);

    return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return input.color;
}