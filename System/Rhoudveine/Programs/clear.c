#include <stdint.h>
#include "prog.h"

void program_main(kernel_services_t *ks, int argc, const char **argv) {
    (void)argc; (void)argv;
    /* Clear by printing enough newlines to scroll all content off-screen */
    for (int i = 0; i < 50; i++) p_putchar(ks, '\n');
}
void main(kernel_services_t *ks) { program_main(ks, 0, (void*)0); }
