#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <H5ACpublic.h>
#include "recorder-optimization.h"

#define H5AC__CURR_CACHE_CONFIG_VERSION   1

static bool romio = false;

int apply_optimizations(RecorderLogger* logger, Knowledge* knowledge, Record* record, const char * func_name, int timestep, char file_name[512], int romioOptimized, MPI_Info info){

    if ((strcmp(func_name, "H5Fcreate") == 0)){
        if (strcmp(knowledge->operation, "write") == 0 || strcmp(knowledge->operation, "read") == 0){
            if (knowledge->transfer_size < 16777216){
                hsize_t alignment;
                hsize_t threshold; 
                GOTCHA_REAL_CALL(H5Pget_alignment)(knowledge->fapl_ID, &threshold, &alignment);
                if (logger->rank == 0)
                    printf("* Changed alignment to 1MB\n");
                GOTCHA_REAL_CALL(H5Pset_alignment)(knowledge->fapl_ID, 1, 1000000);

                H5AC_cache_config_t temp;
                temp.version = 1;
                GOTCHA_REAL_CALL(H5Pget_mdc_config)(knowledge->fapl_ID, &temp);  
                temp.initial_size = 8000000;
                if (logger->rank == 0)
                    printf("* Changed cache size to %d\n", temp.initial_size);
                GOTCHA_REAL_CALL(H5Pset_mdc_config)(knowledge->fapl_ID, &temp);
            }
            else {
                hsize_t alignment;
                hsize_t threshold; 
                H5Pget_alignment(knowledge->fapl_ID, &threshold, &alignment);
                if (logger->rank == 0)
                    printf("* Changed alignment to 16MB\n");
                GOTCHA_REAL_CALL(H5Pset_alignment)(knowledge->fapl_ID, 1, 1600000);

                H5AC_cache_config_t temp;
                temp.version = 1;
                GOTCHA_REAL_CALL(H5Pget_mdc_config)(knowledge->fapl_ID, &temp);  
                temp.initial_size = 16777216;
                if (logger->rank == 0)
                    printf("* Changed cache size to %d\n", temp.initial_size);
                GOTCHA_REAL_CALL(H5Pset_mdc_config)(knowledge->fapl_ID, &temp);
            }
        }


        char file_path[PATH_MAX];

        GOTCHA_REAL_CALL(getcwd)(file_path, sizeof(file_path));
        strcat(file_path, "/"); 
        strcat(file_path, file_name); 

        char checkpoint = file_name[strlen(file_name) - 1];
        int digit_chk = checkpoint - '0';
        digit_chk = digit_chk + 1;
        checkpoint = digit_chk +'0';
        file_path[strlen(file_path) - 1] = checkpoint;
        char command[50];
        int error = 0;

        if (strcmp(knowledge->file_operation, "shared_file") == 0 && logger->rank == 0){

            int ret = system("lfs df > /dev/null 2>&1");  // Redirect both stdout and stderr to /dev/null

            if (ret == 0) {
                FILE *fp; 
                char buffer[128];
                char version[32];

                fp = popen("rpm -qi lustre-tests | grep Version", "r");
                if (fp == NULL) {
                    perror("popen failed");
                    return EXIT_FAILURE;
                }

                char *token;
                if (fgets(buffer, sizeof(buffer), fp) != NULL) {
                    token = strstr(buffer, "Version     :");
                    if (token) {
                        token += strlen("Version     :"); 
                        while (*token == ' ') token++;
                        token = strtok(token, "_");
                    }
                }
                pclose(fp);

                if (strcmp(token, "2.10") > 0){
                    sprintf(command, "lfs setstripe -E 256M -c 1 -E 4G -c 4 -E -1 -c -1 %s", file_path);
                }
                else{
                    sprintf(command, "lfs setstripe -c -1 %s", file_path);
                }
                
                system(command);
                printf("* Changed stripe count of %s to %s\n", file_path, command);
                } 
            else {
                printf("No Lustre file system detected.\n");
            }
        }
    }
    else if ((strcmp(func_name, "H5open") == 0)){
        if ((strcmp(knowledge->file_operation, "shared_file") == 0) && knowledge->collective == 1 && knowledge->spatial_locality == 1 && knowledge->dcpl_ID != 0){ 
            if (logger->rank == 0)
                printf("* Changed data transfer mode to independent\n");
            H5Pset_dxpl_mpio(knowledge->dcpl_ID, H5FD_MPIO_INDEPENDENT);
            knowledge->dcpl_ID = 0;    
        }
    } 
}
