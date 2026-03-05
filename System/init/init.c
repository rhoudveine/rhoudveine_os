/* Graviton — Rhoudveine OS Init Process (PID 1)
 *
 * graviton is the main init process. It boots the system, runs initrc,
 * and then hands control to rash (the Rhoudveine Adaptive Shell), which
 * is the interactive command-line interface built into this binary.
 *
 * Per programs_name_lists.txt:
 *   graviton    — this binary (PID 1)
 *   init        — symlink/copy of graviton
 *   rash        — the shell (built-in to graviton, prompt: rash>)
 *   gravitonctl — daemon manager (stub)
 *   gravitonadm — security manager (stub)
 */

#include <stdint.h>
#include <stddef.h>
#include "kernel_services.h"

static kernel_services_t *g_services = NULL;

/* Weak kernel helpers */
extern void beep(uint32_t freq, uint64_t duration) __attribute__((weak));
extern int getchar(void) __attribute__((weak));
extern int putchar(int c) __attribute__((weak));
extern void puts(const char* s) __attribute__((weak));

/* Current working directory */
static char g_cwd[128] = "/";

/* ACPI power management */
extern void acpi_shutdown(void) __attribute__((weak));
extern void acpi_reboot(void)   __attribute__((weak));

/* Timer */
extern uint64_t timer_get_uptime_ms(void) __attribute__((weak));

/* AHCI disk */
extern int ahci_read_sectors(uint64_t lba, uint32_t count, uint8_t *buffer) __attribute__((weak));
extern int ahci_is_initialized(void) __attribute__((weak));

/* Framebuffer helpers */
extern void fb_backspace(void)   __attribute__((weak));
extern void fb_cursor_show(void) __attribute__((weak));
extern void fb_cursor_hide(void) __attribute__((weak));
extern int  try_getchar(void)    __attribute__((weak));
extern void kernel_panic_shell(const char *reason) __attribute__((weak));

/* -------------------------------------------------------------------------
 * Output helpers
 * ---------------------------------------------------------------------- */
void out_puts(const char *s) {
    if (g_services && g_services->fb_cursor_hide) g_services->fb_cursor_hide();
    if (g_services && g_services->puts) g_services->puts(s);
    else if (puts) puts(s);
    if (g_services && g_services->fb_cursor_show) g_services->fb_cursor_show();
}

void out_putchar(char c) {
    if (g_services && g_services->fb_cursor_hide) g_services->fb_cursor_hide();
    if (g_services && g_services->putchar) g_services->putchar((int)c);
    else if (putchar) putchar((int)c);
    if (g_services && g_services->fb_cursor_show) g_services->fb_cursor_show();
}

/* -------------------------------------------------------------------------
 * String helpers
 * ---------------------------------------------------------------------- */
static int my_strlen(const char *s) {
    int i = 0; while (s && s[i]) i++; return i;
}
static int my_strcmp(const char *a, const char *b) {
    if (!a||!b) return (a==b)?0:(a?1:-1);
    int i=0;
    while(a[i]&&b[i]){ if(a[i]!=b[i]) return (int)(a[i]-b[i]); i++; }
    return (int)(a[i]-b[i]);
}
static int my_strncmp(const char *a, const char *b, int n) {
    if(!a||!b) return (a==b)?0:(a?1:-1);
    for(int i=0;i<n;i++){
        if(a[i]=='\0'&&b[i]=='\0') return 0;
        if(a[i]!=b[i]) return (int)(a[i]-b[i]);
        if(a[i]=='\0'||b[i]=='\0') return (int)(a[i]-b[i]);
    }
    return 0;
}

/* Print a uint64_t as decimal */
static void out_uint64(uint64_t v) {
    if (v == 0) { out_putchar('0'); return; }
    char buf[24]; int len = 0;
    while (v > 0) { buf[len++] = '0' + (int)(v % 10); v /= 10; }
    for (int i = len-1; i >= 0; i--) out_putchar(buf[i]);
}

