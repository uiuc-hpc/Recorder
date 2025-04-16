#include <stdio.h>
#include <H5ACpublic.h>
#include "recorder-analysis.h"
#include "recorder-optimization.h"
#include "mpi.h"


#define SIZE 100
#define THRESHOLD 0  // For IOR, set threshold to 5. For FLASH, set threshold to 2

#define H5AC__CURR_CACHE_CONFIG_VERSION   1

static RuleConfidence* cfg_conf = NULL;
static Knowledge* rank_knowledge = NULL;

static int top = -1;
static int timestep = -1;
static int romioOptimized = 0;
static int prediction_timesteps[10];
static int prediction_timesteps_index = 0;
static int optimization_timestep = 0;

static char file_name[512];
static char _faplID[512];
static off_t *prev_offset_write;
static off_t *prev_offset_read;
static int locality_threshold_read = 0;
static int locality_threshold_write = 0;

void push(int x, int stack[])
{
    if (top != SIZE - 1)
    {
        top = top + 1;
        stack[top] = x;
    }
}

void pop(int stack[])
{
    if (top != -1)
    {
        printf("\nPopped element: %d", stack[top]);
        top = top - 1;
    }
}

void show(int stack[])
{
    if (top != -1)
    {
        printf("\nElements present in the stack: \n");
        for (int i = top; i >= 0; --i){
            printf(" element %d - %d\n", i, stack[i]);
        }      
    }
}

int custom_cmp(Symbol* matched_rule, int val){
    if (matched_rule->val == val){
        return 0;
    }
    return 1;
}

void extract_io_behavior(RecorderLogger* logger, int stack[]){
    int tmp_stack[SIZE];
    int tmp_top = top;

    for (int i = 0; i <= top; i++) {
        tmp_stack[i] = stack[i];
    }

    int terminal_id = 0; 

    Knowledge *entry = NULL;
    HASH_FIND(hh, rank_knowledge, &timestep, sizeof(int), entry);

    if (entry){
        for (int i = 0; i <= tmp_top; i++){
            terminal_id = tmp_stack[i];
            
            CallSignature *s;
            
            for (s = logger->cst; s != NULL; s = (CallSignature *)(s->hh.next)) {
                if (s->terminal_id == terminal_id){
                    Record * record = cs_to_record(s);
                    const char * func_name = get_function_name_by_id(record->func_id);
                    if (strcmp(func_name, "MPI_File_write_at_all") == 0 || strcmp(func_name, "MPI_File_read_at_all") == 0){
                        entry->collective = 1;
                        if (strcmp(func_name, "MPI_File_write_at_all") == 0)
                            sprintf(entry->operation, "write");
                        else if (strcmp(func_name, "MPI_File_read_at_all") == 0)
                            sprintf(entry->operation, "read");
                    }
                    else if (strcmp(func_name, "MPI_File_write_at") == 0 || strcmp(func_name, "MPI_File_read_at") == 0){
                        entry->collective = 0;
                        if (strcmp(func_name, "MPI_File_write_at") == 0)
                            sprintf(entry->operation, "write");
                        else if (strcmp(func_name, "MPI_File_read_at") == 0)
                            sprintf(entry->operation, "read");
                        char *transfer_size = (char*) record->args[3];
                        entry->transfer_size = atoi(transfer_size);
                    }

                    if (strcmp(func_name, "write") == 0){
                        char *transfer_size = (char*) record->args[2];
                        entry->transfer_size = atoi(transfer_size);
                        sprintf(entry->operation, "write");

                        off_t *offset = (off_t*) record->args[3];
                        if (offset == prev_offset_write){

                            locality_threshold_write = locality_threshold_write + 1;
    
                            if (locality_threshold_write > 2)
                                entry->spatial_locality = 1;
                        }
                        prev_offset_write = offset;
                    }
                    else if (strcmp(func_name, "read") == 0){
                        char *transfer_size = (char*) record->args[2];
                        entry->transfer_size = atoi(transfer_size);
                        sprintf(entry->operation, "read");

                        off_t *offset = (off_t*) record->args[3];
                        if (offset == prev_offset_read){
                            locality_threshold_read = locality_threshold_read + 1;
                            if (locality_threshold_read > 1)
                                entry->spatial_locality = 1;
                        }
                        prev_offset_read = offset;
                    } 
                }  

            } 
        }     
    } 
}

