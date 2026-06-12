
#define CONTROL_PENALTY 0.000005f
#define for_n(I,A,B) for (int I = (A); I < (B); I++)

struct Param {
    float cell_size;
    int cell_count;
    int grid_dim_x;
    int grid_dim_y;
    int key_dim_x;
    int key_dim_y;
    int num_timesteps;
    float step_size;
    float final_t;
};
[vk::binding(0, 0)] ConstantBuffer<Param> param;

struct Wind_Force {
    float x; // position
    float y;
    float radius;   // how spread out the force is
    float theta;    // direction of the force
    float strength; // how strong the force is
    float t;        // when to apply the force
    float interval; // how long to apply the force
    float _padding;
};

struct Fluid
{
    // StructuredBuffer<float> s;
    // StructuredBuffer<float> u;
    // StructuredBuffer<float> v;
    // StructuredBuffer<float> m;
    // StructuredBuffer<float> t1;
    // StructuredBuffer<float> t2;
    StructuredBuffer<float> keyframe;
    StructuredBuffer<float> objective;

    int get(int x, int y)
    {
        return y * param.grid_dim_x + x;
    }

    void ApplyControl(
        uint i,
        StructuredBuffer<Wind_Force> controls,
        RWStructuredBuffer<Atomic<float>> u,
        RWStructuredBuffer<Atomic<float>> v,
        float t)
    {
        if (i >= controls.getCount())
            return;

        const float eps = 0.00001;
        float h = param.cell_size;

        float r = controls[i].radius * 2.0;
        int x0 = max((int)((controls[i].x - r + h * 0.5f) / h), 2);
        int x1 = min((int)((controls[i].x + r + h * 0.5f) / h), param.grid_dim_x - 3) + 1;
        int y0 = max((int)((controls[i].y - r + h * 0.5f) / h), 2);
        int y1 = min((int)((controls[i].y + r + h * 0.5f) / h), param.grid_dim_y - 3) + 1;

        // How strong the gaussian is in time
        float dt = controls[i].t - t;
        float time_magnitude = exp(-1.0f * dt * dt / (controls[i].interval * controls[i].interval + eps));

        float dir_x = cos(controls[i].theta);
        float dir_y = sin(controls[i].theta);
        float acc_x = controls[i].strength * controls[i].strength * dir_x * time_magnitude;
        float acc_y = controls[i].strength * controls[i].strength * dir_y * time_magnitude;

        for_n(y, y0, y1)
        for_n(x, x0, x1)
        {
            float cell_center_x = float(x)*h + h * 0.5f;
            float cell_center_y = float(y)*h + h * 0.5f;
            float dx = cell_center_x - controls[i].x;
            float dy = cell_center_y - controls[i].y;
            float r2 = dx * dx + dy * dy;

            // How strong the gaussian is in space
            float space_magnitude = exp(-1.0f * r2 / (controls[i].radius * controls[i].radius + eps));

            // Add to velocity field
            u[get(x, y)] += acc_x * space_magnitude;
            v[get(x, y)] += acc_y * space_magnitude;
        }
    }

    float MatchKeyframe(StructuredBuffer<float> m, StructuredBuffer<Wind_Force> controls, float t)
    {
        // TODO: Use thread id?
        float data_term = 0.0f;
        float regularization_term = 0.0f;
        for_n(y, 0, param.key_dim_y)
        for_n(x, 0, param.key_dim_x)
        {
            float target = keyframe[y * param.key_dim_x + x];
            float real = m[get(x + 1, y + 1)];
            float image_difference = real - target;
            float image_contribution = image_difference * image_difference;
            data_term += image_contribution;
        }
        data_term *= exp((t - param.final_t) * 3.0f); // NOTE: 2 can be adjusted
        for_n(i, 0, controls.getCount())
        {
            float volume = controls[i].radius * controls[i].radius * controls[i].interval * controls[i].interval;
            float control_contribution = CONTROL_PENALTY * volume * controls[i].strength * controls[i].strength;
            regularization_term += control_contribution;
        }
        return data_term + regularization_term;
    }
};

ParameterBlock<Fluid> fluid;
