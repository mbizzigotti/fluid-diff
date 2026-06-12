import Fluid;

[NumThreads(256, 1, 1)]
void update(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint thread_id = dispatchThreadID.x;
}
