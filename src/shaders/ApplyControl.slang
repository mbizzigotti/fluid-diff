import Fluid;

StructuredBuffer<Wind_Force> controls;
RWStructuredBuffer<Atomic<float>> u;
RWStructuredBuffer<Atomic<float>> v;
[[vk_push_constant]] ConstantBuffer<float> t;

[NumThreads(256, 1, 1)]
void main(uint3 id: SV_DispatchThreadID)
{
    fluid.ApplyControl(id.x, controls, u, v, t);
}
