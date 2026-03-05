/* rash.c — Rhoudveine Adaptive Shell
 * Lives at: /System/Rhoudveine/Programs/rash/rash
 */

#include <stdint.h>
#include <stddef.h>
#include "kernel_services.h"
#include "prog.h"

static int sq_len(const char *s)          { int i=0; while(s&&s[i]) i++; return i; }
static int sq_cmp(const char *a, const char *b) {
    if(!a||!b) return (int)(a==b?0:a?1:-1);
    int i=0; while(a[i]&&b[i]){if(a[i]!=b[i])return(int)(a[i]-b[i]);i++;}
    return(int)(a[i]-b[i]);
}
static void sq_cpy(char *dst, const char *src, int max) {
    int i=0; while(src[i]&&i<max-1){dst[i]=src[i];i++;} dst[i]='\0';
}
static void sq_cat(char *dst, const char *src, int max) {
    int d = sq_len(dst); int i = 0;
    while(src[i] && d < max-1) dst[d++] = src[i++];
    dst[d] = '\0';
}

static kernel_services_t *g_ks = NULL;
static char g_cwd[128] = "/";

/* ── Path resolution ── */
static void resolve_path(char *dest, const char *cwd, const char *input) {
    char temp[128]; int t = 0;
    if (!input || !*input) { sq_cpy(dest, cwd, 128); return; }
    if (input[0] == '/') { temp[t++]='/'; temp[t]='\0'; input++; }
    else {
        sq_cpy(temp, cwd, 128); t = sq_len(temp);
        if (t==0||temp[t-1]!='/') { temp[t++]='/'; temp[t]='\0'; }
    }
    const char *p = input;
    while (*p) {
        while (*p=='/') p++;
        if (!*p) break;
        const char *e = p; while(*e&&*e!='/') e++;
        int sl = (int)(e-p);
        if (sl==1&&p[0]=='.') { /* skip */ }
        else if (sl==2&&p[0]=='.'&&p[1]=='.') {
            if (t>1) { t--; while(t>0&&temp[t-1]!='/') t--; temp[t]='\0'; }
        } else {
            if (t>1&&temp[t-1]!='/') temp[t++]='/';
            for(int k=0;k<sl&&t<127;k++) temp[t++]=p[k];
            temp[t]='\0';
        }
        p = e;
    }
    if (t>1&&temp[t-1]=='/') temp[--t]='\0';
    if (t==0) { temp[0]='/'; temp[1]='\0'; }
    sq_cpy(dest, temp, 128);
}

/* ── Check if path is a directory ── */
static int is_directory(const char *path) {
    if (!g_ks || !g_ks->vfs_open || !g_ks->vfs_readdir || !g_ks->vfs_close) return 0;
    if (sq_cmp(path, "/") == 0) return 1;

    p_puts(g_ks, "DEBUG: is_dir checking '"); p_puts(g_ks, path); p_puts(g_ks, "'\n");

    int fd = g_ks->vfs_open(path, 0);
    if (fd < 0) {
        p_puts(g_ks, "DEBUG: vfs_open failed\n");
        return 0;
    }

    vfs_dirent_t e;
    int res = g_ks->vfs_readdir(fd, &e);
    g_ks->vfs_close(fd);

    p_puts(g_ks, "DEBUG: readdir res=");
    { char b[12]; int v=res; int i=0; if(v<0){p_putchar(g_ks,'-');v=-v;} 
      if(v==0)b[i++]='0'; else while(v>0){b[i++]='0'+v%10;v/=10;}
      while(i>0)p_putchar(g_ks,b[--i]); }
    p_puts(g_ks, "\n");

    return (res == 0);
}

/* ── Navigation ── */
static void builtin_cd(const char *target) {
    if (!target||!*target) return;
    char abs[128]; resolve_path(abs, g_cwd, target);
    
    p_puts(g_ks, "DEBUG: builtin_cd to '"); p_puts(g_ks, abs); p_puts(g_ks, "'\n");

    if (is_directory(abs)) { 
        p_puts(g_ks, "DEBUG: cd SUCCESS\n");
        sq_cpy(g_cwd, abs, 128);
        if (g_ks && g_ks->vfs_chdir) g_ks->vfs_chdir(abs);
    } else {
        p_puts(g_ks, "cd: not a directory: "); p_puts(g_ks, target); p_puts(g_ks, "\n");
    }
}

