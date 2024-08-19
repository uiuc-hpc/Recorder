#include <stdio.h>
#include <H5ACpublic.h>
#include "recorder-optimization.h"

#define H5AC__CURR_CACHE_CONFIG_VERSION   1

int apply_optimizations(Knowledge* knowledge, hid_t dcplID, hid_t faplID, int nprocs){
    if (strcmp(knowledge->operation, "write") == 0){
        // RECORDER_LOGINFO("\nTransfer Size %d\n", knowledge->transfer_size);
        // RECORDER_LOGINFO("nprocs %d\n\n", nprocs);
        if (knowledge->transfer_size < 16777216){
            
            RECORDER_LOGINFO("Changing alignment to 1MB\n");
            //Set alignment to 1MB
            GOTCHA_REAL_CALL(H5Pset_alignment)(faplID, 1, 1000000);

            H5AC_cache_config_t temp;
            temp.version = 1;
            GOTCHA_REAL_CALL(H5Pget_mdc_config)(faplID, &temp);  

            //Set cache size to 8MB
            temp.initial_size = 8000000;
            RECORDER_LOGINFO("Changing cache size to %d\n", temp.initial_size);
            GOTCHA_REAL_CALL(H5Pset_mdc_config)(faplID, &temp);
            
            if(knowledge->collective == 1){
                RECORDER_LOGINFO("Changing data transfer to independent\n");
                GOTCHA_REAL_CALL(H5Pset_dxpl_mpio)(dcplID, H5FD_MPIO_INDEPENDENT);
            }  
            // else if (knowledge->collective == 0 && nprocs > 8){
            //     RECORDER_LOGINFO("Changing data transfer to collective\n");
            //     GOTCHA_REAL_CALL(H5Pset_dxpl_mpio)(dcplID, H5FD_MPIO_COLLECTIVE);  
            // }
            // GOTCHA_REAL_CALL(H5Pset_dxpl_mpio)(dcplID, H5FD_MPIO_INDEPENDENT);
        }
        else {
            //Set alignment to 16MB
            RECORDER_LOGINFO("Changing alignment to 16MB\n");
            GOTCHA_REAL_CALL(H5Pset_alignment)(faplID, 1, 1600000);

            H5AC_cache_config_t temp;
            temp.version = 1;
            GOTCHA_REAL_CALL(H5Pget_mdc_config)(faplID, &temp);  

            //Set cache size to 16MB
            temp.initial_size = 16777216;
            RECORDER_LOGINFO("Changing cache size to %d\n", temp.initial_size);
            GOTCHA_REAL_CALL(H5Pset_mdc_config)(faplID, &temp);

            if(knowledge->collective == 1){
                RECORDER_LOGINFO("Changing data transfer to independent\n");
                GOTCHA_REAL_CALL(H5Pset_dxpl_mpio)(dcplID, H5FD_MPIO_INDEPENDENT);  
            }
            // else if (knowledge->collective == 0 && nprocs > 8){
            //     RECORDER_LOGINFO("Changing data transfer to collective\n");
            //     GOTCHA_REAL_CALL(H5Pset_dxpl_mpio)(dcplID, H5FD_MPIO_COLLECTIVE);  
            // }
            // GOTCHA_REAL_CALL(H5Pset_dxpl_mpio)(dcplID, H5FD_MPIO_INDEPENDENT);
        }
    }
}