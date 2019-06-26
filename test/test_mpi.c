#include <mpi.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
    MPI_Init(NULL, NULL);

    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    char processor_name[MPI_MAX_PROCESSOR_NAME];
    int name_len;
    MPI_Get_processor_name(processor_name, &name_len);



    void *sbuf = processor_name;
    int scount = name_len;
    char rbuf[MPI_MAX_PROCESSOR_NAME];
    MPI_Alltoall(sbuf, scount, MPI_BYTE, rbuf, scount, MPI_BYTE, MPI_COMM_WORLD);

    printf("Hello world from processor %s, rank %d out of %d processors\n",
            rbuf, world_rank, world_size);

    MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);

    MPI_Finalize();

    return 0;
}
