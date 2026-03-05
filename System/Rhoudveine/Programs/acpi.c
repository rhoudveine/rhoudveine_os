#include <stdint.h>
#include "prog.h"

void program_main(kernel_services_t *ks, int argc, const char **argv) {
    (void)argc; (void)argv;

    p_puts(ks, "\n  ACPI System Information\n");
    p_puts(ks, "  -----------------------\n");

    if (!ks) { p_puts(ks, "  Kernel services unavailable\n\n"); return; }

    p_puts(ks, "  CPU count    : ");
    p_uint(ks, (uint64_t)ks->acpi_cpu_count);
    p_puts(ks, "\n");

    p_puts(ks, "  Local APIC   : ");
    p_hex(ks, (uint64_t)ks->acpi_local_apic_addr);
    p_puts(ks, "\n\n");
}
void main(kernel_services_t *ks) { program_main(ks, 0, (void*)0); }
