#include <stdint.h>
#include "prog.h"

static int sq_len(const char *s) { int i=0; while(s&&s[i]) i++; return i; }
static void sq_cpy(char *dst, const char *src, int max) {
    int i=0; while(src[i]&&i<max-1){dst[i]=src[i];i++;} dst[i]='\0';
}

void program_main(kernel_services_t *ks, int argc, const char **argv) {
    if (!ks || !ks->vfs_open || !ks->vfs_readdir || !ks->vfs_close) {
        p_puts(ks, "VFS not available\n"); return;
    }
    
    char path_buf[128];
    const char *path = NULL;
    
    if (argc > 1) {
        path = argv[1];
    } else {
        if (ks->vfs_getcwd) {
            ks->vfs_getcwd(path_buf, 128);
            path = path_buf;
        } else {
            path = "/";
        }
    }
    
    int fd = ks->vfs_open(path, 0);
    if (fd < 0) {
        p_puts(ks, "cdl: cannot open: "); p_puts(ks, path); p_puts(ks, "\n");
        return;
    }
    
    vfs_dirent_t e;
    int count = 0;
    while (ks->vfs_readdir(fd, &e) == 0) {
        p_puts(ks, "  "); p_puts(ks, e.name);
        if (e.type & 0x02) p_putchar(ks, '/');
        p_putchar(ks, '\n');
        count++;
    }
    if (!count) p_puts(ks, "  (empty)\n");
    ks->vfs_close(fd);
}
void main(kernel_services_t *ks) { program_main(ks, 0, (void*)0); }
