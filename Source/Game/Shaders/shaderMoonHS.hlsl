struct VSOutput
{
    float3 position : POSITION;
    float2 texCoord : TEXCOORD;
    float3 normal : NORMAL;
};

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

#define NUM_CONTROL_POINTS 3

HS_CONSTANT_DATA_OUTPUT CalcHSPatchConstants(
	InputPatch<VSOutput, NUM_CONTROL_POINTS> ip,
	uint PatchID : SV_PrimitiveID)
{
	HS_CONSTANT_DATA_OUTPUT output;

    float tessellationAmount = 2.0f;
    
    output.Edges[0] = tessellationAmount;
    output.Edges[1] = tessellationAmount;
    output.Edges[2] = tessellationAmount;
    
    output.Inside = tessellationAmount;

	return output;
}

[domain("tri")]
[partitioning("fractional_odd")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(NUM_CONTROL_POINTS)]
[patchconstantfunc("CalcHSPatchConstants")]
VSOutput HSMain(
	InputPatch<VSOutput, NUM_CONTROL_POINTS> ip,
	uint i : SV_OutputControlPointID,
	uint PatchID : SV_PrimitiveID )
{
    VSOutput output;

    output.position = ip[i].position;
    output.texCoord = ip[i].texCoord;
    output.normal = ip[i].normal;

	return output;
}