void find_rule_confidence(RecorderLogger* logger, CallSignature *entry){
    Symbol *rule, *sym, *firstRule;
    int rules_count = 0;
    DL_COUNT(logger->cfg.rules, rule, rules_count);

    bool first = true;
    bool found = false;

    if (rules_count > 1){
        DL_FOREACH(logger->cfg.rules, rule) {
            if (first){
                firstRule = rule;
                first = false;
            }
            else {
                int count = 0;
                DL_FOREACH(firstRule->rule_body, sym) {
                    if (sym->val == rule->val){
                        if (sym->exp > 0){
                            count += sym->exp;
                        }
                        else{
                            count += 1;
                        }
                    }
                }

                RuleConfidence *entry = NULL;
                HASH_FIND(hh, cfg_conf, &rule->val, sizeof(int), entry);
                if(entry) {    
                    entry->count = count;
                } else {                          
                    entry = (RuleConfidence*) recorder_malloc(sizeof(RuleConfidence));
                    entry->rule = rule->val;
                    entry->count = count; 
                    entry->probability = 0;
                    HASH_ADD(hh, cfg_conf, rule, sizeof(int), entry);
                }
            }
        }

        RuleConfidence *s;
        for (s = cfg_conf; s != NULL; s = (RuleConfidence *)(s->hh.next)) {
            found = false;
            DL_FOREACH(logger->cfg.rules, rule) {
                if (s->rule == rule->val){
                    found = true;
                    break;
                }
            }
            if (!found){
                RuleConfidence *rs, *tmp;
                HASH_ITER(hh, cfg_conf, rs, tmp) {
                    if (rs->rule == s->rule){
                        HASH_DEL(cfg_conf, rs);
                    }
                }
            }
        }
    }
}

void find_pattern_within(RecorderLogger* logger, Symbol* rule, int stack[]){
    Symbol *sym;
    DL_FOREACH(rule->rule_body, sym) {
        if (sym->val < 0) {
            Symbol *matched_rule;
            DL_SEARCH(logger->cfg.rules, matched_rule, sym->val, custom_cmp);
            if (matched_rule->exp > 0){
                for (int i = 0; i < matched_rule->exp; i++){
                    find_pattern_within(logger, matched_rule, stack);
                }
            }
            else {
                find_pattern_within(logger, matched_rule, stack);
            }
        } else
            push(sym->val, stack);
    }
}

int find_pattern_start(RecorderLogger* logger, int search_value, bool initial, int stack[]){
    Symbol *rule, *sym;
    int target_rule = 0, rules_count = 0;
    DL_COUNT(logger->cfg.rules, rule, rules_count);

    if (initial && rules_count > 1){
        bool first = true;
        DL_FOREACH(logger->cfg.rules, rule) {
            if (!first) {
                DL_FOREACH(rule->rule_body, sym) {
                    if (sym->val == search_value){
                        target_rule = rule->val;     
                        return target_rule;
                    }
                    else {
                        break;
                    }  
                }
            }
            first = false;
        }  
    }
    else if (!initial){
        DL_FOREACH(logger->cfg.rules, rule) {
            if (rule->val == search_value) {
                DL_FOREACH(rule->rule_body, sym) {
                    if (sym->val < 0) {
                        Symbol *matched_rule;
                        DL_SEARCH(logger->cfg.rules, matched_rule, sym->val, custom_cmp);
                        if (matched_rule){
                            if (matched_rule->exp > 0){
                                for (int i = 0; i < matched_rule->exp; i++){
                                    find_pattern_within(logger, matched_rule, stack);
                                }
                            }
                            else {
                                find_pattern_within(logger, matched_rule, stack);
                            }
                        }   
                    }
                    else {
                        push(sym->val, stack);
                    }  
                }
            }
        }
    }
    return 0;
}

void analysis_cs(RecorderLogger* logger, CallSignature *entry){
    int stack[SIZE];
    top = -1;

    int target_rule = find_pattern_start(logger, entry->terminal_id, true, stack);
    if (target_rule < -1){
        find_rule_confidence(logger, entry);
        int max_rule = target_rule;
        int max_rule_count = 0;

        RuleConfidence *s = NULL;
        HASH_FIND_INT(cfg_conf, &target_rule, s);
        if (s){
            max_rule_count = s->count;
        }

        Symbol *rule, *sym;
        bool first = true;

        DL_FOREACH(logger->cfg.rules, rule) {
            if (!first) {
                DL_FOREACH(rule->rule_body, sym) {
                    if (sym->val == target_rule){
                        RuleConfidence *s = NULL;
                        HASH_FIND_INT(cfg_conf, &rule->val, s);
                        if (s){
                            if (s->count > max_rule_count){
                                max_rule = s->rule;
                                max_rule_count = s->count;
                            }
                        }
                    }
                    else {
                        break;
                    }    
                }
            }
            first = false;
        }

        if (max_rule_count >= THRESHOLD){
            find_pattern_start(logger, max_rule, false, stack);
            extract_io_behavior(logger, stack);
        }
    }
}

