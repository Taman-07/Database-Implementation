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

    // table file
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

    // schema file
    const char *open_paren=strchr(rest_of_command, '(');
    const char *closed_paren=strrchr(rest_of_command, ')');

    if(open_paren==NULL || closed_paren==NULL || closed_paren <= open_paren){
        fprintf(stderr, "Error: could not find column list in '%s'.\n", rest_of_command);
        return;
    }

    char schema_text[256];
    size_t len=closed_paren-(open_paren+1);
    strncpy(schema_text, open_paren + 1, len);
    schema_text[len]='\0';

    FILE *schema_file=fopen(schema_path, "w");
    if(schema_file==NULL){
        fprintf(stderr, "Error: could not create table file '%s'.\n", schema_path);
        fclose(tbl_file);
        return;
    }

    char *piece=strtok(schema_text, ",");
    while(piece!=NULL){
        char col_name[64];
        char col_type[16];
        if(sscanf(piece, "%63s %15s", col_name, col_type)!=2){
            fprintf(stderr, "Error: could not parse column '%s'.\n", piece);
            fclose(schema_file);
            return;
        }

        if(strcmp(col_type, "INT")!=0 && strcmp(col_type, "TEXT")!=0){
            fprintf(stderr, "Error: invalid type '%s' for column '%s'. Only INT and TEXT allowed.\n", col_type, col_name);
            fclose(schema_file);
            return;
        }

        fprintf(schema_file, "%s %s\n", col_name, col_type);
        piece=strtok(NULL, ",");
    }

    fclose(schema_file);

    printf("Table '%s' created. \n", table_name);
    printf("Schema '%s' created. \n", table_name);
}