/* Print a uint64_t as hex with "0x" prefix */
static void out_hex(uint64_t v) {
    const char *hex = "0123456789ABCDEF";
    out_puts("0x");
    if (v == 0) { out_putchar('0'); return; }
    char buf[18]; int len = 0;
    while (v > 0) { buf[len++] = hex[v & 0xF]; v >>= 4; }
    for (int i = len-1; i >= 0; i--) out_putchar(buf[i]);
}

/* -------------------------------------------------------------------------
 * Path resolution
 * ---------------------------------------------------------------------- */
static void resolve_path(char *dest, const char *cwd, const char *input) {
    char temp[128];
    int t_len = 0;

    if (input[0] == '/') {
        temp[0] = '/'; temp[1] = '\0'; t_len = 1; input++;
    } else {
        int i = 0;
        while(cwd[i]) { temp[t_len++] = cwd[i]; i++; }
        if (t_len == 0 || temp[t_len-1] != '/') temp[t_len++] = '/';
        temp[t_len] = '\0';
    }

    const char *p = input;
    while (*p) {
        while (*p == '/') p++;
        if (!*p) break;
        const char *end = p;
        while (*end && *end != '/') end++;
        int seg_len = (int)(end - p);

        if (seg_len == 1 && p[0] == '.') {
            /* ignore */
        } else if (seg_len == 2 && p[0] == '.' && p[1] == '.') {
            if (t_len > 1) {
                t_len--;
                while (t_len > 0 && temp[t_len-1] != '/') t_len--;
                temp[t_len] = '\0';
            }
        } else {
            if (t_len > 1 && temp[t_len-1] != '/') temp[t_len++] = '/';
            for (int k=0; k<seg_len; k++) {
                if (t_len < 127) temp[t_len++] = p[k];
            }
            temp[t_len] = '\0';
        }
        p = end;
    }

    if (t_len > 1 && temp[t_len-1] == '/') { temp[t_len-1] = '\0'; }
    else if (t_len == 0) { temp[0] = '/'; temp[1] = '\0'; }
    else { temp[t_len] = '\0'; }

    int i = 0; while(temp[i]) { dest[i] = temp[i]; i++; } dest[i] = '\0';
}

/* -------------------------------------------------------------------------
 * Command implementations
 * ---------------------------------------------------------------------- */

static void cmd_help(void) {
    out_puts("\n");
    out_puts("  Rhoudveine rash - Rhoudveine Adaptive Shell\n");
    out_puts("  ============================================\n\n");
    out_puts("  GRAVITON COMMANDS\n");
    out_puts("  gravitonctl <args>   Daemon/service manager\n");
    out_puts("  gravitonadm <args>   Security manager and audit\n\n");
    out_puts("  CORE UTILITIES\n");
    out_puts("  clear                Clear the terminal\n");
    out_puts("  reset                Reset the terminal\n");
    out_puts("  cdl [path]           List directory contents\n");
    out_puts("  cdir <path>          Create directory\n");
    out_puts("  cd <path>            Change directory\n");
    out_puts("  echo <text>          Echo text\n");
    out_puts("  dump <file>          Display file contents\n");
    out_puts("  write <file> <text>  Write text to file\n");
    out_puts("  mount <p> <t> <d>    Mount filesystem\n");
    out_puts("  uptime               Show system uptime\n");
    out_puts("  diskread <lba>       Read sector from disk\n");
    out_puts("  acpi                 Show ACPI info\n");
    out_puts("  safemode             Switch kernel to safe mode\n");
    out_puts("  sys <args>           System/Kernel API caller\n");
    out_puts("  shutdown             ACPI shutdown\n");
    out_puts("  reboot               ACPI reboot\n");
    out_puts("\n");
}

static void cmd_clear(void) {
    /* Output enough newlines to push old content off screen,
     * then print carriage return to reset cursor. Works on both
     * raw framebuffer consoles and serial terminals. */
    for (int i = 0; i < 50; i++) out_putchar('\n');
}

