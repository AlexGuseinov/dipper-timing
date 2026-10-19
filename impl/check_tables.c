/* Checks that the generated const tables (fast_const_tables.h) are
 * byte-identical to the tables init_fast() builds at run time.
 * Includes dipper_fast.c directly to reach its static tables. */
#include <stdio.h>
#include <string.h>
#include "dipper_fast.c"
#include "fast_const_tables.h"
int main(void){
    init_fast();
    int ok = !memcmp(SB2, SB2C, sizeof SB2) && !memcmp(SB2I, SB2IC, sizeof SB2I)
          && !memcmp(PT, PTC, sizeof PT) && !memcmp(PTI, PTIC, sizeof PTI);
    printf("tables: SB2 %zu B, SB2I %zu B, PT %zu B, PTI %zu B, total %zu B\n",
           sizeof SB2, sizeof SB2I, sizeof PT, sizeof PTI,
           sizeof SB2 + sizeof SB2I + sizeof PT + sizeof PTI);
    printf("const tables byte-identical to init_fast(): %s\n", ok ? "YES" : "NO");
    return !ok;
}
