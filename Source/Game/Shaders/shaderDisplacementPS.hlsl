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

struct DS_OUTPUT
{
    float4 pos : SV_POSITION;
    float3 worldpos : POSITION;
    float2 texCoord : TEXCOORD;
    float3 normal : NORMAL;
};

float4 PSMain(DS_OUTPUT input) : SV_TARGET
{
    float3 directionalLightDirection = float3(-1.0f, -1.0f, -1.0f);
    
    float3 ambient = float3(0.1f, 0.1f, 0.1f);
    
    float3 diffuse = saturate(dot(input.normal, directionalLightDirection)) * 0.5f;
    
    float3 viewDirection = normalize(cbPerFrame.CameraPos - input.worldpos);
    float3 reflectDirection = reflect(-directionalLightDirection, input.normal);
    float shiness = 20.0f;
    //float3 specular = pow(saturate(dot(reflectDirection, viewDirection)), shiness);
    
    return float4(ambient + diffuse, 1.0f) * g_texture.Sample(g_sampler, input.texCoord);
}