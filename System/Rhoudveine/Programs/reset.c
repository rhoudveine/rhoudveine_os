#include <stdint.h>
#include "prog.h"

void program_main(kernel_services_t *ks, int argc, const char **argv) {
    (void)argc; (void)argv;
    for (int i = 0; i < 50; i++) p_putchar(ks, '\n');
    p_puts(ks, "rash - Rhoudveine Adaptive Shell\n");
    p_puts(ks, "Type 'help' for commands.\n\n");
}
void main(kernel_services_t *ks) { program_main(ks, 0, (void*)0); }
