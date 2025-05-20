#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <libgen.h>

typedef struct {
  char id[16], user_name[32], clue_text[64];
  float x, y;
  int value;
} treasure;

int main() {
    DIR *dir = opendir(".");
    if (!dir) {
        perror("opendir");
        return 1;
    }
    
	char cwd[256];
	if (getcwd(cwd, sizeof(cwd)) != NULL) {
		printf("SCORES FOR HUNT %s\n", basename(cwd));
	}

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type != DT_REG) continue;
    	if(strcmp(entry->d_name, "logged_hunt.txt") == 0)
			continue;
        FILE *f = fopen(entry->d_name, "rb");
        if (!f) continue;

        int total = 0;
        treasure t;
        while (fread(&t, sizeof(treasure), 1, f) == 1) {
            total += t.value;
        }

        fclose(f);
        printf("%s : %d\n", entry->d_name, total);
    }

    closedir(dir);

    return 0;
}

