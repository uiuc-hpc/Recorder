#include <stdio.h>
#include "mpi.h"

typedef enum { false, true } bool;

static int rank;

void testfunc1() {
}
void testfunc2() {
}
void testfunc3() {
}


int main(int argc, char *argv[]) {

    MPI_Init(&argc, &argv);

    int world_size;

    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);


    MPI_Comm comm = MPI_COMM_WORLD;
    //MPI_Comm_dup(MPI_COMM_WORLD, &comm);

    MPI_Datatype type = MPI_LONG_INT;
    MPI_Type_commit(&type);

    char processor_name[MPI_MAX_PROCESSOR_NAME];
    int name_len;
    MPI_Get_processor_name(processor_name, &name_len);

    MPI_Comm_set_errhandler(comm, MPI_ERRORS_RETURN);
    MPI_Comm_free(&comm);

    void *sbuf = processor_name;
    int scount = name_len;
    char rbuf[MPI_MAX_PROCESSOR_NAME*world_size];
    MPI_Alltoall(sbuf, scount, MPI_BYTE, rbuf, scount, MPI_BYTE, MPI_COMM_WORLD);

    // MPI Info related
    MPI_Info info;
    MPI_Info_create(&info);
    MPI_Info_set(info, "cb_nodes", "2");

    // IO-Realted MPI Calls
    MPI_File fh;
    MPI_Status status;
    MPI_Offset offset;
    int i, a[10];
    for (i=0;i<10;i++) a[i] = 5;

    int err = MPI_File_open(MPI_COMM_WORLD, "workfile.out", MPI_MODE_RDWR | MPI_MODE_CREATE, info, &fh);

    MPI_File_set_view(fh, 0, MPI_INT, MPI_INT, "native", info);

    MPI_File_set_atomicity(fh, 1);
    MPI_File_write_at(fh, 0, a, 10, MPI_INT, &status);
    MPI_File_seek(fh, 100, MPI_SEEK_SET);
    MPI_File_get_size(fh, &offset);

    MPI_File_close(&fh);

    MPI_Barrier(MPI_COMM_WORLD);

    int flag, idx;
    if(world_size > 1) {
        if (rank == 0) {
            MPI_Send(sbuf, scount, MPI_BYTE, 1, 0, MPI_COMM_WORLD);
            MPI_Send(sbuf, scount, MPI_BYTE, 1, 0, MPI_COMM_WORLD);
        } else if(rank == 1){
            MPI_Request req[2];
            int finished[2];
            MPI_Irecv(rbuf, scount, MPI_BYTE, 0, 0, MPI_COMM_WORLD, &req[0]);
            MPI_Irecv(rbuf, scount, MPI_BYTE, 0, 0, MPI_COMM_WORLD, &req[1]);
            MPI_Testsome(2, req, &idx, finished, MPI_STATUSES_IGNORE);
            MPI_Testany(2, req, &idx, &flag, MPI_STATUS_IGNORE);
            MPI_Testall(2, req, &flag, MPI_STATUSES_IGNORE);
        }
    }

    MPI_Request req;
    MPI_Bcast(a, 1, MPI_INT, 0, MPI_COMM_WORLD);

    //MPI_Ibcast(a, 1, MPI_INT, 0, MPI_COMM_WORLD, &req);
    //MPI_Wait(&req, MPI_STATUS_IGNORE);
    //MPI_Test(&req, &flag, MPI_STATUS_IGNORE);

    int recv, send;
    MPI_Reduce(&send, &recv, 1, MPI_INT, MPI_MAX, 2, MPI_COMM_WORLD);

    for(int i = 0; i < 10; i++) {
        MPI_Barrier(MPI_COMM_WORLD);
    }

    testfunc1();
    testfunc2();
    testfunc3();

    MPI_Finalize();
    return 0;
}
