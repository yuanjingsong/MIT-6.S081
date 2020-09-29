#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

char* getFileName(char* path) {
    char *t = path;
    char *last = 0;
    while (*t != '\0') {
        if (*t == '/') {
            last = t;
        }
        t ++;
    }
    if (last == 0) {
        return path;
    }else {
        return last + 1;
    }
}

void find(char* root_path, char* target){
    int fd;
    char buf[512], *p;

    struct stat st;
    struct dirent de;

    if ((fd = open(root_path, 0)) < 0) {
        fprintf(2, "Cannot access %s\n", root_path);
        return ;
    }

    if (fstat(fd, &st) < 0) {
        fprintf(2, "Can not stat %s\n", root_path);
        close(fd);
        return ;
    }

    switch (st.type)
    {
    case T_FILE:
        p = getFileName(root_path);
        if (strcmp(p, target) == 0) {
            printf("%s\n", root_path);
        }
        break;

    case T_DIR:
        if(strlen(root_path) + 1 + DIRSIZ + 1 > sizeof buf) {
            printf("Path too long \n");
            break;
        }
        strcpy(buf, root_path);
        p = buf+strlen(buf);
        *p++ = '/';
        while (read(fd, &de, sizeof(de)) == sizeof(de)) {
            if (de.inum == 0)
                continue;

            int t = strlen(de.name) > DIRSIZ ? DIRSIZ : strlen(de.name);
            memmove(p, de.name, t);
            p[t] = 0;
            if (stat(buf, &st) < 0) {
                printf("find: cannot stat %s \n", buf);
                continue;
            }

            if (strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0){
                continue;
            }

            find(buf, target);
        }
        break;
    
    default:
        break;
    }
    close(fd);

}

int main(int argc, char* argv[]) {
    if (argc <= 2) {
        fprintf(2, "find: root path, target name\n");
        exit(1);
    }
    find(argv[1], argv[2]);
    exit(0);
}