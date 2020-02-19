#include "hdf5.h"
#include "mpi.h"

#define FILE "workfile.out"

int main() {

    MPI_Init(NULL, NULL);

    hid_t file_id, dataset_id, dataspace_id;
    hsize_t dims[2];
    herr_t status;

    file_id = H5Fcreate(FILE, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

    // Create data space for the dataset
    dims[0] = 4;
    dims[1] = 6;
    dataspace_id = H5Screate_simple(2, dims, NULL);

    // Create the dataset
    dataset_id = H5Dcreate2(file_id, "/dest", H5T_STD_I32BE, dataspace_id,
                            H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

    status = H5Dclose(dataset_id);
    status = H5Sclose(dataspace_id);
    status = H5Fclose(file_id);

    // Do it again
    file_id = H5Fopen(FILE, H5F_ACC_RDWR, H5P_DEFAULT);
    dataspace_id = H5Screate_simple(2, dims, NULL);
    dataset_id = H5Dcreate2(file_id, "/dest2", H5T_STD_I32BE, dataspace_id,
                            H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    status = H5Dclose(dataset_id);
    status = H5Sclose(dataspace_id);
    status = H5Fclose(file_id);

    MPI_Finalize();
    return 0;
}
