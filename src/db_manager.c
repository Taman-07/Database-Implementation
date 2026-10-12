#include<stdio.h>
#include<string.h>
#include<direct.h>
#include<errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <windows.h>
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

bool exists_database(const char *name){
    char path[256];
    snprintf(path, sizeof(path), "data/databases/%s", name);

    struct stat st;
    if(stat(path, &st)==0 && (st.st_mode & _S_IFDIR)){
        return true;
    }
    return false;
}


bool drop_database(const char *name){
    char path[256];
    snprintf(path, sizeof(path), "data/databases/%s", name);

    if(!exists_database(name)){
        printf("Database '%s' does not exist.\n", name);
        return false;
    }

    char pattern[300];
    snprintf(pattern, sizeof(pattern), "%s/*", path);

    WIN32_FIND_DATAA fd;
    HANDLE h=FindFirstFileA(pattern, &fd);

    if(h!=INVALID_HANDLE_VALUE){
        do{
            if(strcmp(fd.cFileName, ".")==0 || strcmp(fd.cFileName, "..")==0){
                continue;
            }
            char file_path[512];
            snprintf(file_path, sizeof(file_path), "%s/%s", path, fd.cFileName);
            remove(file_path);
        }
        while(FindNextFileA(h, &fd));
        FindClose(h);
    }

    if(_rmdir(path)==0){
        printf("Database '%s' dropped.\n", name);
        return true;
    }
    printf("Could not drop database '%s'.\n", name);
    return false;
}
