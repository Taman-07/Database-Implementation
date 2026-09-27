#ifndef TABLE_MANAGER_H
#define TABLE_MANAGER_H

typedef struct{
    char name[64];
    char type[16];
} Column;

void create_table(const char *db_name, const char *rest_of_command);
void insert_into_table(const char *db_name, const char *rest_of_command);

#endif

