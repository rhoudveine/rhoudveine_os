/* graviton.c — Rhoudveine OS PID 1 Init Process
 *
 * Minimal responsibilities:
 *   1. Display the Graviton banner
 *   2. Execute /System/Rhoudveine/Booter/initrc (boot script)
 *   3. Load and run /System/Rhoudveine/Programs/rash/rash (the shell)
 *
 * graviton is NOT a shell. It is the init process that sets up the system
 * and hands control to rash. Programs live on the filesystem, not here.
 */

#include <stdint.h>
#include <stddef.h>
#include "kernel_services.h"

static kernel_services_t *g_services = NULL;

static void out(const char *s) {
    if (g_services && g_services->puts) g_services->puts(s);
}

static int my_strlen(const char *s) { int i=0; while(s&&s[i]) i++; return i; }

static void run_initrc(const char *path) {
    if (!g_services) return;

    int fd = g_services->vfs_open(path, 0);
    if (fd < 0) {
        out("graviton: warning: initrc not found at ");
        out(path); out("\n");
        return;
    }

    char line[128]; int pos = 0;
    uint8_t byte;
    while (g_services->vfs_read(fd, &byte, 1) == 1) {
        if (byte == '\n' || byte == '\r') {
            if (pos > 0) {
                line[pos] = '\0';
                /* Strip comments */
                if (line[0] != '#') {
                    /* Only mount and mkdir are needed in initrc */
                    if (line[0] == 'm' && line[1] == 'k' &&
                        g_services->vfs_mkdir) {
                        g_services->vfs_mkdir(line + 6); /* skip "mkdir " */
                    } else if (line[0] == 'm' && line[1] == 'o' &&
                               g_services->vfs_mount) {
                        /* mount <path> <type> <dev> */
                        const char *p = line + 6;
                        char mpath[64], mtype[32], mdev[64];
                        int i = 0;
                        while (*p && *p != ' ' && i < 63) mpath[i++] = *p++;
                        mpath[i] = '\0'; if (*p==' ') p++;
                        i = 0;
                        while (*p && *p != ' ' && i < 31) mtype[i++] = *p++;
                        mtype[i] = '\0'; if (*p==' ') p++;
                        i = 0;
                        while (*p && *p != ' ' && i < 63) mdev[i++]  = *p++;
                        mdev[i]  = '\0';
                        int r = g_services->vfs_mount(mpath, mtype, mdev);
                        if (r == 0) {
                            out("Mounted "); out(mtype);
                            out(" at ");    out(mpath); out("\n");
                        }
                    } else if (line[0] == 'e') {
                        /* echo */
                        out(line + 5); out("\n");
                    }
                }
                pos = 0;
            }
        } else if (pos < 127) {
            line[pos++] = (char)byte;
        }
    }
    g_services->vfs_close(fd);
}

void main(kernel_services_t *services) {
    g_services = services;

    if (services && services->beep)
        services->beep(50000000, 1000, 1);

    out("---  INIT GRAVITON ---\n");
    out("Rhoudveine OS - Graviton Init (PID 1)\n");
    out("Running initrc...\n");

    run_initrc("/System/Rhoudveine/Booter/graviton/initrc");

    out("\nStarting rash...\n");

    /* Load and run rash from the filesystem.
     * rash is a separate ELF at /System/Rhoudveine/Programs/rash/rash */
    if (services && services->kernel_exec) {
        const char *rash_path = "/System/Rhoudveine/Programs/rash/rash";
        const char *rash_argv[] = { "rash", NULL };
        int r = services->kernel_exec(rash_path, 1, rash_argv);
        if (r != 0) {
            out("graviton: failed to exec rash (error ");
            /* simple int print */
            char ebuf[12]; int el = 0;
            int ev = (r < 0) ? -r : r;
            if (ev == 0) { ebuf[el++] = '0'; }
            else { while (ev > 0) { ebuf[el++] = '0' + ev % 10; ev /= 10; } }
            ebuf[el] = '\0';
            /* reverse */
            for (int i = 0, j = el-1; i < j; i++, j--) {
                char t = ebuf[i]; ebuf[i] = ebuf[j]; ebuf[j] = t;
            }
            if (r < 0) out("-");
            out(ebuf); out(")\n");
            out("graviton: halting.\n");
            for (;;) { __asm__("cli; hlt"); }
        }
    } else {
        out("graviton: kernel_exec not available, halting.\n");
        for (;;) { __asm__("cli; hlt"); }
    }

    for (;;) { __asm__("hlt"); }
}
