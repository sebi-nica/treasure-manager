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

void handle_sigusr1(){
	FILE *f = fopen("commands.txt", "r");
    if (!f) {
        perror("fopen");
        return;
    }

    char command[256];
    if (fgets(command, sizeof(command), f) != NULL) {
        size_t len = strlen(command);
        if (len > 0 && command[len - 1] == '\n')
            command[len - 1] = '\0';

        system(command);
    }

    fclose(f);
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
