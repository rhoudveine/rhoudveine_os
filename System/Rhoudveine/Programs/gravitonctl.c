#include <stdint.h>
#include "prog.h"

void program_main(kernel_services_t *ks, int argc, const char **argv) {
    (void)argc; (void)argv;
    p_puts(ks, "gravitonctl: service and daemon manager\n");
    p_puts(ks, "             not yet implemented\n");
}
void main(kernel_services_t *ks) { program_main(ks, 0, (void*)0); }
