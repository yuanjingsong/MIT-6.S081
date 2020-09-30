#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/param.h"
#include "user/user.h"

#define MAX_BUF 512

int main(int argc, char  *argv[])
{
    if (argc < 2) {
        exit(1);
    }

    char* cmd = argv[1];
    char buf[MAX_BUF];
    char *params[MAXARG];
    int idx = 0;

    params[idx++] = cmd;
    for (int i = 2; i < argc; i++) {
        params[idx++] = argv[i];
    }

    int n = 0;
    while ((n = read(0, buf, MAX_BUF)) > 0) {
        if (fork()) {
            //parent 
            wait(0);
        }else {
            //child 
            char *t = (char*) malloc(sizeof(char) * MAX_BUF);
            int c = 0;
            for (int i = 0; i < n; i++) {
                if (buf[i] == '\n' || buf[i] == ' ') {
                    t[c] = '\0';
                    params[idx++] = t;
                    c = 0;
                    t = (char*) malloc(sizeof(char) * MAX_BUF);
                }else {
                    t[c++] = buf[i];
                }
            }
            t[c] = '\0';
            params[idx] = 0;
            exec(cmd, params);
        }
    }
    exit(0);

    return 0;
}
