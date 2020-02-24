#include <stdio.h>
#include "mpi.h"

typedef enum { false, true } bool;

static int rank;

static void print(char *func_name, bool start) {
    if (rank == 0) {
        if (start == true)
            printf("\n%s START\n", func_name);
        else
            printf("%s SUCCESS\n", func_name);
    }
}


#define TEST_MPI_CALL(func, args)       \
    print(#func, true);                 \
    func args ;                         \
    print(#func, false);


int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);

    int world_size;

    TEST_MPI_CALL(MPI_Comm_size, (MPI_COMM_WORLD, &world_size));

    TEST_MPI_CALL(MPI_Comm_rank, (MPI_COMM_WORLD, &rank));

    MPI_Datatype type = MPI_LONG_INT;
    TEST_MPI_CALL(MPI_Type_commit, (&type));

    char processor_name[MPI_MAX_PROCESSOR_NAME];
    int name_len;
    TEST_MPI_CALL(MPI_Get_processor_name, (processor_name, &name_len));

    TEST_MPI_CALL(MPI_Comm_set_errhandler, (MPI_COMM_WORLD, MPI_ERRORS_RETURN))

    void *sbuf = processor_name;
    int scount = name_len;
    char rbuf[MPI_MAX_PROCESSOR_NAME];
    TEST_MPI_CALL(MPI_Alltoall, (sbuf, scount, MPI_BYTE, rbuf, scount, MPI_BYTE, MPI_COMM_WORLD));

    // MPI Info related
    MPI_Info info;
    TEST_MPI_CALL(MPI_Info_create, (&info));
    TEST_MPI_CALL(MPI_Info_set, (info, "cb_nodes", "2"));


    /* IO-Realted MPI Calls */
    MPI_File fh;
    MPI_Status status;
    int i, a[10];
    for ( i=0;i<10;i++) a[i] = 5;

    TEST_MPI_CALL(MPI_File_open, (MPI_COMM_WORLD, "workfile.out", MPI_MODE_RDWR | MPI_MODE_CREATE, info, &fh))

    TEST_MPI_CALL(MPI_File_set_view, (fh, 0, MPI_INT, MPI_INT, "native", info))

    //MPI_File_set_atomicity(fh0, 1);
    TEST_MPI_CALL(MPI_File_write_at, (fh, 0, a, 10, MPI_INT, &status))

    TEST_MPI_CALL(MPI_File_close, (&fh))

    TEST_MPI_CALL(MPI_Barrier, (MPI_COMM_WORLD))

    TEST_MPI_CALL(MPI_Finalize, ())

    return 0;
}
