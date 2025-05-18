#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>


pid_t monitor_pid = -1; // monitor process ID
int monitor_state = 0; // 0 = off 1 = stopping 2 = on

int pipefd[2]; // pipefd[0] = read end, pipefd[1] = write end
// this is the pipe between this and the monitor

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
	syscall_return = system("gcc score_calc.c -o score_calc_exec");
	if (syscall_return != 0) {
		  fprintf(stderr, "ERROR: failed to compile score_calc.c\n");
		  exit(EXIT_FAILURE);
	}
}


void printHelpMessage(){
  printf("help message\n	start_monitor - starts the monitor process\n	list_hunts - [available only when monitor is on] - lists all hunts and number of treasures in each hunt\n	");
  printf("list_treasures <hunt_id> - [available only when monitor is on] - lists all treasures in a hunt\n	view_treasure <hunt_id> <treasure_id> - [available only when monitor is on] - view a single treasure\n	");
  printf("stop_monitor - stops the monitor process\n	calculate_scores - shows scores for every user in every hunt DO NOT USE THIS AFTER THE MONITOR HAS BEEN STARTED - BUG TO BE FIXED\n	exit - exits the program. doesn't work if the monitor is still running\n");
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
		    dup2(pipefd[1], STDOUT_FILENO); // redirect stdout to pipe input
		    for (int fd = 3; fd < 1024; ++fd)
    			if (fd != STDOUT_FILENO)
        			close(fd);

		    execl("./monitor_exec", "monitor_exec", NULL);

		    // execl failed if the program ends up here
		    perror("execl");
		    exit(EXIT_FAILURE);
			break;
		default: // parent
		    close(pipefd[1]); // Close input
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
  if (strncmp(command, "calculate_scores", strlen("calculate_scores")) == 0) return 6;
  if (strncmp(command, "exit", strlen("exit")) == 0) return 7;
  return 8;
}


void handle_sigchld(){
	if(monitor_pid == -1) return;
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

void readFromPipe(int fd) {
    char buffer[1024];
    ssize_t bytesRead;

    while (1) {
        fd_set fds;
        struct timeval tv;

        FD_ZERO(&fds);
        FD_SET(fd, &fds);

        tv.tv_sec = 0;
        tv.tv_usec = 100000; // 0.1s

        int ret = select(fd + 1, &fds, NULL, NULL, &tv);
        if (ret == 0) break;       // timeout: stop reading
        if (ret < 0) {             // error
            perror("select");
            break;
        }

        bytesRead = read(fd, buffer, sizeof(buffer) - 1);
        if (bytesRead <= 0) break; // EOF or error

        buffer[bytesRead] = '\0';
        printf("%s", buffer);
    }
}




void calculateScores() {
    DIR *dir = opendir(".");
    if (!dir) {
        perror("opendir");
        return;
    }
	
	int calc_pipe[512][2], i = -1;
	
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type != DT_DIR) continue;

		if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0 || strcmp(entry->d_name, "logs") == 0 || strcmp(entry->d_name, ".git") == 0)
			continue;
		
		i++;
		if (pipe(calc_pipe[i]) == -1) continue;
		
		
		fcntl(calc_pipe[i][1], F_SETFD, FD_CLOEXEC);

		pid_t pid = fork();
		if (pid == 0) {
			usleep(10000);
		    close(calc_pipe[i][0]);
		    dup2(calc_pipe[i][1], STDOUT_FILENO);
		    close(calc_pipe[i][1]);
		    chdir(entry->d_name);
		    execlp("../score_calc_exec", "../score_calc_exec", NULL);
		    exit(1);
		} else {
		    close(calc_pipe[i][1]);
		    readFromPipe(calc_pipe[i][0]); 
		    close(calc_pipe[i][0]);
		    waitpid(pid, NULL, 0);
		}
	}

    closedir(dir);
}





int main() {
	compileDependencies();
	pipe(pipefd);
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
		  		usleep(10000);
		  		readFromPipe(pipefd[0]);
		  		break;
		  	case 5:
		  		stopMonitor();
		  		break;
		  	case 6:
		  		calculateScores();
		  		break;
		  	case 7:
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

