#include <stdio.h>
#include <sys/stat.h>
#include <H5ACpublic.h>
#include "recorder-optimization.h"

#define H5AC__CURR_CACHE_CONFIG_VERSION   1

int apply_optimizations(RecorderLogger* logger, Knowledge* knowledge, Record* record, const char * func_name, int timestep, char file_name[512]){

    if ((strcmp(func_name, "H5Fcreate") == 0)){
        // printf("Knowledge for rank %d, Operation %s, Transfer Size %d, Collective I/O %d, File Operation %s, File Name %s, File Size %lu, fapl_id %lu, dcpl_id %lu\n", 
        // logger->rank, knowledge->operation, knowledge->transfer_size, knowledge->collective, knowledge->file_operation, knowledge->file_name, knowledge->file_size, knowledge->fapl_ID, knowledge->dcpl_ID);

        if (strcmp(knowledge->operation, "write") == 0){
            if (knowledge->transfer_size < 16777216){
                if (logger->rank == 0)
                    printf("Changing alignment to 1MB\n");
                GOTCHA_REAL_CALL(H5Pset_alignment)(knowledge->fapl_ID, 1, 1000000);

                H5AC_cache_config_t temp;
                temp.version = 1;
                GOTCHA_REAL_CALL(H5Pget_mdc_config)(knowledge->fapl_ID, &temp);  
                temp.initial_size = 8000000;
                if (logger->rank == 0)
                    printf("Changing cache size to %d\n", temp.initial_size);
                GOTCHA_REAL_CALL(H5Pset_mdc_config)(knowledge->fapl_ID, &temp);
            }
            else {
                if (logger->rank == 0)
                    printf("Changing alignment to 16MB\n");
                GOTCHA_REAL_CALL(H5Pset_alignment)(knowledge->fapl_ID, 1, 1600000);

                H5AC_cache_config_t temp;
                temp.version = 1;
                GOTCHA_REAL_CALL(H5Pget_mdc_config)(knowledge->fapl_ID, &temp);  

                temp.initial_size = 16777216;
                if (logger->rank == 0)
                    printf("Changing cache size to %d\n", temp.initial_size);
                GOTCHA_REAL_CALL(H5Pset_mdc_config)(knowledge->fapl_ID, &temp);
            }
        }
    }
    else if ((strcmp(func_name, "H5open") == 0)){
        // char *temp_ID = (char*) record->args[5];
        // int ID = strtol(temp_ID, NULL, 10);
        // // knowledge->dcpl_ID = *(hid_t*) record->res;
        if ((strcmp(knowledge->file_operation, "shared_file") == 0) && knowledge->collective == 1 && knowledge->dcpl_ID != 0){ 
            if (logger->rank == 0)
                printf("Changed data transfer mode to independent\n");
            H5Pset_dxpl_mpio(knowledge->dcpl_ID, H5FD_MPIO_INDEPENDENT);
            knowledge->dcpl_ID = 0;    
        }
    }

    // else if ((strcmp(func_name, "MPI_File_open") == 0)){
    //     fileOpenCount += 1;
    //     printf("Count %d, rank %d\n", fileOpenCount, logger->rank);
    //     if (fileOpenCount == 1){
    //         MPI_Info romio_cb_config_list;
    //         MPI_Info_create(&romio_cb_config_list);
            
    //         MPI_Info_set(romio_cb_config_list, "cb_nodes", "4" );
    //         MPI_Info_set(romio_cb_config_list, "cb_config_list", "*:16" );
    //         printf("Handle %s, rank %d\n", record->args[4], logger->rank);
    //         // GOTCHA_REAL_CALL(MPI_Barrier) (MPI_COMM_WORLD);
    //         MPI_File_set_info((MPI_File)record->args[4], romio_cb_config_list);
    //         MPI_File_get_info((MPI_File)record->args[4], &romio_cb_config_list);
    //         char value[MPI_MAX_INFO_VAL];
    //         int flag = 1;
    //         MPI_Info_get( romio_cb_config_list, "cb_nodes", MPI_MAX_INFO_VAL, value, &flag);
    //         // printf("CB NODES %s, rank %d\n", value,  logger->rank);
    //         // MPI_Info_free( &romio_cb_config_list );
    //     }
    // }


}
