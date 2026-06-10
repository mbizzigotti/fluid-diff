#ifndef INCLUDE_OPTIMIZATION
#define INCLUDE_OPTIMIZATION
#endif
#include <stdio.h>
#include "fluid.h"
#include "3rdparty/lbfgs.h"

static int progress(
    void *instance,
    const lbfgsfloatval_t *x,
    const lbfgsfloatval_t *g,
    const lbfgsfloatval_t fx,
    const lbfgsfloatval_t xnorm,
    const lbfgsfloatval_t gnorm,
    const lbfgsfloatval_t step,
    int n,
    int k,
    int ls
    )
{
    printf("Iteration %d: objective = %f\n", k, fx);
    return 0;
}
