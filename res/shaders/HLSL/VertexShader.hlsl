struct VsInput {
    float3 pos : POSITION;
};

struct VsOutput {
    float4 pos : SV_POSITION;
    float4 color : COLOR;
};

VsOutput main(VsInput input)
{
    VsOutput output;
    output.pos = float4(input.pos, 1.0);
    output.color = float4(input.pos * 0.5 + 0.5, 1.0);
    return output;
}
