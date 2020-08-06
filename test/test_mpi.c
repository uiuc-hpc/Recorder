#include <stdio.h>
#include "mpi.h"

typedef enum { false, true } bool;

static int rank;


int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);

    int world_size;

    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    MPI_Datatype type = MPI_LONG_INT;
    MPI_Type_commit(&type);

    char processor_name[MPI_MAX_PROCESSOR_NAME];
    int name_len;
    MPI_Get_processor_name(processor_name, &name_len);

    MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);

    void *sbuf = processor_name;
    int scount = name_len;
    char rbuf[MPI_MAX_PROCESSOR_NAME];
    MPI_Alltoall(sbuf, scount, MPI_BYTE, rbuf, scount, MPI_BYTE, MPI_COMM_WORLD);

    // MPI Info related
    MPI_Info info;
    MPI_Info_create(&info);
    MPI_Info_set(info, "cb_nodes", "2");

    /* IO-Realted MPI Calls */
    MPI_File fh;
    MPI_Status status;
    int i, a[10];
    for (i=0;i<10;i++) a[i] = 5;

    int err = MPI_File_open(MPI_COMM_WORLD, "workfile.out", MPI_MODE_RDWR | MPI_MODE_CREATE, info, &fh);

    MPI_File_set_view(fh, 0, MPI_INT, MPI_INT, "native", info);

    //MPI_File_set_atomicity(fh0, 1);
    MPI_File_write_at(fh, 0, a, 10, MPI_INT, &status);

    MPI_File_close(&fh);

    MPI_Barrier(MPI_COMM_WORLD);

    if(world_size > 1) {
        if (rank == 0) {
            MPI_Request req[2];
            MPI_Isend(sbuf, scount, MPI_BYTE, 1, 0, MPI_COMM_WORLD, &req[0]);
            MPI_Isend(sbuf, scount, MPI_BYTE, 1, 0, MPI_COMM_WORLD, &req[1]);
            MPI_Waitall(2, req, MPI_STATUSES_IGNORE);
        } else if(rank == 1){
            MPI_Recv(rbuf, scount, MPI_BYTE, 0, 0, MPI_COMM_WORLD, &status);
            MPI_Recv(rbuf, scount, MPI_BYTE, 0, 0, MPI_COMM_WORLD, &status);
        }
    }

    MPI_Finalize();

    return 0;
}
