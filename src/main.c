#include<stdio.h>
#include<string.h>
#include "db_manager.h"

int main()
{
    char command[50];

    // if folder doesn't exists
    ensure_base_directories();

    while(1){
        printf("Enter command: ");
        fgets(command, sizeof(command), stdin);
        
        size_t len=strlen(command);
        if(len>0 && command[len-1]=='\n'){
            command[len-1]='\0';
        }

        if(strncmp(command, "EXIT", 4)==0){
            break;
        }

        if(strncmp(command, "CREATE DATABASE ", 16)==0){
            const char *db_name=command+16;
            create_database(db_name);
            printf("Database name , name = %s\n", db_name);
        }

        else if (strncmp(command, "USE ", 4) == 0) {
            const char *db_name = command + 4;
            printf("Currently using , name = %s\n", db_name);
        }

        else{
            printf("Enter the valid command \n");
        }
    }   
}
