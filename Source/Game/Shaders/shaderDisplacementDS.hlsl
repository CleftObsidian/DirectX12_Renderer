struct cbEveryFrame
{
    matrix World;
    matrix View;
    matrix Projection;
    float3 CameraPos;
};
ConstantBuffer<cbEveryFrame> cbPerFrame : register(b0);

Texture2D displacementmap : register(t1);
SamplerState displacementsampler : register(s1);

#define NUM_CONTROL_POINTS 4

struct HS_CONTROL_POINT_OUTPUT
{
    float3 worldpos : POSITION;
    float2 texCoord : TEXCOORD;
    float3 normal : NORMAL;
};

struct HS_CONSTANT_DATA_OUTPUT
{
    float Edges[4] : SV_TessFactor;
    float Inside[2] : SV_InsideTessFactor;
};

struct DS_OUTPUT
{
    float4 pos : SV_POSITION;
    float3 worldpos : POSITION;
    float2 texCoord : TEXCOORD;
    float3 normal : NORMAL;
};

[domain("quad")]
DS_OUTPUT DSMain(HS_CONSTANT_DATA_OUTPUT input,
    float2 domain : SV_DomainLocation,
    const OutputPatch<HS_CONTROL_POINT_OUTPUT, NUM_CONTROL_POINTS> patch)
{
    DS_OUTPUT output;

    output.worldpos = lerp(lerp(patch[0].worldpos, patch[1].worldpos, domain.x), lerp(patch[2].worldpos, patch[3].worldpos, domain.x), domain.y);
    output.texCoord = lerp(lerp(patch[0].texCoord, patch[1].texCoord, domain.x), lerp(patch[2].texCoord, patch[3].texCoord, domain.x), domain.y);
    output.normal = lerp(lerp(patch[0].normal, patch[1].normal, domain.x), lerp(patch[2].normal, patch[3].normal, domain.x), domain.y);
    
    output.worldpos = output.normal * 0.5f * (2.0f * displacementmap.SampleLevel(displacementsampler, output.worldpos / 32, 0.0f).w - 1.0f);
    
    // generate coordinates transformed into view/projection space.
    output.pos = float4(output.worldpos, 1.0f);
    output.pos = mul(output.pos, cbPerFrame.View);
    output.pos = mul(output.pos, cbPerFrame.Projection);

    return output;
}