static void cmd_reset(void) {
    cmd_clear();
    out_puts("rash - Rhoudveine Adaptive Shell\n");
    out_puts("Type 'help' for a list of commands.\n\n");
}

static void cmd_acpi(void) {
    out_puts("\n  ACPI System Information\n");
    out_puts("  -----------------------\n");

    if (!g_services) { out_puts("  Kernel services unavailable\n\n"); return; }

    /* CPU count */
    out_puts("  CPU count    : ");
    out_uint64((uint64_t)g_services->acpi_cpu_count);
    out_puts("\n");

    /* Local APIC address */
    out_puts("  Local APIC   : ");
    out_hex((uint64_t)g_services->acpi_local_apic_addr);
    out_puts("\n");

    out_puts("\n");
}

static void cmd_uptime(void) {
    if (g_services && g_services->timer_get_uptime_ms) {
        uint64_t ms = g_services->timer_get_uptime_ms();
        uint64_t s  = ms / 1000;
        uint64_t m  = s  / 60;
        uint64_t h  = m  / 60;
        s %= 60; m %= 60;
        out_puts("Uptime: ");
        out_uint64(h); out_puts("h ");
        out_uint64(m); out_puts("m ");
        out_uint64(s); out_puts("s\n");
    } else {
        out_puts("Timer not available\n");
    }
}

static void cmd_cdl(const char *arg) {
    if (!g_services || !g_services->vfs_open || !g_services->vfs_readdir || !g_services->vfs_close) {
        out_puts("VFS not available\n"); return;
    }
    char abs_path[128];
    resolve_path(abs_path, g_cwd, (arg && *arg) ? arg : g_cwd);

    int fd = g_services->vfs_open(abs_path, 0);
    if (fd < 0) { out_puts("Cannot open: "); out_puts(abs_path); out_puts("\n"); return; }

    vfs_dirent_t entry;
    int count = 0;
    while (g_services->vfs_readdir(fd, &entry) == 0) {
        out_puts("  ");
        out_puts(entry.name);
        if (entry.type & 0x02) out_puts("/");
        out_puts("\n");
        count++;
    }
    if (count == 0) out_puts("  (empty)\n");
    g_services->vfs_close(fd);
}

static void cmd_cdir(const char *path) {
    if (!path || !*path) { out_puts("Usage: cdir <path>\n"); return; }
    if (!g_services || !g_services->vfs_mkdir) { out_puts("VFS not available\n"); return; }
    char abs_path[128];
    resolve_path(abs_path, g_cwd, path);
    if (g_services->vfs_mkdir(abs_path) != 0) {
        out_puts("cdir: failed to create '"); out_puts(abs_path); out_puts("'\n");
    }
}

static void cmd_cd(const char *target) {
    if (!target || !*target) { out_puts("Usage: cd <path>\n"); return; }
    char new_path[128];
    resolve_path(new_path, g_cwd, target);
    if (g_services && g_services->vfs_open && g_services->vfs_close) {
        int fd = g_services->vfs_open(new_path, 0);
        if (fd >= 0) {
            g_services->vfs_close(fd);
            int n = my_strlen(new_path);
            for (int i = 0; i <= n; i++) g_cwd[i] = new_path[i];
        } else {
            out_puts("cd: no such path: "); out_puts(new_path); out_puts("\n");
        }
    }
}

static void cmd_dump(const char *path) {
    if (!path || !*path) { out_puts("Usage: dump <file>\n"); return; }
    if (!g_services || !g_services->vfs_open || !g_services->vfs_read || !g_services->vfs_close) {
        out_puts("VFS not available\n"); return;
    }
    char abs_path[128];
    resolve_path(abs_path, g_cwd, path);
    int fd = g_services->vfs_open(abs_path, 0);
    if (fd < 0) { out_puts("dump: cannot open '"); out_puts(abs_path); out_puts("'\n"); return; }
    static uint8_t read_buf[512];
    int bytes;
    while ((bytes = g_services->vfs_read(fd, read_buf, sizeof(read_buf))) > 0) {
        for (int i = 0; i < bytes; i++) out_putchar(read_buf[i]);
    }
    out_putchar('\n');
    g_services->vfs_close(fd);
}

