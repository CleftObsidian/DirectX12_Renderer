struct TransformMatrix
{
    matrix World;
    matrix View;
    matrix Projection;
};
ConstantBuffer<TransformMatrix> TransformCB : register(b0);

struct VSInput
{
    float4 Position : POSITION;
    float2 TexCoord : TEXCOORD;
    float3 Normal : NORMAL;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD;
    float3 normal : NORMAL;
    float3 worldPosition : WORLDPOS;
};

Texture2D g_texture : register(t0);
SamplerState g_sampler : register(s0);

PSInput VSMain(VSInput input)
{
    PSInput output;

    output.position = mul(TransformCB.World, input.Position);
    output.worldPosition = output.position;
    output.position = mul(TransformCB.View, output.position);
    output.position = mul(TransformCB.Projection, output.position);
    output.texCoord = input.TexCoord;
    output.normal = normalize(mul(float4(input.Normal, 0.0f), TransformCB.World).xyz);

    return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    float3 directionalLightDirection = float3(-1.0f, -1.0f, -1.0f);
    
    float3 ambient = float3(0.5f, 0.5f, 0.5f);
    
    float3 diffuse = saturate(dot(input.normal, directionalLightDirection));
    
    float3 viewDirection = normalize(float3(0.0f, 0.0f, -10.0f) - input.worldPosition);
    float3 reflectDirection = reflect(-directionalLightDirection, input.normal);
    float shiness = 20.0f;
    float3 specular = pow(saturate(dot(reflectDirection, viewDirection)), shiness);
    
    return float4(ambient + diffuse + specular, 1.0f) * g_texture.Sample(g_sampler, input.texCoord);
}