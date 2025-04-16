struct Material
{
    float4 color;
};

ConstantBuffer<Material> gMaterial : register(b0);

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

struct PixelShaderInput
{
    float4 position : SV_POSITION;
};

PixelShaderOutput main(PixelShaderInput input)
{
    PixelShaderOutput output;
    output.color = gMaterial.color;
    return output;
}