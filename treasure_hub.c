#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>

pid_t monitor_pid = -1; // monitor process ID
int monitor_state = 0; // 0 = off 1 = stopping 2 = on

void compileDependencies(){
	int syscall_return;
	syscall_return = system("gcc treasure_manager.c -o manager_exec");
	if (syscall_return != 0) {
		  fprintf(stderr, "ERROR: failed to compile treasure_manager.c\n");
		  exit(EXIT_FAILURE);
	}
	syscall_return = system("gcc monitor.c -o monitor_exec");
	if (syscall_return != 0) {
		  fprintf(stderr, "ERROR: failed to compile monitor.c\n");
		  exit(EXIT_FAILURE);
	}
}

void printHelpMessage(){
  printf("help message\n	start_monitor - starts the monitor process\n	list_hunts - [available only when monitor is on] - lists all hunts and number of treasures in each hunt\n	");
  printf("list_treasures <hunt_id> - [available only when monitor is on] - lists all treasures in a hunt\n	view_treasure <hunt_id> <treasure_id> - [available only when monitor is on] - view a single treasure\n	");
  printf("stop_monitor - stops the monitor process\n	exit - exits the program. doesn't work if the monitor is still running\n");
}

void startMonitor(){
	if(monitor_state == 2){
		printf("<<monitor already running\n");
		return;
	}else if(monitor_state == 1){
		kill(monitor_pid, SIGUSR2);
		monitor_state = 2;
		printf("<<monitor termination cancelled\n");
		return;
	}
	monitor_pid = fork();
	switch(monitor_pid){
		case -1: // error
			printf("fork error\n");
			return;
			break;
		case 0: // child
			if (execl("./monitor_exec", "monitor_exec", NULL) < 0) { // check if monitor executable is ok
            perror("execl");
            exit(EXIT_FAILURE);
        }
			break;
		default: // parent
			printf("<<monitor is ON.\n");
			monitor_state = 2;
			break;
	}
}

void stopMonitor(){
	if(monitor_state == 1){
		printf("<<monitor already stopping\n");
		return;
	} else if(monitor_state == 0){
		printf("<<monitor already off\n");
		return;
	}
	kill(monitor_pid, SIGTERM); // sending monitor termination signal
	monitor_state = 1;
	printf("<<terminating monitor in 5 seconds\n");
}

int getCommand(char* command) {
  if (strncmp(command, "help", strlen("help")) == 0) return 0;
  if (strncmp(command, "start_monitor", strlen("start_monitor")) == 0) return 1;
  if (strncmp(command, "list_hunts", strlen("list_hunts")) == 0) return 2;
  if (strncmp(command, "list_treasures", strlen("list_treasures")) == 0) return 3;
  if (strncmp(command, "view_treasure", strlen("view_treasure")) == 0) return 4;
  if (strncmp(command, "stop_monitor", strlen("stop_monitor")) == 0) return 5;
  if (strncmp(command, "exit", strlen("exit")) == 0) return 6;
  return 7;
}


void handle_sigchld(){
		int status;
    waitpid(monitor_pid, &status, 0);
    monitor_state = 0;
    if (WIFEXITED(status)) {
        int exit_code = WEXITSTATUS(status);
        printf("<<monitor terminated with status: %d\n", exit_code);
        printf("<<monitor is OFF.\n");
        return;
    }
   	printf("bye bye monitor\n");
}

void writeCommand(char* command) {
    if (monitor_state != 2) {
        printf("<<monitor has to be running\n");
        printf("monitor state = %d\n", monitor_state);
        return; // check monitor status
    }

    char trimmed_command[strlen(command) + 1];
    strcpy(trimmed_command, command);
    trimmed_command[strcspn(trimmed_command, "\n")] = 0; // format the command to match desired type

    char *token = strtok(trimmed_command, " ");
    char a1[64], a2[64];
    char final_command[256];
    
    int code;// -1 = incorrect; 0 = list_hunts; 1 = list_treasures; 2 = view_treasure;
    
    if (strcmp(token, "list_hunts") == 0) {
   		code = 0;
    } else if (strcmp(token, "list_treasures") == 0) {
				//list_treasures should have exactly 1 argument
        token = strtok(NULL, " ");
        if (token == NULL) code = -1;
        else{
        	code = 1;
        	strcpy(a1, token);	
        }
    } else if (strcmp(token, "view_treasure") == 0) {
				//view_treasure should have 2 args
        token = strtok(NULL, " ");
        if (token == NULL) code = -1;
        else{
        	strcpy(a1, token);	
        	token = strtok(NULL, " ");
	        if (token == NULL) code = -1;
	        else{
	        	code = 2;
	        	strcpy(a2, token);	
	        }
        }
    } else code = -1;
		
		switch(code){
			case -1:
				printf("<<incorrect format\n");
				return;
			case 0: // ./manager_exec hunts
				snprintf(final_command, sizeof(final_command), "./manager_exec hunts");
				break;
			case 1: // ./manager_exec list hunt_name
				snprintf(final_command, sizeof(final_command), "./manager_exec list %s", a1);
				break;
			case 2: // ./manager_exec view hunt_name treasure_name
				snprintf(final_command, sizeof(final_command), "./manager_exec view %s %s", a1, a2);
				break;
			default:
				printf("<<incorrect format (but weird code value)\n");
				break;
		
		}

		int fd = open("commands.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);

    if (fd == -1) {
        perror("fopen error");
        return;
    }

    if (write(fd, final_command, strlen(final_command)) != strlen(final_command)) {
        perror("write error");
        close(fd);
        return;
    }
    close(fd);

    kill(monitor_pid, SIGUSR1); // send SIGUSR1 to monitor so it knows knows to read command from file
}

int main() {
		compileDependencies();

    char command[64];
    struct sigaction act;
		memset(&act, 0x00, sizeof(struct sigaction));
		
		act.sa_handler = handle_sigchld;
  	if (sigaction(SIGCHLD, &act, NULL) < 0) {
      perror("Process set SIGCHLD");
      exit(-1);
    }
		
    while(1){
    	char* input_result = NULL;

		  do {
		      input_result = fgets(command, sizeof(command), stdin);
		  } while (input_result == NULL && errno == EINTR);
		  
		  command[strcspn(command, "\n")] = 0;
      
		  switch(getCommand(command)){
		  	case 0:
		  		printHelpMessage();
		  		break;
		  	case 1:
		  		startMonitor();
		  		break;
		  	case 2:
		  	case 3:
		  	case 4:
		  		writeCommand(command);
		  		break;
		  	case 5:
		  		stopMonitor();
		  		break;
		  	case 6:
		  		if(!monitor_state){
		  			printf("<<closing program\n");
		  			return 0;
		  		}
		  		printf("<<monitor must be terminated before you exit.\n");
		  		break;
		  	default:
		  	printf("<<unknown command\n");
		  		break;
		  }
    }
    return 0;
}

