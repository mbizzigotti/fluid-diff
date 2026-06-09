#include "config.h"

int main(int argc, char *argv[])
{
    Config config = Config_Default();
    if (argc < 2)
    {
        return printf("Usage:\n  %s <file>\n", argv[0]);
    }
    if (Config_Load(argv[1], &config) < 0)
    {
        return printf(ERROR": Failed to parse config file \"%s\"\n", argv[1]);
    }
    Config_Print(&config, stdout);
}