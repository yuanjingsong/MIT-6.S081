#include "kernel/types.h"
#include "user.h"
#include "kernel/stat.h"

#define R 0
#define W 1

void redirect(int port, int pd[]) {
    close(port);
    dup(pd[port]);
    close(pd[R]);
    close(pd[W]);
}

void newProc() {
    int pd[2];
    int this_prime;
    int pass_num;
    if (read(0, &this_prime, sizeof(this_prime))) {
        printf("prime %d\n", this_prime);
        pipe(pd);
        if (fork()) {
            redirect(0, pd);
            newProc();
        }else {
            redirect(1, pd);
            while (read(0, &pass_num, sizeof(pass_num))) {
                if (pass_num % this_prime != 0) {
                    write(1, &pass_num, sizeof(pass_num));
                }
            }
        }
    }
}

int main(int argc, char *argv[])
{
    int pd[2];
    pipe(pd);

    if (fork())  {
        redirect(0, pd);
        newProc();
    }else {
        redirect(1, pd);
        for (int i = 2; i <= 35; i++) {
            write(1, &i, sizeof(i));
        }
    }
    exit(0);

    return 0;
}
