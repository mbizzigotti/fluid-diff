
f32 Fluid_Sample(Fluid *fluid, f32 px, f32 py)
{
    f32 h = fluid->cell_size;
    f32 inv_h = 1.0f / h;

    f32 x = (clamp_f32(px, h, (f32)(fluid->grid_dim_x) * h));
    f32 y = (clamp_f32(py, h, (f32)(fluid->grid_dim_y) * h));

#define u1 0
#define v1 1
#define m1 2
#if field == u1
    y -= 0.5f * h;
#elif field == v1
    x -= 0.5f * h;
#elif field == m1
    x -= 0.5f * h; y -= 0.5f * h;
#endif
#undef u1
#undef v1
#undef m1

    // Compute indices
    int x0 = min_int((int)(floorf(x*inv_h)), fluid->grid_dim_x - 1);
    int x1 = min_int(x0 + 1, fluid->grid_dim_x - 1);
    int y0 = min_int((int)(floorf(y*inv_h)), fluid->grid_dim_y - 1);
    int y1 = min_int(y0 + 1, fluid->grid_dim_y - 1);

    // Compute weight values
    f32 tx = (x - (f32)(x0) * h) * inv_h;
    f32 ty = (y - (f32)(y0) * h) * inv_h;
    f32 sx = 1.0 - tx;
    f32 sy = 1.0 - ty;

    return sx*sy*get(field, x0, y0)
         + tx*sy*get(field, x1, y0)
         + tx*ty*get(field, x1, y1)
         + sx*ty*get(field, x0, y1);
}