static void cmd_write(const char *args) {
    if (!args || !*args) { out_puts("Usage: write <file> <text>\n"); return; }
    if (!g_services || !g_services->vfs_open || !g_services->vfs_write || !g_services->vfs_close) {
        out_puts("VFS not available\n"); return;
    }
    char filename[128]; int i = 0;
    const char *p = args;
    while (*p && *p != ' ' && i < 127) filename[i++] = *p++;
    filename[i] = '\0';
    if (*p == ' ') p++;
    char abs_path[128];
    resolve_path(abs_path, g_cwd, filename);
    int fd = g_services->vfs_open(abs_path, 0x0101);
    if (fd < 0) { out_puts("write: cannot open '"); out_puts(abs_path); out_puts("'\n"); return; }
    int len = my_strlen(p);
    g_services->vfs_write(fd, p, len);
    g_services->vfs_write(fd, "\n", 1);
    g_services->vfs_close(fd);
}

static void cmd_mount(const char *args) {
    if (!g_services || !g_services->vfs_mount) { out_puts("VFS not available\n"); return; }
    if (!args || !*args) { out_puts("Usage: mount <path> <type> <dev>\n"); return; }
    const char *p = args;
    char path[64], type[32], dev[64];
    int i = 0;
    while (*p && *p != ' ' && i < 63) path[i++] = *p++;  path[i] = '\0'; if (*p==' ') p++;
    i = 0;
    while (*p && *p != ' ' && i < 31) type[i++] = *p++;  type[i] = '\0'; if (*p==' ') p++;
    i = 0;
    while (*p && *p != ' ' && i < 63) dev[i++]  = *p++;  dev[i]  = '\0';
    if (g_services->vfs_mount(path, type, dev) == 0) {
        out_puts("Mounted "); out_puts(type); out_puts(" at "); out_puts(path); out_puts("\n");
    } else {
        out_puts("mount: failed\n");
    }
}

static void cmd_diskread(const char *args) {
    if (!g_services || !g_services->ahci_is_initialized || !g_services->ahci_read_sectors) {
        out_puts("AHCI driver not available\n"); return;
    }
    if (!g_services->ahci_is_initialized()) { out_puts("AHCI not initialized\n"); return; }
    if (!args || !*args) { out_puts("Usage: diskread <lba>\n"); return; }
    uint64_t lba = 0;
    const char *p = args;
    while (*p >= '0' && *p <= '9') { lba = lba * 10 + (*p - '0'); p++; }
    static uint8_t sector_buf[512];
    int result = g_services->ahci_read_sectors(lba, 1, sector_buf);
    if (result == 0) out_puts("diskread: success\n");
    else             out_puts("diskread: failed\n");
}

/* -------------------------------------------------------------------------
 * Main command dispatcher
 * ---------------------------------------------------------------------- */
