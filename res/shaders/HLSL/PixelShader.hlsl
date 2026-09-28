struct VsOutput {
    float4 pos : SV_POSITION;
    float4 color : COLOR;
};

float4 main(VsOutput input) : SV_TARGET
{
    return input.color;
}
