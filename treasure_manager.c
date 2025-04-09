#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <stdarg.h>

typedef struct {
  char id[16], user_name[32], clue_text[64];
  float x, y;
  int value;
}treasure;

void error(char* message){
  printf("[ERROR] type=%s\n", message);
  exit(-1);
}



void printHelpMessage(){
  printf("help message\n   add <hunt_name> - adds a treasure in specified hunt\n   list <hunt_name> - lists all treasures in specified hunt\n");
  printf("   view <hunt_name> <treasure_id> - display info about a hunt\n   remove_treasure <hunt_name> <treasure_id> - deletes a treasure\n   remove_hunt <hunt_name> - remove an entire hunt\n");
  exit(0);
}

treasure readTreasure(char* username){
  treasure t;
  strcpy(t.user_name, username);
  printf("enter treasure ID ->");
  scanf("%16s", t.id);
  printf("enter value ->");
  scanf("%d", &t.value);
  printf("enter coord [x] ->");
  scanf("%f", &t.x);
  printf("enter coord [y] ->");
  scanf("%f", &t.y);
  printf("enter clue ->");
  scanf("%64s", t.clue_text);
  return t;
}

void printTreasure(treasure t){
  printf("ID: %s\nUSER_NAME: %s\nVALUE: %d\nCOORD: x=%f, y=%f\nCLUE: %s\n", t.id, t.user_name, t.value, t.x, t.y, t.clue_text);
  return;
}

void logAction(char* hunt_name, const char* format, ...) {//ez to use log function
    char path[128];
    snprintf(path, sizeof(path), "./%s/logged_hunt.txt", hunt_name);

    int log_fd = open(path, O_CREAT | O_WRONLY | O_APPEND, 0644);
    if (log_fd == -1){
    	snprintf(path, sizeof(path), "./logged_hunt_%s.txt", hunt_name);
    	log_fd = open(path, O_WRONLY | O_APPEND, 0644);
    	if (log_fd == -1) error("open_log_file");
    }

    char message[512]; 
    va_list args;
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);
    
    strcat(message, "\n");

    write(log_fd, message, strlen(message));
    close(log_fd);
}

void addTreasure(char* hunt_name){
  DIR *cwd = opendir(".");
  if(cwd == NULL) error("opendir_cwd");
  struct dirent *entry;
  while((entry = readdir(cwd)) != NULL){
    if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
      continue;
    if(strcmp(entry->d_name, hunt_name) == 0) break;
  }
  int create_hunt = 0;
  if(entry == NULL){
    if(mkdir(hunt_name, 0755)){
      closedir(cwd);
      error("mkdir");
    }
		create_hunt = 1;
  }
  DIR *hunt_dir = opendir(hunt_name);
  if(hunt_dir == NULL){
      closedir(cwd);
      error("opendir_huntdir");
    }
  closedir(cwd);
  //we now have a pointer to hunt directory in 'hunt_dir'
  printf("user name->");
  char username[32];
  scanf("%s", username);
  if(create_hunt)     logAction(hunt_name, "[user=%s] created hunt [hunt_name=%s]", username, hunt_name); // LOG IT
  // build the file path
  char path[128];
  snprintf(path, sizeof(path), "./%s/%s", hunt_name,  username);
  int fd = open(path, O_RDWR | O_CREAT, 0644); // if the file exists, open it, if not, create it
  if (fd == -1){
      closedir(hunt_dir);
      error("open");
    }
  lseek(fd, 0, SEEK_END);   // go to the end
  treasure new_treasure = readTreasure(username);
  if (write(fd, &new_treasure, sizeof(treasure)) != sizeof(treasure)) {
        error("write");
        close(fd);
    }
  logAction(hunt_name, "[user=%s]added treasure [treasure_id=%s]", new_treasure.user_name, new_treasure.id);
  close(fd);
  return;
}

void viewTreasure(char* hunt_name, char* treasure_id) {
    char path[128];
    printf("user name->");
    char username[32];
    scanf("%s", username);
    snprintf(path, sizeof(path), "./%s/%s", hunt_name, username);
    int fd = open(path, O_RDONLY);
    if(fd == -1) error("open_treasures_file");
    treasure t;
    while (read(fd, &t, sizeof(treasure)) == sizeof(treasure))
      if(strcmp(t.id, treasure_id) == 0){
      printTreasure(t);
      close(fd);
      return;
      }
    printf("no treasure found\n");
    close(fd);
    return;
}

void listTreasure(char* hunt_name){
  DIR *hunt_dir = opendir(hunt_name);
  if(hunt_dir == NULL) error("opendir_huntdir");
  struct dirent *entry;
  char path[1024];
  int fd;
  treasure t;
  while((entry = readdir(hunt_dir)) != NULL){
    if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0 || strcmp(entry->d_name, "logged_hunt.txt") == 0)
      continue;
    snprintf(path, sizeof(path), "./%s/%s", hunt_name, entry->d_name);
    fd = open(path, O_RDONLY);
    if(fd == -1) error("open_treasures_file");
    while (read(fd, &t, sizeof(treasure)) == sizeof(treasure)){
      printTreasure(t);
      printf("\n");
    }
    close(fd);
  }
  closedir(hunt_dir);
  return;
}

