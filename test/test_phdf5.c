#include "hdf5.h"
#include "mpi.h"

#define FILE "workfile.out"

int main(int argc, char *argv[]) {

    MPI_Init(&argc, &argv);

    hid_t plist_id = H5Pcreate(H5P_FILE_ACCESS);
    //H5Pset_fapl_mpio(plist_id, MPI_COMM_WORLD, MPI_INFO_NULL);
    H5Pset_fapl_split(plist_id, "-metadata.h5", H5P_DEFAULT, "-data.h5", H5P_DEFAULT);

    hid_t file_id, dataset_id, dataspace_id, attr_id;
    hsize_t dims[2] = {2, 3};
    herr_t status;
    int data[] = {100, 100, 100, 100, 100, 100};

    file_id = H5Fcreate(FILE, H5F_ACC_TRUNC, H5P_DEFAULT, plist_id);
    dataspace_id = H5Screate_simple(2, dims, NULL);

    dataset_id = H5Dcreate2(file_id, "/dest", H5T_STD_I32BE, dataspace_id,
                            H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    H5Fflush(dataset_id, H5F_SCOPE_GLOBAL);
    H5Dwrite(dataset_id, H5T_STD_I32BE, H5S_ALL, H5S_ALL, H5P_DEFAULT, data);
    H5Fflush(dataset_id, H5F_SCOPE_GLOBAL);


    attr_id = H5Acreate2(dataset_id, "min", H5T_STD_I32BE, dataspace_id, H5P_DEFAULT, H5P_DEFAULT);
    H5Awrite(attr_id, H5T_STD_I32BE, &data[0]);
    H5Fflush(attr_id, H5F_SCOPE_GLOBAL);

    H5Awrite(attr_id, H5T_STD_I32BE, &data[1]);
    H5Fflush(attr_id, H5F_SCOPE_GLOBAL);

    H5Awrite(attr_id, H5T_STD_I32BE, &data[1]);
    H5Fflush(attr_id, H5F_SCOPE_GLOBAL);


    H5Aclose(attr_id);
    H5Dclose(dataset_id);
    H5Sclose(dataspace_id);
    H5Pclose(plist_id);
    H5Fclose(file_id);

    MPI_Finalize();
    return 0;
}
