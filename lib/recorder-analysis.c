#include "recorder-analysis.h"
#include "mpi.h"

#define SIZE 100
#define THRESHOLD 4

static RuleConfidence* cfg_conf = NULL;
static int top = -1;
char pattern[512];

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
        for (int i = top; i >= 0; --i)
            printf("%d\n", stack[i]);
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
    for (int i = tmp_top; i >= 0; --i){
        terminal_id = tmp_stack[i];
        tmp_top = tmp_top - 1;
        CallSignature *s;
        for (s = logger->cst; s != NULL; s = (CallSignature *)(s->hh.next)) {
            if (s->terminal_id == terminal_id){
                Record * record = cs_to_record(s);
                const char * func_name = get_function_name_by_id(record->func_id);
                if (strcmp(func_name, "MPI_File_write") == 0){
                    sprintf(pattern, "%s",  "MPI_File_write");
                    // logger->pattern = "write";
                    printf("Pattern %s \n", pattern);
                    // char file_name[50];
                    // sprintf(file_name, ".%d.io-pattern.txt", logger->rank);
                    // FILE *fptr;
                    // fptr = fopen(file_name, "a");
                    // if (fptr) {
                    //     fprintf(fptr, "write");         
                    //     fclose(fptr);
                    // }
                    // printf("Key %p, Terminal %d\n", s->key, s->terminal_id);
                }

                printf("Function Name %s\n", func_name);

                // printf("Args Count: %d\n", record->arg_count);

                // for (int i = 0; i < record->arg_count; i++){
                //     printf(record->args[i]);      
                //     printf("\n");
                // }
            }  
        } 
    }
    printf("\n\n\n");
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

        // for (s = logger->cfg_conf; s != NULL; s = (RuleConfidence *)(s->hh.next)) {
        //     printf("Rule %d, Count %d, Probability %d\n", s->rule, s->count, s->probability);  
        // }
    }
}

void find_pattern_within(RecorderLogger* logger, Symbol* rule, int stack[]){
    Symbol *sym;
    DL_FOREACH(rule->rule_body, sym) {
        if (sym->val < 0) {
            Symbol *matched_rule;
            DL_SEARCH(logger->cfg.rules, matched_rule, sym->val, custom_cmp);
            if (matched_rule){
                find_pattern_within(logger, matched_rule, stack);
            }
        }
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
                            find_pattern_within(logger, matched_rule, stack);
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
    Symbol *rule1, *sym1;
    int rules_count1 = 0, symbols_count1 = 0;
    DL_COUNT(logger->cfg.rules, rule1, rules_count1);

    DL_FOREACH(logger->cfg.rules, rule1) {
        int count1;
        DL_COUNT(rule1->rule_body, sym1, count1);
        symbols_count1 += count1;

        RECORDER_LOGINFO("Rule %d :-> ", rule1->val);

        DL_FOREACH(rule1->rule_body, sym1) {
            if(sym1->exp > 1)
                RECORDER_LOGINFO("%d^%d ", sym1->val, sym1->exp);
            else
                RECORDER_LOGINFO("%d ", sym1->val);
        }
        RECORDER_LOGINFO("\n");
    }

    int stack[SIZE];
    top = -1;

    int target_rule = find_pattern_start(logger, entry->terminal_id, true, stack);
    int max_rule = target_rule;
    int max_rule_count = 0;

    RuleConfidence *s = NULL;
    HASH_FIND_INT(cfg_conf, &target_rule, s);
    if (s){
        max_rule_count = s->count;
    }

    if (target_rule < -1){
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

        if (max_rule_count > THRESHOLD){
            printf("Chosen rule %d, count %d, terminal ID %d , Rank %d\n", max_rule, max_rule_count, entry->terminal_id, logger->rank);
            find_pattern_start(logger, max_rule, false, stack);
        }
    }

    // show(stack);
    if (top != -1){
        extract_io_behavior(logger, stack);
    }
}

int recorder_analysis(RecorderLogger* logger,  Record* record, CallSignature* entry) {
    const char * func_name = get_function_name_by_id(record->func_id);
    RECORDER_LOGINFO("Function Name %s\n", func_name);

    if(strcmp("MPI_File_close", func_name) == 0){
        int *sub_avgs = NULL;
        if (logger->rank == 0) {
            sub_avgs = (int *) recorder_malloc(sizeof(int) * logger->nprocs);
        }

        // RECORDER_LOGINFO("PMPI_Gather start with rank %d\n", logger->rank);
        GOTCHA_REAL_CALL(MPI_Gather)(&(logger->current_cfg_terminal), 1, MPI_INT, sub_avgs, 1, MPI_INT, 0, MPI_COMM_WORLD);
        // RECORDER_LOGINFO("PMPI_Gather end with rank %d\n", logger->rank);
        // if (logger->rank == 0){
        //     for(int i = 0; i < logger->nprocs; i++){
        //         RECORDER_LOGINFO("Val %d :-> ", sub_avgs[i]);
        //     }
        //     printf("\n");        
        // }

        if (logger->rank == 0)
            recorder_free(sub_avgs, logger->nprocs);
    }

    find_rule_confidence(logger, entry);
    analysis_cs(logger, entry);

    return 0;
}
