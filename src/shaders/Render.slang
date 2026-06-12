// Simple compute shader that fills a storage image.
// Bound as a STORAGE_IMAGE at set 0, binding 0.

[[vk::binding(0, 0)]]
RWTexture2D<float4> output;

[shader("compute")]
[NumThreads(16, 16, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    uint width, height;
    output.GetDimensions(width, height);
    if (id.x >= width || id.y >= height)
        return;

    float2 uv = float2(id.xy) / float2(width, height);

    // A simple gradient so we can see something on screen.
    float3 color = float3(uv, 0.5);
    output[id.xy] = float4(color, 1.0);
}
