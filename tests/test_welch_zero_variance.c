/* Regression test for the zero-variance case of dudect_welch():
 * both classes constant but different -> |t| = infinity, not 0.
 *   cc -O2 -Icommon -Iimpl -Iharness/posix tests/test_welch_zero_variance.c common/dudect.c impl/<variants>.c -lm */
#include <stdio.h>
#include <math.h>
#include "dudect.h"
#include "platform.h"
/* platform stubs: the test only calls dudect_welch() */
const char *plat_timer_name = "none";
const char *plat_name(void){ return "test"; }
void plat_print(const char *line){ puts(line); }
int main(void){
    welch_t w = {{0}};
    for (int i = 0; i < 20000; i++) { w.n[0]++; w.mean[0] = 100; w.n[1]++; w.mean[1] = 101; }
    double t = dudect_welch(&w);
    welch_t e = {{0}};
    for (int i = 0; i < 20000; i++) { e.n[0]++; e.mean[0] = 100; e.n[1]++; e.mean[1] = 100; }
    printf("means 100 vs 101, zero variance: t=%f (expect -inf)\nmeans equal, zero variance: t=%f (expect 0)\n", t, dudect_welch(&e));
    return !(isinf(t) && t < 0 && dudect_welch(&e) == 0.0);
}
