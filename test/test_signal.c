#include <stdio.h>
#include <unistd.h>
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

    int recv, send;
    MPI_Reduce(&send, &recv, 1, MPI_INT, MPI_MAX, 2, MPI_COMM_WORLD);

    for(int i = 0; i < 10; i++) {
        MPI_Barrier(MPI_COMM_WORLD);
    }

    testfunc1();
    testfunc2();
    testfunc3();

    // Sleep and wait for a kill()
    sleep(20);

    MPI_Finalize();
    return 0;
}
