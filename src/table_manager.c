#include <stdio.h>
#include <string.h>
#include<direct.h>
#include <ctype.h>
#include <stdlib.h>
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
        if(sscanf(piece+chars_read, "%63s", leftover)==1){
            fprintf(stderr, "Error: unexpected extra text after column definition '%s'.\n", piece);
            fclose(schema_file);
            remove(schema_path);
            remove(tbl_path);
            return;
        }

        if(strcmp(col_type, "INT")!=0 && strcmp(col_type, "STRING")!=0 && strcmp(col_type, "TEXT")!=0 &&
           strcmp(col_type, "FLOAT")!=0 && strcmp(col_type, "DATE")!=0){
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
        if(sscanf(schema_line, "%63s %15s", dummy_name, col_types[col_count]) == 2){
            col_count++;
        }
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

    char parsed_values[32][64];
    int is_quoted[32];
    int val_count = 0;

    const char *p = open_paren + 1;
    while(p < close_paren){
        while(p < close_paren && *p == ' ') p++;

        if(val_count >= 32){
            fprintf(stderr, "Error: too many values.\n");
            return;
        }

        char buf[64];
        int n = 0;
        int quoted = 0;

        if(*p == '\''){
            quoted = 1;
            p++;
            while(p < close_paren && *p != '\''){
                if(n < 63) buf[n++] = *p;
                p++;
            }
            if(p >= close_paren){
                fprintf(stderr, "Error: missing closing quote.\n");
                return;
            }
            p++; 
        }
        else{
            while(p < close_paren && *p != ','){
                if(n < 63) buf[n++] = *p;
                p++;
            }
            while(n > 0 && buf[n-1] == ' ') n--; 
        }
        buf[n] = '\0';

        while(p < close_paren && *p == ' ') p++;
        if(p < close_paren && *p != ','){
            fprintf(stderr, "Error: unexpected text after value '%s'.\n", buf);
            return;
        }
        if(p < close_paren) p++; 

        strcpy(parsed_values[val_count], buf);
        is_quoted[val_count] = quoted;
        val_count++;
    }

    if(val_count != col_count){
        fprintf(stderr, "Error: expected %d values but got %d.\n", col_count, val_count);
        return;
    }

    // type validation
    for(int i = 0; i < col_count; i++){
        const char *v = parsed_values[i];
        const char *type = col_types[i];

        if(strcmp(type, "INT") == 0){
            int ok = !is_quoted[i] && v[0] != '\0';
            for(int j = 0; ok && v[j]; j++){
                if(!isdigit((unsigned char)v[j]) && !(j == 0 && v[j] == '-' && v[1] != '\0')) ok = 0;
            }
            if(!ok){
                fprintf(stderr, "Error: value '%s' is not a valid INT (column %d).\n", v, i + 1);
                return;
            }
        }
        else if(strcmp(type, "FLOAT") == 0){
            char *end;
            if(is_quoted[i] || v[0] == '\0'){
                fprintf(stderr, "Error: value '%s' is not a valid FLOAT (column %d).\n", v, i + 1);
                return;
            }
            strtod(v, &end);
            if(*end != '\0'){
                fprintf(stderr, "Error: value '%s' is not a valid FLOAT (column %d).\n", v, i + 1);
                return;
            }
        }
        else if(strcmp(type, "STRING") == 0 || strcmp(type, "TEXT") == 0){
            if(!is_quoted[i]){
                fprintf(stderr, "Error: STRING value must be in single quotes (column %d).\n", i + 1);
                return;
            }
        }
        else if(strcmp(type, "DATE") == 0){
            int ok = is_quoted[i] && strlen(v) == 10 && v[4] == '-' && v[7] == '-';
            for(int j = 0; ok && j < 10; j++){
                if(j != 4 && j != 7 && !isdigit((unsigned char)v[j])) ok = 0;
            }
            if(ok){
                int m = (v[5] - '0') * 10 + (v[6] - '0');
                int d = (v[8] - '0') * 10 + (v[9] - '0');
                if(m < 1 || m > 12 || d < 1 || d > 31) ok = 0;
            }
            if(!ok){
                fprintf(stderr, "Error: DATE must be like '2024-01-15' (column %d).\n", i + 1);
                return;
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


int drop_table(const char *db_name, const char *table_name){
    char name[64];
    char tbl_path[300];
    char schema_path[300];

    strncpy(name, table_name, sizeof(name) - 1);
    name[sizeof(name) - 1] = '\0';

    size_t len = strlen(name);
    while(len > 0 && (isspace((unsigned char)name[len-1]) || name[len-1] == ';')){
        name[--len] = '\0';
    }
    if(len == 0){
        fprintf(stderr, "Error: table name missing.\n");
        return 0;
    }

    if(strchr(name, '/') || strchr(name, '\\') || strstr(name, "..")){
        fprintf(stderr, "Error: invalid table name.\n");
        return 0;
    }

    snprintf(tbl_path, sizeof(tbl_path), "data/databases/%s/%s.tbl", db_name, name);
    snprintf(schema_path, sizeof(schema_path), "data/databases/%s/%s.schema", db_name, name);

    if(remove(tbl_path) != 0){
        fprintf(stderr, "Error: table '%s' does not exist.\n", name);
        return 0;
    }
    remove(schema_path);  

    printf("Table '%s' dropped.\n", name);
    return 1;
}

static void skip_spaces(const char **p){
    while(**p && isspace((unsigned char)**p)) (*p)++;
}

static int match_keyword(const char **p, const char *kw){
    skip_spaces(p);
    size_t n=strlen(kw);
    for(size_t i=0;i<n;i++){
        if(toupper((unsigned char) (*p)[i])!=kw[i]){
            return 0;
        }
    }
    char after=(*p)[n];
    if(after != '\0' && !isspace((unsigned char) after) && after !=';'){
        return 0;
    }
    *p+=n;
    return 1;
}


static int row_matches(const char *cell, const char *type, const char *op, const char *val){
    int cmp;
    if(strcmp(type, "INT") == 0 || strcmp(type, "FLOAT") == 0){
        double a = strtod(cell, NULL);
        double b = strtod(val, NULL);
        cmp = (a < b) ? -1 : (a > b) ? 1 : 0;
    } else {
        cmp = strcmp(cell, val);   // works for TEXT/STRING and ISO dates
    }

    if(strcmp(op, "=")  == 0) return cmp == 0;
    if(strcmp(op, "!=") == 0 || strcmp(op, "<>") == 0) return cmp != 0;
    if(strcmp(op, "<")  == 0) return cmp <  0;
    if(strcmp(op, ">")  == 0) return cmp >  0;
    if(strcmp(op, "<=") == 0) return cmp <= 0;
    if(strcmp(op, ">=") == 0) return cmp >= 0;
    return 0;
}

void select_fromtable(const char *db_name, const char *rest_of_command){
    if(db_name == NULL || db_name[0] == '\0'){
        fprintf(stderr, "Error: no database selected\n");
        return;
    }

    const char *p = rest_of_command;

    skip_spaces(&p);
    if(*p != '*'){
        fprintf(stderr, "Error: only SELECT * is supported.\n");
        return;
    }
    p++;

    if(!match_keyword(&p, "FROM")){
        fprintf(stderr, "Error: expected FROM.\n");
        return;
    }

    skip_spaces(&p);
    char table_name[64];
    int n = 0;
    while(*p && !isspace((unsigned char)*p) && *p != ';' && n < 63){
        table_name[n++] = *p++;
    }
    table_name[n] = '\0';
    if(n == 0){
        fprintf(stderr, "Error: table name missing.\n");
        return;
    }
    if(strchr(table_name, '/') || strchr(table_name, '\\') || strstr(table_name, "..")){
        fprintf(stderr, "Error: invalid table name.\n");
        return;
    }

    int has_where = 0;
    char where_col[64] = "";
    char where_op[3] = "";
    char where_val[64] = "";

    if(match_keyword(&p, "WHERE")){
        has_where = 1;

        skip_spaces(&p);
        n = 0;
        while(*p && !isspace((unsigned char)*p) && !strchr("=!<>;", *p) && n < 63){
            where_col[n++] = *p++;
        }
        where_col[n] = '\0';

        skip_spaces(&p);
        n = 0;
        while(*p && strchr("=!<>", *p) && n < 2){
            where_op[n++] = *p++;
        }
        where_op[n] = '\0';

        if(where_col[0] == '\0' || where_op[0] == '\0'){
            fprintf(stderr, "Error: WHERE must look like: WHERE column > value\n");
            return;
        }
        if(strcmp(where_op, "=") && strcmp(where_op, "!=") && strcmp(where_op, "<>") &&
           strcmp(where_op, "<") && strcmp(where_op, ">") &&
           strcmp(where_op, "<=") && strcmp(where_op, ">=")){
            fprintf(stderr, "Error: unknown operator '%s'.\n", where_op);
            return;
        }

        skip_spaces(&p);
        char raw[64];
        strncpy(raw, p, sizeof(raw) - 1);
        raw[sizeof(raw) - 1] = '\0';
        size_t len = strlen(raw);
        while(len > 0 && (isspace((unsigned char)raw[len-1]) || raw[len-1] == ';')) raw[--len] = '\0';

        if(len == 0){
            fprintf(stderr, "Error: WHERE value missing.\n");
            return;
        }
        if(len >= 2 && raw[0] == '\'' && raw[len-1] == '\''){
            raw[len-1] = '\0';
            strcpy(where_val, raw + 1);
        } else {
            strcpy(where_val, raw);
        }
    }
    else {
        skip_spaces(&p);
        if(*p == ';') p++;
        skip_spaces(&p);
        if(*p != '\0'){
            fprintf(stderr, "Error: unexpected text '%s'.\n", p);
            return;
        }
    }

    char db_dir[256], tbl_path[300], schema_path[300];
    snprintf(db_dir, sizeof(db_dir), "data/databases/%s", db_name);
    snprintf(tbl_path, sizeof(tbl_path), "%s/%s.tbl", db_dir, table_name);
    snprintf(schema_path, sizeof(schema_path), "%s/%s.schema", db_dir, table_name);

    FILE *schema_file = fopen(schema_path, "r");
    if(schema_file == NULL){
        fprintf(stderr, "Error: table '%s' does not exist.\n", table_name);
        return;
    }

    char col_names[32][64];
    char col_types[32][16];
    int col_count = 0;
    char line[512];

    while(col_count < 32 && fgets(line, sizeof(line), schema_file) != NULL){
        if(sscanf(line, "%63s %15s", col_names[col_count], col_types[col_count]) == 2){
            col_count++;
        }
    }
    fclose(schema_file);

    int where_idx = -1;
    if(has_where){
        for(int i = 0; i < col_count; i++){
            if(strcmp(col_names[i], where_col) == 0){ where_idx = i; break; }
        }
        if(where_idx < 0){
            fprintf(stderr, "Error: column '%s' does not exist in '%s'.\n", where_col, table_name);
            return;
        }
        const char *t = col_types[where_idx];
        if(strcmp(t, "INT") == 0 || strcmp(t, "FLOAT") == 0){
            char *end;
            strtod(where_val, &end);
            if(*end != '\0' || where_val[0] == '\0'){
                fprintf(stderr, "Error: '%s' is not a number for column '%s'.\n", where_val, where_col);
                return;
            }
        }
    }

    FILE *tbl_file = fopen(tbl_path, "r");
    if(tbl_file == NULL){
        fprintf(stderr, "Error: could not open table file '%s'.\n", tbl_path);
        return;
    }

    for(int i = 0; i < col_count; i++) printf("%-15s", col_names[i]);
    printf("\n");
    for(int i = 0; i < col_count * 15; i++) printf("-");
    printf("\n");


    int shown = 0;
    while(fgets(line, sizeof(line), tbl_file) != NULL){
        size_t len = strlen(line);
        while(len > 0 && (line[len-1] == '\n' || line[len-1] == '\r')) line[--len] = '\0';
        if(len == 0) continue;

        char *fields[32];
        int fc = 0;
        char *tok = line;
        while(fc < 32){
            fields[fc++] = tok;
            char *comma = strchr(tok, ',');
            if(comma == NULL) break;
            *comma = '\0';
            tok = comma + 1;
        }
        if(fc != col_count) continue;  

        if(has_where && !row_matches(fields[where_idx], col_types[where_idx], where_op, where_val)){
            continue;
        }

        for(int i = 0; i < fc; i++) printf("%-15s", fields[i]);
        printf("\n");
        shown++;
    }
    fclose(tbl_file);
    printf("(%d row%s)\n", shown, shown == 1 ? "" : "s");
}

