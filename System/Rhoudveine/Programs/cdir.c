#include <stdint.h>
#include "prog.h"

static int sq_len(const char *s) { int i=0; while(s&&s[i]) i++; return i; }
static void sq_cpy(char *dst, const char *src, int max) {
    int i=0; while(src[i]&&i<max-1){dst[i]=src[i];i++;} dst[i]='\0';
}
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

void program_main(kernel_services_t *ks, int argc, const char **argv) {
    if (argc < 2) { p_puts(ks, "Usage: cdir <path>\n"); return; }
    if (!ks || !ks->vfs_mkdir) { p_puts(ks, "VFS not available\n"); return; }
    
    /* Assuming argv[0] is program name, argv[1] is path */
    char abs[128];
    /* For now, utilities don't know CWD easily unless passed in.
       We'll assume the shell passes absolute paths or we'll need to pass CWD in argv.
       Let's assume the shell resolves for us for now or we use a fallback. */
    if (argv[1][0] == '/') {
        sq_cpy(abs, argv[1], 128);
    } else {
        /* This is a limitation of not having a 'stat' or 'getcwd' syscall yet.
           We'll just try relative to root for now if it's not absolute. */
        sq_cpy(abs, "/", 128);
        sq_cpy(abs + 1, argv[1], 127);
    }

    if (ks->vfs_mkdir(abs) != 0) {
        p_puts(ks, "cdir: failed to create '"); p_puts(ks, abs); p_puts(ks, "'\n");
    }
}
void main(kernel_services_t *ks) { 
    /* This main is called by kernel_exec. We need to parse command line.
       For now, let's keep it simple. */
    program_main(ks, 0, (void*)0); 
}
