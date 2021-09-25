#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>


int main() {

    struct stat buf;
    int res;

    res = stat("./workfile.out", &buf);

    res = lstat("./workfile.out", &buf);

    int fd = open("./workfile.out", O_RDONLY);
    res = fstat(fd, &buf);

    struct flock lk;
    fcntl(fd, F_GETLK, &lk);

    char data[] = "hello world";
    write(fd, &data, sizeof(char)*5);
    res = fstat(fd, &buf);
    write(fd, &data, sizeof(char)*5);
    res = fstat(fd, &buf);
    write(fd, &data, sizeof(char)*7);
    close(fd);

    FILE* fp = fopen("./workfile.out", "w");
    fseeko(fp, 10, SEEK_CUR);
    ftello(fp);
    fclose(fp);

    int a, b;
    a = 0;
    b = 10;
    fprintf(stderr, "hello world: %d %d\n", a, b);


    DIR* dir = opendir("/sys");
    struct dirent *dent = readdir(dir);
    do {
        printf("dent %s\n", dent->d_name);
        dent = readdir(dir);
    } while(dent);
    closedir(dir);


    return 0;
}