/* ── Smart execute/dispatch ── */
static void execute(char *buf) {
    if (!buf||!*buf||buf[0]=='#') return;
    int len = sq_len(buf);
    while(len>0&&(buf[len-1]==' '||buf[len-1]=='\r')) buf[--len]='\0';
    if (!len) return;

    // Split buf into command name and arguments
    char *args[16]; int argc = 0;
    char *p = buf;
    while (*p && argc < 15) {
        while (*p == ' ') *p++ = '\0';
        if (!*p) break;
        args[argc++] = p;
        while (*p && *p != ' ') p++;
    }
    args[argc] = NULL;
    if (argc == 0) return;

    char *cmd = args[0];
    int cmd_len = sq_len(cmd);

    /* 1. Explicit Built-ins */
    if (sq_cmp(cmd, "cd") == 0) {
        if (argc > 1) builtin_cd(args[1]);
        else builtin_cd("/");
        return;
    }
    if (sq_cmp(cmd, "help") == 0) {
        p_puts(g_ks, "\n  rash - Rhoudveine Adaptive Shell\n");
        p_puts(g_ks, "  Smart Dispatch: type a path to CD or RUN.\n");
        p_puts(g_ks, "  Utilities live in /System/Rhoudveine/Programs/.\n\n");
        return;
    }

    /* 2. Path-based Dispatch (starts with /, ., or ends with /) */
    if (cmd[0] == '/' || cmd[0] == '.' || cmd[cmd_len-1] == '/') {
        char abs[128]; resolve_path(abs, g_cwd, cmd);
        
        if (cmd[cmd_len-1] == '/' || is_directory(abs)) {
            builtin_cd(abs);
            return;
        }

        if (g_ks && g_ks->kernel_exec) {
            int err = g_ks->kernel_exec(abs, argc, (const char**)args);
            if (err == 0) return;
            if (cmd[0] == '/' || cmd[0] == '.') {
                p_puts(g_ks, "rash: command failed: "); p_puts(g_ks, abs); p_puts(g_ks, "\n");
                return;
            }
        }
    }

    /* 3. Search in system program path */
    char prog_path[192];
    sq_cpy(prog_path, "/System/Rhoudveine/Programs/", 192);
    sq_cat(prog_path, cmd, 192);
    sq_cat(prog_path, "/", 192);
    sq_cat(prog_path, cmd, 192);
    
    if (g_ks && g_ks->kernel_exec && g_ks->kernel_exec(prog_path, argc, (const char**)args) == 0) return;

    /* 4. Fallback: try as a relative directory (navigation shortcut) */
    char rel_abs[128]; resolve_path(rel_abs, g_cwd, cmd);
    if (is_directory(rel_abs)) {
        builtin_cd(rel_abs);
        return;
    }

    p_puts(g_ks, "rash: command not found: "); p_puts(g_ks, cmd); p_putchar(g_ks, '\n');
}

/* ── Entry point ── */
void main(kernel_services_t *services, int argc, const char **argv) {
    (void)argc; (void)argv;
    g_ks = services;
    p_puts(g_ks,"rash: Rhoudveine Adaptive Shell\n");
    char buf[128]; int pos=0;
    for (;;) {
        if(g_ks&&g_ks->fb_cursor_hide) g_ks->fb_cursor_hide();
        p_puts(g_ks, g_cwd); p_puts(g_ks," rash> ");
        pos=0;
        if(g_ks&&g_ks->fb_cursor_show) g_ks->fb_cursor_show();

        while(1) {
            int c=-1;
            if(g_ks&&g_ks->try_getchar) c=g_ks->try_getchar();
            if(c<=0) { for(volatile int z=0;z<20000;z++); continue; }
            
            if(c=='\r'||c=='\n') { p_putchar(g_ks,'\n'); buf[pos]='\0'; break; }
            if(c=='\b'||c==127) {
                if(pos>0){ pos--;
                    if(g_ks&&g_ks->fb_backspace) g_ks->fb_backspace();
                    else p_putchar(g_ks,'\b');
                }
                continue;
            }
            if(pos<127){
                buf[pos++]=(char)c;
                p_putchar(g_ks,(char)c);
            }
        }
        if(pos>0) execute(buf);
    }
}
