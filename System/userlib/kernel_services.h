#ifndef KERNEL_SERVICES_H
#define KERNEL_SERVICES_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// Matches kernel's struct dirent
typedef struct {
    uint32_t inode;
    char name[256];
    uint8_t type;
} vfs_dirent_t;

typedef struct {
    void (*puts)(const char*);
    void (*putchar)(char);
    int (*getchar)(void);
    int (*try_getchar)(void);
    
    void (*fb_backspace)(void);
    void (*fb_cursor_show)(void);
    void (*fb_cursor_hide)(void);
    
    void (*beep)(double, double, bool);
    uint64_t (*timer_get_uptime_ms)(void);
    
    void (*kernel_panic_shell)(const char*);

    // VFS operations
    int (*vfs_open)(const char *path, uint32_t flags);
    int (*vfs_close)(int fd);
    int (*vfs_read)(int fd, void *buffer, size_t count);
    int (*vfs_write)(int fd, const void *buffer, size_t count);
    int (*vfs_readdir)(int fd, vfs_dirent_t *entry);
    int (*vfs_mkdir)(const char *path);
    int (*vfs_mount)(const char *path, const char *fstype, const char *device);
    int (*vfs_chdir)(const char *path);
    int (*vfs_getcwd)(char *buf, size_t size);

    // ACPI/AHCI operations
    void (*acpi_shutdown)(void);
    void (*acpi_reboot)(void);
    int (*ahci_read_sectors)(uint64_t lba, uint32_t count, uint8_t *buffer);
    int (*ahci_is_initialized)(void);

    // ACPI data fields (populated after acpi_init)
    int      acpi_cpu_count;       // Number of CPUs detected by MADT
    uint32_t acpi_local_apic_addr; // Local APIC base address

    // Process / exec
    // Load an ELF binary from the VFS at 'path' and call its entry point with
    // this services struct, argc and argv. Returns 0 on success, negative on error.
    int (*kernel_exec)(const char *path, int argc, const char **argv);
} kernel_services_t;

#endif
