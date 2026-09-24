#include <stdio.h>
#include <string.h>
#include<direct.h>
#include "table_manager.h"
#include "db_manager.h"


void create_table(const char *db_name, const char *rest_of_command){
    if(db_name==NULL || db_name[0]=='\0'){
        fprintf(stderr, "Error: no database selected");
        return;
    }
    char table_name[64];
    if (sscanf(rest_of_command, "%63[^ (]", table_name)!=1 || table_name[0]=='\0') {
        fprintf(stderr, "Error: could not parse table name.\n");
        return;
    }
    char db_dir[256];
    snprintf(db_dir, sizeof(db_dir), "data/databases/%s", db_name);
    mkdir(db_dir);

    char tbl_path[300];
    char schema_path[300];
    snprintf(tbl_path, sizeof(tbl_path), "%s/%s.tbl", db_dir, table_name);
    snprintf(schema_path, sizeof(schema_path), "%s/%s.schema", db_dir, table_name);

    FILE *tbl_file=fopen(tbl_path, "w");
    if(tbl_file==NULL){
        fprintf(stderr, "Error: could not create table file '%s'.\n", tbl_path);
        return;
    }
    fclose(tbl_file);
    printf("Table '%s' created. \n", table_name);
}
