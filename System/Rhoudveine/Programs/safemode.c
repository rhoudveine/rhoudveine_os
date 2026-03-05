#include <stdint.h>
#include "prog.h"

void program_main(kernel_services_t *ks, int argc, const char **argv) {
    (void)argc; (void)argv;
    p_puts(ks, "safemode: not yet implemented\n");
    p_puts(ks, "         (kernel safe mode will be added in a future release)\n");
}
void main(kernel_services_t *ks) { program_main(ks, 0, (void*)0); }