static void execute_command(char *buf) {
    if (!buf || buf[0] == '\0' || buf[0] == '#') return;

    /* Strip trailing whitespace */
    int len = my_strlen(buf);
    while (len > 0 && (buf[len-1] == ' ' || buf[len-1] == '\r')) buf[--len] = '\0';
    if (len == 0) return;

    /* ── Single-word commands ── */
    if (my_strcmp(buf, "help")    == 0) { cmd_help();    return; }
    if (my_strcmp(buf, "clear")   == 0) { cmd_clear();   return; }
    if (my_strcmp(buf, "reset")   == 0) { cmd_reset();   return; }
    if (my_strcmp(buf, "acpi")    == 0) { cmd_acpi();    return; }
    if (my_strcmp(buf, "uptime")  == 0) { cmd_uptime();  return; }
    if (my_strcmp(buf, "cdl")     == 0) { cmd_cdl(NULL); return; }

    if (my_strcmp(buf, "shutdown") == 0) {
        if (g_services && g_services->acpi_shutdown) {
            out_puts("Initiating ACPI shutdown...\n");
            g_services->acpi_shutdown();
        } else {
            out_puts("ACPI shutdown not available, halting\n");
            for (;;) { __asm__("cli; hlt"); }
        }
        return;
    }

    if (my_strcmp(buf, "reboot") == 0) {
        if (g_services && g_services->acpi_reboot) {
            out_puts("Initiating ACPI reboot...\n");
            g_services->acpi_reboot();
        } else {
            out_puts("ACPI reboot not available, halting\n");
            for (;;) { __asm__("cli; hlt"); }
        }
        return;
    }

    if (my_strcmp(buf, "panic") == 0) {
        if (g_services && g_services->kernel_panic_shell)
            g_services->kernel_panic_shell("manual panic from rash");
        return;
    }

    /* safemode — stub */
    if (my_strcmp(buf, "safemode") == 0) {
        out_puts("safemode: safe mode not yet implemented\n");
        return;
    }

    /* rash — self-referential: this IS rash */
    if (my_strcmp(buf, "rash") == 0) {
        out_puts("rash: Rhoudveine Adaptive Shell - you are already in rash.\n");
        return;
    }

    /* ── Commands with arguments ── */
    if (my_strncmp(buf, "echo ",        5) == 0) { out_puts(buf+5); out_putchar('\n'); return; }
    if (my_strncmp(buf, "cdl ",         4) == 0) { cmd_cdl(buf+4);   return; }
    if (my_strncmp(buf, "cdir ",        5) == 0) { cmd_cdir(buf+5);  return; }
    if (my_strncmp(buf, "mkdir ",       6) == 0) { cmd_cdir(buf+6);  return; } /* compat alias */
    if (my_strncmp(buf, "cd ",          3) == 0) { cmd_cd(buf+3);    return; }
    if (my_strncmp(buf, "dump ",        5) == 0) { cmd_dump(buf+5);  return; }
    if (my_strncmp(buf, "write ",       6) == 0) { cmd_write(buf+6); return; }
    if (my_strncmp(buf, "mount ",       6) == 0) { cmd_mount(buf+6); return; }
    if (my_strncmp(buf, "diskread ",    9) == 0) { cmd_diskread(buf+9); return; }
    if (my_strncmp(buf, "panic ",       6) == 0) {
        if (g_services && g_services->kernel_panic_shell)
            g_services->kernel_panic_shell(buf+6);
        return;
    }

    /* gravitonctl — daemon manager stub */
    if (my_strcmp(buf, "gravitonctl") == 0 || my_strncmp(buf, "gravitonctl ", 12) == 0) {
        out_puts("gravitonctl: service/daemon manager - not yet implemented\n");
        return;
    }

    /* gravitonadm — security manager stub */
    if (my_strcmp(buf, "gravitonadm") == 0 || my_strncmp(buf, "gravitonadm ", 12) == 0) {
        out_puts("gravitonadm: security manager and audit - not yet implemented\n");
        return;
    }

    /* sys — system/kernel API caller stub */
    if (my_strcmp(buf, "sys") == 0 || my_strncmp(buf, "sys ", 4) == 0) {
        out_puts("sys: System/Service/Kernel/Daemon API caller - not yet implemented\n");
        return;
    }

    /* Bare directory navigation: `..`, `/path`, `path/` */
    {
        int is_cd = 0;
        const char *target = NULL;
        if (buf[0] == '/')                                { target = buf; is_cd = 1; }
        else if (my_strcmp(buf, "..") == 0)               { target = ".."; is_cd = 1; }
        else if (len > 0 && buf[len-1] == '/')            { target = buf; is_cd = 1; }
        if (is_cd && target) { cmd_cd(target); return; }
    }

    out_puts("rash: command not found: "); out_puts(buf); out_puts("\n");
    out_puts("      Type 'help' for available commands.\n");
}

