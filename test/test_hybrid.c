// C program to illustrate use of fork() &
// exec() system call for process creation

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/wait.h>
#include <string.h>
#include <mpi.h>

int main(int argc, char* argv[]) {

    MPI_Init(&argc, &argv);

    pid_t pid;
    int ret = 1;
    int status;
    pid = fork();

    if (pid == -1){
        printf("can't fork, error occured\n");
        exit(EXIT_FAILURE);
    }
    else if (pid == 0){

        printf("child process, pid = %u\n",getpid());
        // Here It will return Parent of child Process means Parent process it self
        printf("parent of child process, pid = %u\n",getppid());

        // the argv list first argument should point to
        // filename associated with file being executed
        // the array pointer must be terminated by NULL
        // pointer
        /*
        char * argv_list[] = {"test_posix", NULL};
        execv("./test_posix",argv_list);
        exit(0);
        */

        char * argv_list[] = {"mpirun", "-np", "2", "./test_mpi", NULL};
        ret = execv("/usr/bin/mpirun", argv_list);
        printf("return from execv: %d, %s\n", ret, strerror(errno));
        exit(0);
    }
    else{
        // parent
        for(int i = 0; i < 10; i++) {
            MPI_Barrier(MPI_COMM_WORLD);
        }

        waitpid(pid, &status, 0);
        exit(0);
    }


    MPI_Finalize();

    return 0;
}

