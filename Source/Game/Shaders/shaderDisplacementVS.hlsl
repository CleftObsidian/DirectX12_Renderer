struct cbEveryFrame
{
    matrix World;
    matrix View;
    matrix Projection;
    float3 CameraPos;
};
ConstantBuffer<cbEveryFrame> cbPerFrame : register(b0);

struct VS_CONTROL_POINT_INPUT
{
    float3 Position : POSITION;
    float2 TexCoord : TEXCOORD;
    float3 Normal : NORMAL;
};

struct VS_CONTROL_POINT_OUTPUT
{
    float3 worldpos : POSITION;
    float2 texCoord : TEXCOORD;
    float3 normal : NORMAL;
};

VS_CONTROL_POINT_OUTPUT VSMain(VS_CONTROL_POINT_INPUT input)
{
    VS_CONTROL_POINT_OUTPUT output;

    output.worldpos = mul(float4(input.Position, 1.0f), cbPerFrame.World);
    output.texCoord = input.TexCoord;
    output.normal = input.Normal;

    return output;
}