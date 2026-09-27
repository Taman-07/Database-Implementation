#include <stdio.h>
#include <string.h>
#include<direct.h>
#include <ctype.h>
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
        return;
    }

    char *piece=strtok(schema_text, ",");
    
    while(piece!=NULL){
        char col_name[64];
        char col_type[16];
        int chars_read=0;

        if(sscanf(piece, "%63s %15s%n", col_name, col_type, &chars_read)!=2){
            fprintf(stderr, "Error: could not parse column '%s'.\n", piece);
            fclose(schema_file);
            remove(schema_path);
            remove(tbl_path);
            return;
        }

        char leftover[64];
        if(sscanf(piece+chars_read, "%63s", leftover)!=1){
            fprintf(stderr, "Error: unexpected extra text after column definition '%s'.\n", piece);
            fclose(schema_file);
            remove(schema_path);
            remove(tbl_path);
            return;
        }

        if(strcmp(col_type, "INT")!=0 && strcmp(col_type, "TEXT")!=0){
            fprintf(stderr, "Error: invalid type '%s' for column '%s'. Only INT and TEXT allowed.\n", col_type, col_name);
            fclose(schema_file);
            remove(schema_path);
            remove(tbl_path);
            return;
        }

        fprintf(schema_file, "%s %s\n", col_name, col_type);
        piece=strtok(NULL, ",");
    }

    fclose(schema_file);

    printf("Table '%s' created. \n", table_name);
    printf("Schema '%s' created. \n", table_name);
}

void insert_into_table(const char *db_name, const char *rest_of_command){
    if(db_name==NULL || db_name[0]=='\0'){
        fprintf(stderr, "Error: no database selected\n");
        return;
    }

    char table_name[64];
    if(sscanf(rest_of_command, "%63[^ ]", table_name)!=1 || table_name[0]=='\0'){
        fprintf(stderr, "Error: could not parse table name.\n");
        return;
    }

    char db_dir[256];
    snprintf(db_dir, sizeof(db_dir), "data/databases/%s", db_name);

    char tbl_path[300], schema_path[300];
    snprintf(tbl_path, sizeof(tbl_path), "%s/%s.tbl", db_dir, table_name);
    snprintf(schema_path, sizeof(schema_path), "%s/%s.schema", db_dir, table_name);


    FILE *schema_file = fopen(schema_path, "r");
    if(schema_file==NULL){
        fprintf(stderr, "Error: could not open schema for table '%s'.\n", table_name);
        return;
    }

    char col_types[32][16];
    int col_count = 0;
    char schema_line[128];

    while(fgets(schema_line, sizeof(schema_line), schema_file)!=NULL && col_count < 32){
        char dummy_name[64];
        sscanf(schema_line, "%63s %15s", dummy_name, col_types[col_count]);
        col_count++;
    }
    fclose(schema_file);

    const char *values_kw = strstr(rest_of_command, "VALUES");
    if(values_kw == NULL){
        fprintf(stderr, "Error: expected VALUES keyword.\n");
        return;
    }

    const char *open_paren = strchr(values_kw, '(');
    const char *close_paren = strrchr(values_kw, ')');
    if(open_paren==NULL || close_paren==NULL || close_paren<=open_paren){
        fprintf(stderr, "Error: could not find value list.\n");
        return;
    }

    char values_text[256];
    size_t len = close_paren - (open_paren+1);
    strncpy(values_text, open_paren+1, len);
    values_text[len]='\0';

    char parsed_values[32][64];
    int val_count=0;

    char *piece = strtok(values_text, ",");
    while(piece != NULL){
        if(val_count >= 32){
            fprintf(stderr, "Error: too many values.\n");
            return;
        }

        while(*piece==' ') piece++;

        char cleaned[64];
        sscanf(piece, "%63s", cleaned);

        size_t clen = strlen(cleaned);
        if(clen>=2 && cleaned[0]=='\'' && cleaned[clen-1]=='\''){
            memmove(cleaned, cleaned+1, clen-2);
            cleaned[clen-2]='\0';
        }

        strcpy(parsed_values[val_count], cleaned);
        val_count++;
        piece = strtok(NULL, ",");
    }

    if(val_count != col_count){
        fprintf(stderr, "Error: expected %d values but got %d.\n", col_count, val_count);
        return;
    }

    for(int i=0; i<col_count; i++){
        if(strcmp(col_types[i], "INT")==0){
            for(int j=0; parsed_values[i][j]; j++){
                if(!isdigit((unsigned char)parsed_values[i][j]) && !(j==0 && parsed_values[i][j]=='-')){
                    fprintf(stderr, "Error: value '%s' is not a valid INT.\n", parsed_values[i]);
                    return;
                }
            }
        }
    }

    FILE *tbl_file = fopen(tbl_path, "a");
    if(tbl_file==NULL){
        fprintf(stderr, "Error: could not open table file '%s'.\n", tbl_path);
        return;
    }

    for(int i=0;i<val_count;i++){
        fprintf(tbl_file, "%s", parsed_values[i]);
        if(i < val_count-1) fprintf(tbl_file, ",");
    }
    fprintf(tbl_file, "\n");
    fclose(tbl_file);

    printf("1 row inserted into '%s'.\n", table_name);
}