/* -------------------------------------------------------------------------
 * initrc runner
 * ---------------------------------------------------------------------- */
static void run_initrc(const char *path) {
    if (!g_services || !g_services->vfs_open || !g_services->vfs_read || !g_services->vfs_close)
        return;
    int fd = g_services->vfs_open(path, 0);
    if (fd < 0) return;

    char line[128]; int line_pos = 0;
    uint8_t byte;
    while (g_services->vfs_read(fd, &byte, 1) == 1) {
        if (byte == '\n' || byte == '\r') {
            if (line_pos > 0) {
                line[line_pos] = '\0';
                execute_command(line);
                line_pos = 0;
            }
        } else if (line_pos < 127) {
            line[line_pos++] = (char)byte;
        }
    }
    if (line_pos > 0) {
        line[line_pos] = '\0';
        execute_command(line);
    }
    g_services->vfs_close(fd);
}

/* -------------------------------------------------------------------------
 * Entry point  (called by kernel ELF loader)
 * ---------------------------------------------------------------------- */
void main(kernel_services_t *services) {
    g_services = services;

    if (g_services && g_services->beep) {
        g_services->beep(50000000, 1000, 1);
    }

    out_puts("--- GRAVITON ---\n");
    out_puts("Rhoudveine OS - Graviton Init (PID 1)\n");

    /* 1. Run startup script */
    run_initrc("/System/Rhoudveine/Booter/initrc");

    /* 2. Hand off to rash — the interactive shell */
    out_puts("\nStarting rash (Rhoudveine Adaptive Shell)...\n");
    out_puts("Type 'help' for available commands.\n\n");

    const int BUF_SIZE = 128;
    char buf[BUF_SIZE];
    int pos = 0;

    for (;;) {
        /* Print prompt: "cwd rash> " */
        if (g_services && g_services->fb_cursor_hide) g_services->fb_cursor_hide();
        out_puts(g_cwd);
        out_puts(" rash> ");
        pos = 0;

        int blink_counter = 0, cursor_state = 0;
        if (g_services && g_services->fb_cursor_show) g_services->fb_cursor_show();

        /* Read one line */
        while (1) {
            int c = -1;
            if (g_services && g_services->try_getchar) c = g_services->try_getchar();

            if (c <= 0) {
                blink_counter++;
                if (blink_counter >= 50) {
                    blink_counter = 0;
                    cursor_state = !cursor_state;
                    if (cursor_state) { if (g_services && g_services->fb_cursor_show) g_services->fb_cursor_show(); }
                    else              { if (g_services && g_services->fb_cursor_hide) g_services->fb_cursor_hide(); }
                }
                for (volatile int z = 0; z < 20000; z++);
                continue;
            }

            if (c == '\r' || c == '\n') {
                if (g_services && g_services->fb_cursor_hide) g_services->fb_cursor_hide();
                out_putchar('\n');
                buf[pos] = '\0';
                break;
            }
            if (c == '\b' || c == 127) {
                if (pos > 0) {
                    pos--;
                    if (g_services && g_services->fb_cursor_hide) g_services->fb_cursor_hide();
                    if (g_services && g_services->fb_backspace) g_services->fb_backspace();
                    else out_putchar('\b');
                    if (g_services && g_services->fb_cursor_show) g_services->fb_cursor_show();
                }
                continue;
            }
            if (pos < BUF_SIZE - 1) {
                buf[pos++] = (char)c;
                if (g_services && g_services->fb_cursor_hide) g_services->fb_cursor_hide();
                out_putchar((char)c);
                if (g_services && g_services->fb_cursor_show) g_services->fb_cursor_show();
            }
        }

        if (pos > 0) execute_command(buf);
    }
}
