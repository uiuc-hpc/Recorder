#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <mpi.h>


int main() {
    //MPI_Init(NULL, NULL);

    struct stat buf;
    int res;

    res = stat("./workfile.out", &buf);

    res = lstat("./workfile.out", &buf);

    int fd = open("./workfile.out", O_RDONLY);
    res = fstat(fd, &buf);

    char data[] = "hello world";
    write(fd, &data, sizeof(char)*5);
    res = fstat(fd, &buf);
    write(fd, &data, sizeof(char)*5);
    res = fstat(fd, &buf);
    write(fd, &data, sizeof(char)*7);
    close(fd);

    FILE* fp = fopen("./workfile.out", "w");
    fclose(fp);

    int a, b;
    a = 0;
    b = 10;
    fprintf(stderr, "hello world: %d %d", a, b);
    //MPI_Finalize();

    return 0;
}
