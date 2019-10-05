#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <mpi.h>


int main() {
    MPI_Init(NULL, NULL);

    struct stat buf;
    int res;

    res = stat("./workfile.out", &buf);

    res = lstat("./workfile.out", &buf);

    int fd = open("./workfile.out", O_RDONLY);
    res = fstat(fd, &buf);
    close(fd);

    MPI_Finalize();
    return 0;
}
