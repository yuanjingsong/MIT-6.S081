#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define R 0
#define W 1

int main(int argc, char* argv[]) {
    int parent_fd[2], child_fd[2];
    pipe(parent_fd);
    pipe(child_fd);

    if (fork() == 0) {
        // Now it is in child pid
        close(parent_fd[W]);
        close(child_fd[R]);

        char buffer = ' ';
        read(parent_fd[R], &buffer, 1);
        printf("%d: received ping\n", getpid());
        write(child_fd[W], &buffer, 1);
        close(parent_fd[R]);
        close(child_fd[W]);
    }else {
        close(parent_fd[R]);
        close(child_fd[W]);

        char buffer = ' ';
        write(parent_fd[W], &buffer, 1);
        read(child_fd[R], &buffer, 1);
        printf("%d: received pong\n", getpid());
        close(child_fd[R]);
        close(parent_fd[W]);
        wait(0);
    }
    exit(0);
}