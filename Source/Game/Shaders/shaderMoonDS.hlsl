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
    float3 worldPos : POSITION;
    float2 texCoord : TEXCOORD;
    float3 normal : NORMAL;
};

#define NUM_CONTROL_POINTS 3

[domain("tri")]
DSOutput DSMain(
	HS_CONSTANT_DATA_OUTPUT input,
	float3 uvwCoord : SV_DomainLocation,
	const OutputPatch<HSOutput, NUM_CONTROL_POINTS> patch)
{
    float3 vertexPosition;
    float3 vertexNormal;
    float2 textureCoord;
    DSOutput output;
    
    vertexPosition = uvwCoord.x * patch[0].position + uvwCoord.y * patch[1].position + uvwCoord.z * patch[2].position;
    vertexNormal = uvwCoord.x * patch[0].normal + uvwCoord.y * patch[1].normal + uvwCoord.z * patch[2].normal;
    textureCoord = uvwCoord.x * patch[0].texCoord + uvwCoord.y * patch[1].texCoord + uvwCoord.z * patch[2].texCoord;
    
    output.position = mul(float4(vertexPosition, 1.0f), cbPerFrame.World);
    output.normal = mul(float4(vertexNormal, 1.0f), cbPerFrame.World);
    output.texCoord = textureCoord;

    float displacementScale = 10.0f;
    //output.position += displacementScale * displacementmap.SampleLevel(displacementsampler, output.texCoord, 0.0f).r * output.normal;
    
    output.worldPos = output.position;
    output.position = mul(output.position, cbPerFrame.View);
    output.position = mul(output.position, cbPerFrame.Projection);

	return output;
}