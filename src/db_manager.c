#include<stdio.h>
#include<string.h>
#include<direct.h>
#include<errno.h>
#include "db_manager.h"

void ensure_base_directories(void){
    mkdir("data");
    mkdir("data/databases");
    mkdir("data/system");
}

void create_database(const char *name){
    char path[256];
    snprintf(path, sizeof(path), "data/databases/%s", name);
    if(mkdir(path)==0){
        printf("Database '%s' created.\n", name);
    }
    else{
        if(errno==EEXIST){
            printf("Database '%s' already exists.\n", name);
        }
        else{
            printf("Failed to create database '%s'.\n", name);
        }
    }
}

