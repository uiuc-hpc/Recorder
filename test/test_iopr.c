#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <mpi.h>


int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int fd = open("./workfile.out", O_CREAT|O_WRONLY, 0644);
    printf("fd: %d\n", fd);

    off_t offset = 0;
    char data[] = "hello world";

    for(int i = 0; i < 10; i++) {
        lseek(fd, i*5, SEEK_SET);
        pwrite(fd, data, 5, offset);
        offset += 5;
    }

    close(fd);

    MPI_Finalize();
    return 0;
}
