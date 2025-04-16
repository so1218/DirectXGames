struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

struct PixelShaderInput
{
    float4 position : SV_POSITION;
};

struct Material
{
    float4 color;
};

ConstantBuffer<Material> gMaterial : register(b0);


PixelShaderOutput main(PixelShaderInput input)
{
    PixelShaderOutput output;
    output.color = float4(1.0f, 1.0f, 1.0f, 1.0f);
    return output;
}