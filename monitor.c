#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <ctype.h>
#include <stdbool.h>

int cancel_termination = 0;

void handle_sigterm(){
  for (int i = 5; i > 0; i--) {
      sleep(1);
      if(cancel_termination) {
      	cancel_termination = 0;
      	return;
      }
  }
  exit(0);
}

void handle_sigusr2(){
	cancel_termination = 1;
}

void handle_sigusr1() { // updated to call treasure_manager with execl instead of system
    FILE *f = fopen("commands.txt", "r");
    if (!f) {
        perror("fopen");
        return;
    }

    char command_line[256];
    if (fgets(command_line, sizeof(command_line), f) == NULL) {
        fclose(f);
        return;
    }
    fclose(f);

    size_t len = strlen(command_line);
    if (len > 0 && command_line[len - 1] == '\n')
        command_line[len - 1] = '\0';

    char *args[20];
    int i = 0;
    char *token = strtok(command_line, " "); // split command into arguments for execvp
    while (token && i < 19) {
        args[i++] = token;
        token = strtok(NULL, " ");
    }
    args[i] = NULL;

    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        return;
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        return;
    }

    if (pid == 0) { // child
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);

        execvp(args[0], args);
        perror("execvp");
        exit(1);
    } else { // parent
        close(pipefd[1]);

        char buffer[1024];
        ssize_t n;
        while ((n = read(pipefd[0], buffer, sizeof(buffer)-1)) > 0) {
            buffer[n] = '\0';
            printf("%s", buffer); // treasure_hub redirects this through its pipe
            fflush(stdout);
        }

        close(pipefd[0]);
        waitpid(pid, NULL, 0);
    }
}



int main(void){
	struct sigaction act;
  memset(&act, 0x00, sizeof(struct sigaction));
	act.sa_handler = handle_sigterm;
  if (sigaction(SIGTERM, &act, NULL) < 0)
    {
      perror("Process set SIGTERM");
      exit(-1);
    }
  act.sa_handler = handle_sigusr2;
  if (sigaction(SIGUSR2, &act, NULL) < 0)
    {
      perror("Process set SIGUSR2");
      exit(-1);
    }
  act.sa_handler = handle_sigusr1;
  if (sigaction(SIGUSR1, &act, NULL) < 0)
    {
      perror("Process set SIGUSR1");
      exit(-1);
    }
    while (1) pause();
	return 0;
}
