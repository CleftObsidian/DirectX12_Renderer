struct ConstantColor
{
    float r;
    float g;
    float b;
    float a;
};
ConstantBuffer<ConstantColor> cColor : register(b0, space0);

struct PSInput
{
    float4 position : SV_POSITION;
};

PSInput VSMain(float4 position : POSITION)
{
    PSInput result;

    result.position = position;

    return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return float4(cColor.r, cColor.g, cColor.b, cColor.a);
}