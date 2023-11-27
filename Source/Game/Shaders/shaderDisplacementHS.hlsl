struct cbEveryFrame
{
    matrix World;
    matrix View;
    matrix Projection;
    float3 CameraPos;
};
ConstantBuffer<cbEveryFrame> cbPerFrame : register(b0);

#define NUM_CONTROL_POINTS 4

struct VS_CONTROL_POINT_OUTPUT
{
    float3 worldpos : POSITION;
    float2 texCoord : TEXCOORD;
    float3 normal : NORMAL;
};

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

float CalcTessFactor(float3 p)
{
    float d = distance(p, cbPerFrame.CameraPos);

    float s = saturate((d - 16.0f) / (256.0f - 16.0f));
    return pow(2, (lerp(6, 0, s)));
}

HS_CONSTANT_DATA_OUTPUT CalcHSPatchConstants(
	InputPatch<VS_CONTROL_POINT_OUTPUT, NUM_CONTROL_POINTS> ip,
	uint PatchID : SV_PrimitiveID)
{
    HS_CONSTANT_DATA_OUTPUT output;

	// tessellate based on distance from the camera.
	// compute tess factor based on edges.
	// compute midpoint of edges.
    float3 e0 = 0.5f * (ip[0].worldpos + ip[2].worldpos);
    float3 e1 = 0.5f * (ip[0].worldpos + ip[1].worldpos);
    float3 e2 = 0.5f * (ip[1].worldpos + ip[3].worldpos);
    float3 e3 = 0.5f * (ip[2].worldpos + ip[3].worldpos);
    float3 c = 0.25f * (ip[0].worldpos + ip[1].worldpos + ip[2].worldpos + ip[3].worldpos);

    output.Edges[0] = CalcTessFactor(e0);
    output.Edges[1] = CalcTessFactor(e1);
    output.Edges[2] = CalcTessFactor(e2);
    output.Edges[3] = CalcTessFactor(e3);
    output.Inside[0] = CalcTessFactor(c);
    output.Inside[1] = output.Inside[0];

    return output;
}

[domain("quad")]
[partitioning("integer")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(NUM_CONTROL_POINTS)]
[patchconstantfunc("CalcHSPatchConstants")]
HS_CONTROL_POINT_OUTPUT HSMain(
	InputPatch<VS_CONTROL_POINT_OUTPUT, NUM_CONTROL_POINTS> ip,
	uint i : SV_OutputControlPointID,
	uint PatchID : SV_PrimitiveID)
{
    HS_CONTROL_POINT_OUTPUT output;

    output.worldpos = ip[i].worldpos;
    output.texCoord = ip[i].texCoord;
    output.normal = ip[i].normal;

    return output;
}
