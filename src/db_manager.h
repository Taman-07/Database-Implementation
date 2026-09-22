#ifndef DB_MANAGER_H
#define DB_MANAGER_H
#include <stdbool.h>

void ensure_base_directories(void);
void create_database(const char *name);
bool exists_database(const char *name);

#endif