void removeTreasure(char* hunt_name, char* treasure_id){
  char path[128];
  printf("user name->");
  char username[32];
  scanf("%s", username);
  snprintf(path, sizeof(path), "./%s/%s", hunt_name, username);
	printf("path: %s\n", path);
  int fd = open(path, O_RDWR);
  if(fd == -1) error("open_treasures_file");

  treasure t;
  int found = 0;
  while (read(fd, &t, sizeof(treasure)) == sizeof(treasure)) {
    if(strcmp(t.id, treasure_id) == 0) {
      found = 1;
      printf("found it\n");
      break;
    }
  }
  if (!found) {
    printf("treasure not found\n"); 
    close(fd);
    return;
  }
  
  char buffer[sizeof(treasure)];
  while (read(fd, buffer, sizeof(treasure)) == sizeof(treasure)) {//read each treasure after the one to be deleted
    lseek(fd, -2*sizeof(treasure), SEEK_CUR);//go back 
    write(fd, buffer, sizeof(treasure));//and overwrite the one behind it
    lseek(fd, sizeof(treasure), SEEK_CUR);
    printf("moved a treasure\n");
  }

  ftruncate(fd, lseek(fd, 0, SEEK_END) - sizeof(treasure));//truncate the file by one treasure
	
  logAction(hunt_name, "[user=%s]removed treasure [treasure_id=%s]", username, treasure_id); // LOG IT
	
  close(fd);
}

void removeHunt(char* hunt_name) {
    char src[128], dst[128], hunt_dir[128];
    snprintf(src, sizeof(src), "./%s/logged_hunt.txt", hunt_name);
    snprintf(dst, sizeof(dst), "./logged_hunt_%s.txt", hunt_name);
    snprintf(hunt_dir, sizeof(hunt_dir), "./%s", hunt_name);
    
    printf("user name->");
 		char username[32];
	  scanf("%s", username);

    if (rename(src, dst) == -1){
    	open(src, O_RDWR | O_CREAT, 0644); // in the case where for some reason the log file is not in the hunt
    	if (rename(src, dst) == -1)
    		error("moving_log_file");				//we must create an empty one 
		}
    DIR *dir = opendir(hunt_dir);
    if (!dir) error("opendir_hunt_dir");

    struct dirent *entry;
    char file_path[512];

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
        snprintf(file_path, sizeof(file_path), "%s/%s", hunt_dir, entry->d_name);
        if (remove(file_path) == -1) error("remove_file_in_hunt_dir");
    }
    closedir(dir);

    if (rmdir(hunt_dir) == -1) error("rmdir_hunt_dir");
    logAction(hunt_name, "[user=%s]removed hunt [hunt_name=%s]", username, hunt_name); // LOG IT
}


int getCommand(char* command){
  for(int i = 0; i < strlen(command); i++)
    command[i] = tolower(command[i]);
  if(!strcmp(command, "help")) return 0;
  if(!strcmp(command, "add")) return 1;
  if(!strcmp(command, "list")) return 2;
  if(!strcmp(command, "view")) return 3;
  if(!strcmp(command, "remove_treasure")) return 4;
  if(!strcmp(command, "remove_hunt")) return 5;
  return 6;
}

int main(int argc, char** argv){
  if(argc <= 1) error("not_enough_args");
  switch(getCommand(argv[1])){
  case 0:
    printHelpMessage();
    break;
  case 1:
    if(argc != 3) error("incorrect_arg_number");
    printf("adding treasure in hunt %s\n\n", argv[2]);
    addTreasure(argv[2]);
    break;
  case 2:
    if(argc != 3) error("incorrect_arg_number");
    printf("listing treasures in hunt %s\n\n", argv[2]);
    listTreasure(argv[2]);
    break;
  case 3:
    if(argc != 4) error("incorrect_arg_number");
    printf("viewing treasure %s in hunt %s\n\n", argv[3], argv[2]);
    viewTreasure(argv[2], argv[3]);
    break;
  case 4:
  	if(argc != 4) error("incorrect_arg_number");
  	printf("deleting treasure %s in hunt %s\n\n", argv[3], argv[2]);
  	removeTreasure(argv[2], argv[3]);
  	break;
 	case 5:
 		if(argc != 3) error("incorrect_arg_number");
 		printf("deleting hunt %s\n\n", argv[2]);
 		removeHunt(argv[2]);
 		break;
  default:
    error("unknown_command");
    break;   
  }
  return 0;
}