int recorder_analysis(RecorderLogger* logger, Record* record, CallSignature* entry, bool* do_prediction, MPI_Info info, double *prediction_time, double *optimization_time) {
    const char * func_name = get_function_name_by_id(record->func_id);
    if (entry == NULL && info != MPI_INFO_NULL){
        clock_t begin = clock();
        Knowledge *entry = NULL;
        HASH_FIND(hh, rank_knowledge, &optimization_timestep, sizeof(int), entry);
        
        if (entry->spatial_locality == 1 && strcmp(entry->file_operation, "shared_file") == 0){
            if (logger->rank == 0){
                printf("* Changed romio_cb_write to disable\n");
            }
            MPI_Info_set(info, "romio_cb_write", "disable");
        }
        if ((entry->collective==1 && entry->spatial_locality == 0) || (entry->collective==1 && strcmp(entry->file_operation, "file-per-process") == 0)){
            if (logger->rank == 0){
                printf("* Changed cb_config_list to *:8\n");
            } 
            MPI_Info_set(info, "cb_config_list", "*:8"); 
        }
        clock_t end = clock();
        double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
        *optimization_time = *optimization_time + time_spent;
    } else {
        if ((strcmp(func_name, "H5Fcreate") == 0)){
            char *tmp_file_name = (char*) record->args[0];
            strncpy(file_name, tmp_file_name, sizeof(file_name)-1);
            file_name[sizeof(file_name)-1] = '\0';

            char * tmp_faplID = (char*) record->args[3];
            strncpy(_faplID, tmp_faplID, sizeof(_faplID)-1);
            _faplID[sizeof(_faplID)-1] = '\0';

            
            Knowledge *entry = NULL;
            for (int i = 0; i < prediction_timesteps_index; i++){
                int index = prediction_timesteps[i];
                HASH_FIND(hh, rank_knowledge, &index, sizeof(int), entry);
                if (entry){
                    if (strncmp(entry->file_name, file_name, strlen(entry->file_name) - 2) == 0){
                        *do_prediction = false;
                        optimization_timestep = prediction_timesteps[i];

                        char operation[100];
                        if (logger->rank == 0){
                            sprintf(operation, entry->operation);
                        }

                        GOTCHA_REAL_CALL(MPI_Bcast) (&operation, sizeof(operation), MPI_CHAR, 0, MPI_COMM_WORLD);
                        sprintf(entry->operation, operation);

                        int* recvcounts = NULL;
                        if (logger->rank == 0){
                            recvcounts = (int *) recorder_malloc(logger->nprocs*sizeof(int)); 
                        }

                        GOTCHA_REAL_CALL(MPI_Gather)(&entry->collective,1,MPI_INT,recvcounts,1,MPI_INT,0,MPI_COMM_WORLD);

                        int transfer_mode = 0;
                        if (logger->rank == 0){
                            int collective_count = 0;
                            int independent_count = 0;
                            for (int i = 0; i < logger->nprocs; i++){
                                if (recvcounts[i] == 0)
                                    independent_count += 1;
                                else if (recvcounts[i] == 1)
                                    collective_count += 1;
                            }
                            if (collective_count > independent_count){
                                transfer_mode = 1;
                            }
                            else {
                                transfer_mode = 0;
                            }
                            entry->collective = transfer_mode;   
                        }

                        GOTCHA_REAL_CALL(MPI_Bcast) (&transfer_mode, sizeof(transfer_mode), MPI_INT, 0, MPI_COMM_WORLD);
                        entry->collective = transfer_mode; 

                        if (entry->collective == 1){ 
                            int sequential_locality = 0;
                            if(logger->rank == 0){
                                for (int i = 0; i < logger->nprocs; i++){
                                    if (recvcounts[i] == 1){
                                        sequential_locality += 1;
                                        break;
                                    }
                                }
                                if (sequential_locality > 0)
                                    entry->spatial_locality = sequential_locality;   
                            }
                            GOTCHA_REAL_CALL(MPI_Bcast) (&sequential_locality, sizeof(sequential_locality), MPI_INT, 0, MPI_COMM_WORLD);
                            entry->spatial_locality = sequential_locality; 
                        } else {
                            int sequential_locality = 0;
                            if (logger->rank == 0){
                                int non_sequential_locality_count = 0;
                                int sequential_locality_count = 0;
                                for (int i = 0; i < logger->nprocs; i++){
                                    if (recvcounts[i] == 0)
                                        non_sequential_locality_count += 1;
                                    else if (recvcounts[i] == 1)
                                        sequential_locality_count += 1;
                                }
                                if (sequential_locality_count > non_sequential_locality_count){
                                    sequential_locality = 1;
                                }
                                else {
                                    sequential_locality = 0;
                                }
                                entry->spatial_locality = sequential_locality;   
                            }

                            GOTCHA_REAL_CALL(MPI_Bcast) (&sequential_locality, sizeof(sequential_locality), MPI_INT, 0, MPI_COMM_WORLD);
                            entry->spatial_locality = transfer_mode; 
                        }
                        break;
                    }
                    else {
                        *do_prediction = true;
                    }
                }
            }
            
            timestep = timestep + 1;
            if (*do_prediction){ 
                
                Knowledge *new_entry = NULL;
                new_entry = (Knowledge*) recorder_malloc(sizeof(Knowledge));
                new_entry->timestep = timestep;
                new_entry->file_size = -1;
                new_entry->collective = -1;
                sprintf(new_entry->file_name, file_name);
                
                
                int* recvcounts = NULL;
                if (logger->rank == 0){
                    recvcounts = (int *) recorder_malloc(logger->nprocs*sizeof(int)); 
                }
                int mylen = strlen(new_entry->file_name); 

                GOTCHA_REAL_CALL(MPI_Gather)(&mylen,1,MPI_INT,recvcounts,1,MPI_INT,0,MPI_COMM_WORLD);
                
                int totlen = 0; 
                int* displs = NULL;
                char* totalstring = NULL;

                if(logger->rank == 0)
                {   
                    
                    displs = (int *) recorder_malloc(logger->nprocs*sizeof(int));
                    displs[0] = 0; 
                    totlen += recvcounts[0] + 1; 
                    for(int i=1; i < logger->nprocs; i++)
                    {
                        totlen += recvcounts[i]+1; 
                        displs[i] = displs[i-1] + recvcounts[i-1] + 1; 
                    }
                    totalstring = (char *) recorder_malloc(totlen*sizeof(char)); 
                    for (int i = 0; i < totlen - 1; i++) {
                        totalstring[i] = ' '; 
                        totalstring[totlen - 1] = '\0'; 
                    }
                }
                
                
                GOTCHA_REAL_CALL(MPI_Gatherv)(&new_entry->file_name, mylen, MPI_CHAR,
                            totalstring, recvcounts, displs, MPI_CHAR, 0, MPI_COMM_WORLD);
                int shared_file = 1;
                char file_operation[100];
                long int file_size = 0;
                
                if (logger->rank == 0){
                    char * token = strtok(totalstring, " ");
                    char * first_token = token;
                    while( token != NULL ) {
                        token = strtok(NULL, " ");
                        if (token != NULL){
                            if (strcmp(first_token, token) != 0){
                                shared_file = 0;
                                break;
                            }
                        } 
                    }

                    free(totalstring);
                    free(displs);
                    free(recvcounts);
                }

                GOTCHA_REAL_CALL(MPI_Bcast) (&shared_file, sizeof(shared_file), MPI_INT, 0, MPI_COMM_WORLD);

                if (shared_file == 0){
                    sprintf(new_entry->file_operation, "file-per-process");
                } else {
                    sprintf(new_entry->file_operation, "shared_file");
                }
            
                HASH_ADD(hh, rank_knowledge, timestep, sizeof(int), new_entry);

                prediction_timesteps[prediction_timesteps_index] = timestep;
                prediction_timesteps_index = prediction_timesteps_index + 1;

            } else {
                entry->fapl_ID = strtol(_faplID, NULL, 10);
                entry->dcpl_ID = 0;               
            }
        }

        if (*do_prediction && timestep != -1){
            clock_t begin = clock();
            analysis_cs(logger, entry); 
            clock_t end = clock();
            if (timestep == 0){
                double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
                *prediction_time = *prediction_time + time_spent;
            }
        }
        else if (*do_prediction == false){
            clock_t begin = clock();
            Knowledge *entry = NULL;
            HASH_FIND(hh, rank_knowledge, &optimization_timestep, sizeof(int), entry);

            if ((strcmp(func_name, "H5Pcreate") == 0)){
                char *_h5p = (char*) record->args[0];
                hid_t h5p = strtol(_h5p, NULL, 10);
                
                char *temp_ID = (char*) record->args[0];
                int ID = strtol(temp_ID, NULL, 10);
                if (ID == 11){
                    entry->dcpl_ID = *(hid_t*) record->res;
                }
            }
            clock_t end = clock();
            double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
            *prediction_time = *prediction_time + time_spent;

            begin = clock();
            apply_optimizations(logger, entry, record, func_name, timestep, file_name, romioOptimized, info);
            end = clock();
            time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
            *optimization_time = *optimization_time + time_spent;
        }
    }
    return 0;
}        