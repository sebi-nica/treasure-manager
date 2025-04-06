#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/stat.h>

typedef struct {
  char treasure_id[8], user_name[32], clue_text[64];
  float x, y;
  int value;
}treasure;

void error(char* message){
  printf("[ERROR] type=%s\n", message);
  exit(-1);
}


void printHelpMessage(){
  printf("help message\n");
  exit(0);
}

void addTreasure(char* hunt_name){
  char cwd_name[128];
  if(getcwd(cwd_name, sizeof(cwd)) == NULL) error("getcwd");
  DIR* cwd = opendir(cwd_name);
  if(cwd == NULL) error("opendir");
  struct dirent *entry;
  struct stat file_data;
  while((entry = readdir(cwd)) != NULL){
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
      continue;
    if(stat(entry->d_name, file_data) < 0) error("stat");
    
    if(strcmp(subdir->d_name, hunt_name))
  }
}

int getCommand(char* command){
  for(int i = 0; i < strlen(command); i++)
    command[i] = tolower(command[i]);
  if(!strcmp(command, "help")) return 0;
  if(!strcmp(command, "add")) return 1;
  if(!strcmp(command, "list")) return 2;
  if(!strcmp(command, "view")) return 3;
  if(!strcmp(command, "removetreasure")) return 4;
  if(!strcmp(command, "removehunt")) return 5;
  return 6;
}

int main(int argc, char** argv){
  if(argc <= 1) error("not_enough_args");
  switch(getCommand(argv[1])){
  case 0:
    printHelpMessage();
    break;
  case 1:
    if(argc != 3) error("not_enough_args_for_add");
    printf("adding treasure in hunt [%s]\n", argv[2]);
    break;
  default:
    error("unknown_command");
    break;   
  }
  return 0;
}
