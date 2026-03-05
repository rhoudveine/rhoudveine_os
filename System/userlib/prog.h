/* prog.h — Standard header for all Rhoudveine userspace programs.
 * Every program that lives in /System/Rhoudveine/Programs/ uses this
 * interface. The shell (rash) passes kernel_services + argv when calling
 * a program's entry point.
 */
#pragma once
#include <stdint.h>
#include <stddef.h>
#include "kernel_services.h"

/* Every program exports this function as its entry point. */
void program_main(kernel_services_t *services, int argc, const char **argv);

/* ── Output helpers (implementation in prog_helpers.h, inlined) ── */
static inline void p_puts(kernel_services_t *ks, const char *s) {
    if (ks && ks->puts) ks->puts(s);
}
static inline void p_putchar(kernel_services_t *ks, char c) {
    if (ks && ks->putchar) ks->putchar((int)c);
}
static inline void p_uint(kernel_services_t *ks, uint64_t v) {
    if (v == 0) { p_putchar(ks, '0'); return; }
    char buf[24]; int len = 0;
    while (v > 0) { buf[len++] = '0' + (int)(v % 10); v /= 10; }
    for (int i = len-1; i >= 0; i--) p_putchar(ks, buf[i]);
}
static inline void p_hex(kernel_services_t *ks, uint64_t v) {
    const char *h = "0123456789ABCDEF";
    p_puts(ks, "0x");
    if (v == 0) { p_putchar(ks, '0'); return; }
    char buf[18]; int len = 0;
    while (v > 0) { buf[len++] = h[v & 0xF]; v >>= 4; }
    for (int i = len-1; i >= 0; i--) p_putchar(ks, buf[i]);
}
