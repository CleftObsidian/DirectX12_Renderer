struct cbEveryFrame
{
    matrix World;
    matrix View;
    matrix Projection;
    float3 CameraPos;
};
ConstantBuffer<cbEveryFrame> cbPerFrame : register(b0);

Texture2D g_texture : register(t0);
SamplerState g_sampler : register(s0);

struct DSOutput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD;
    float3 normal : NORMAL;
    float3 worldPos : POSITION;
};

float4 PSMain(DSOutput input) : SV_TARGET
{
    float3 directionalLightDirection = float3(-1.0f, -1.0f, -1.0f);
    
    float3 ambient = float3(0.5f, 0.5f, 0.5f);
    
    float3 diffuse = saturate(dot(input.normal, directionalLightDirection));
    
    float3 viewDirection = normalize(cbPerFrame.CameraPos - input.worldPos);
    float3 reflectDirection = reflect(-directionalLightDirection, input.normal);
    
    return float4(ambient + diffuse, 1.0f) * g_texture.Sample(g_sampler, input.texCoord);
}