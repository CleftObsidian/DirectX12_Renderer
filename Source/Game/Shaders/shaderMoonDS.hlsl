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

struct HS_CONSTANT_DATA_OUTPUT
{
    float Edges[3] : SV_TessFactor;
    float Inside : SV_InsideTessFactor;
};

struct HSOutput
{
    float3 position : POSITION;
    float2 texCoord : TEXCOORD;
    float3 normal : NORMAL;
};

struct DSOutput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD;
    float3 normal : NORMAL;
    float3 worldPos : POSITION;
};

#define NUM_CONTROL_POINTS 3

[domain("tri")]
DSOutput DSMain(
	HS_CONSTANT_DATA_OUTPUT input,
	float3 uvwCoord : SV_DomainLocation,
	const OutputPatch<HSOutput, NUM_CONTROL_POINTS> patch)
{
    DSOutput output;
    
    float3 vertexPosition = uvwCoord.x * patch[0].position + uvwCoord.y * patch[1].position + uvwCoord.z * patch[2].position;
    output.texCoord = uvwCoord.x * patch[0].texCoord + uvwCoord.y * patch[1].texCoord + uvwCoord.z * patch[2].texCoord;
    output.normal = uvwCoord.x * patch[0].normal + uvwCoord.y * patch[1].normal + uvwCoord.z * patch[2].normal;

    float displacement = displacementmap.SampleLevel(displacementsampler, output.texCoord.xy, 0.0f).r;
    displacement *= 0.003f;   // scale
    displacement += 0.0f;   // bias
    vertexPosition += output.normal * displacement;
    
    output.position = mul(cbPerFrame.World, float4(vertexPosition, 1.0f));
    output.worldPos = output.position;
    output.position = mul(cbPerFrame.View, output.position);
    output.position = mul(cbPerFrame.Projection, output.position);

	return output;
}