#include <stdint.h>
#include "prog.h"

void program_main(kernel_services_t *ks, int argc, const char **argv) {
    if (!ks || !ks->vfs_mount) { p_puts(ks, "VFS not available\n"); return; }
    if (argc < 4) { p_puts(ks, "Usage: mount <path> <type> <dev>\n"); return; }
    
    if (ks->vfs_mount(argv[1], argv[2], argv[3]) == 0) {
        p_puts(ks, "Mounted "); p_puts(ks, argv[2]);
        p_puts(ks, " at ");    p_puts(ks, argv[1]); p_puts(ks, "\n");
    } else {
        p_puts(ks, "mount: failed\n");
    }
}
void main(kernel_services_t *ks) { program_main(ks, 0, (void*)0); }
