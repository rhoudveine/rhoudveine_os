#include <stdint.h>
#include "prog.h"

void program_main(kernel_services_t *ks, int argc, const char **argv) {
    (void)argc; (void)argv;
    if (!ks || !ks->timer_get_uptime_ms) { p_puts(ks, "Timer unavailable\n"); return; }
    
    uint64_t ms = ks->timer_get_uptime_ms();
    uint64_t s  = ms / 1000;
    uint64_t m  = s / 60;
    uint64_t h  = m / 60;
    s %= 60;
    m %= 60;
    
    p_puts(ks, "Uptime: ");
    p_uint(ks, h); p_puts(ks, "h ");
    p_uint(ks, m); p_puts(ks, "m ");
    p_uint(ks, s); p_puts(ks, "s\n");
}
void main(kernel_services_t *ks) { program_main(ks, 0, (void*)0); }
