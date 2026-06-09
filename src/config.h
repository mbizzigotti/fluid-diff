#ifndef CONFIG_H
#define CONFIG_H
#include "fluid.h"
#include "util.h"
#include "3rdparty/ini.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static inline Config Config_Default()
{
    return (Config) {
        .name = "unnamed",
        .method = Method_Gradient_Decent,
        .step_size = 0.02f,
        .width = 100,
        .height = 100,
        .cell_size = 4.0f,
        .num_controls = 100,
        .num_keyframes = 0,
    };
}

static inline void Config_Print(Config *config, FILE *f)
{
    fprintf(f, "[parameters]\n");
    fprintf(f, "name=%s\n", config->name);
    fprintf(f, "method=%s\n", Method_Name(config->method));
    fprintf(f, "stepsize=%f\n", config->step_size);
    fprintf(f, "width=%i\n", config->width);
    fprintf(f, "height=%i\n", config->height);
    fprintf(f, "controls=%i\n", config->num_controls);
    fprintf(f, "\n");
    fprintf(f, "[keyframes]\n");
    assert(config->num_keyframes <= array_count(config->keyframes));
    for (int i = 0; i < config->num_keyframes; ++i)
        fprintf(f, "%i=%s\n", config->keyframe_times[i], config->keyframes[i]);
}

static inline int __config(void* user, const char* section, const char* name, const char* value)
{
    Config* config = (Config*)user;

    if (0);
    else if (0==strcmp(section, "parameters"))
    {
        if (0);
        else if (0==strcmp(name, "name"))     config->name = clone_cstring(value);
        else if (0==strcmp(name, "method"))   parse_method(value, &config->method);
        else if (0==strcmp(name, "stepsize")) parse_f32(value, &config->step_size);
        else if (0==strcmp(name, "cellsize")) parse_f32(value, &config->cell_size);
        else if (0==strcmp(name, "width"))    parse_int(value, &config->width);
        else if (0==strcmp(name, "height"))   parse_int(value, &config->height);
        else if (0==strcmp(name, "controls")) parse_int(value, &config->num_controls);
        else return 0; /* unknown section/name, error */
    }
    else if (0==strcmp(section, "keyframes"))
    {
        int n = config->num_keyframes;
        parse_int(name, &config->keyframe_times[n]);
        config->keyframes[n] = clone_cstring(value);
        config->num_keyframes += 1;
    }
    else return 0; /* unknown section/name, error */
    return 1;
}

static inline int Config_Load(const char* filename, Config *config)
{
    return ini_parse(filename, __config, config);
}

#endif // CONFIG_H