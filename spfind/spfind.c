/*******************************************************************************
 * Name        : spfind.c
 * Author      : Conor McGullam
 * Date        : 3/31/2021
 * Description : Uses forks and pipes to find files with specfic permission strings and sorts them.
 * Pledge      : I pledge my honor that I have abided by the Stevens Honor System.
 ******************************************************************************/
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
	int dflag = 0;
	int pflag = 0;
	char *diropt;
	char *permopt;
	int opt;
	opterr = 0;
	
	if (argc == 1) {
        fprintf(stderr, "Usage: ./spfind -d <directory> -p <permissions string> [-h]\n");
        return EXIT_FAILURE;
    }
	
	while ((opt = getopt(argc, argv, "d:p:h")) != -1) {
		switch (opt) {
			case 'h':
				fprintf(stderr, "Usage: ./pfind -d <directory> -p <permissions string> [-h]\n");
				return EXIT_SUCCESS;
			case 'd':
				dflag += 1;
				diropt = optarg;
				break;
			case 'p':
				pflag += 1;
				permopt = optarg;
				break;
			case '?':
				if((optopt == 'd') && (optarg == NULL)) {
					fprintf(stderr, "Error: Required argument -d <directory> not found.\n");
					return EXIT_FAILURE;
				}
				if((optopt == 'p') && (optarg == NULL)) {
					fprintf(stderr, "Error: Required argument -p <permissions string> not found.\n");
					return EXIT_FAILURE;
				}
				fprintf(stderr, "Error: Unknown option '-%c' received.\n", optopt);
				return EXIT_FAILURE;
		}
	}
	
	if(dflag == 0) {
		fprintf(stderr, "Error: Required argument -d <directory> not found.\n");
		return EXIT_FAILURE;
	}
	if(pflag == 0) {
		fprintf(stderr, "Error: Required argument -p <permissions string> not found.\n");
		return EXIT_FAILURE;
	}
	
    int pfind_to_sort[2], sort_to_parent[2];
    if (pipe(pfind_to_sort) < 0) {
        fprintf(stderr, "Error: Cannot create pipe pfind_to_sort. %s.\n",
                strerror(errno));
        return EXIT_FAILURE;
    }
    if (pipe(sort_to_parent) < 0) {
        fprintf(stderr, "Error: Cannot create pipe sort_to_parent. %s.\n",
                strerror(errno));
        return EXIT_FAILURE;
    }
	
    pid_t pid[2];
    if ((pid[0] = fork()) == 0) {
        // pfind
        close(pfind_to_sort[0]);
        dup2(pfind_to_sort[1], STDOUT_FILENO);
		
        // Close all unrelated file descriptors.
		close(sort_to_parent[0]);
        close(sort_to_parent[1]);
		
        if (execl("./pfind", "./pfind", "-d", diropt, "-p", permopt, NULL) < 0) {
            fprintf(stderr, "Error: pfind failed. %s.\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
		exit(EXIT_SUCCESS);
    }
	
    if ((pid[1] = fork()) == 0) {
        // sort
        close(pfind_to_sort[1]);
        dup2(pfind_to_sort[0], STDIN_FILENO);
        close(sort_to_parent[0]);
        dup2(sort_to_parent[1], STDOUT_FILENO);

		if (execl("/usr/bin/sort", "/usr/bin/sort", STDIN_FILENO, NULL) < 0) {
            fprintf(stderr, "Error: sort failed. %s.\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
		exit(EXIT_SUCCESS);
    }
	
    dup2(sort_to_parent[0], STDIN_FILENO);
    // Close all unrelated file descriptors.
    close(pfind_to_sort[0]);
    close(pfind_to_sort[1]);
	close(sort_to_parent[1]);
    char *buffer;
	if((buffer = (char *)malloc(1024)) == NULL) {
		fprintf(stderr, "Error: malloc failed.\n");        
		close(sort_to_parent[0]);           
		return EXIT_FAILURE; 
	}
	
	ssize_t count;
	int matches = 0;
    while ((count = read(sort_to_parent[0], buffer, 1024)) > 0) {
		char *buf = buffer;
        if (count == -1) {
            if (errno == EINTR) {
                continue;
            } else {
                perror("read()");
                exit(EXIT_FAILURE);
            }
        } else if (count == 0) {
            break;
        } else {
			while(count > 0) {
				write(STDOUT_FILENO, buf, 1);
				if(*buf == '\n') {
					matches++;
				}
				count--;
				buf++;
			}
        }
    }
	int status;
	for(int i = 2; i > 0; --i) {
		wait(&status);
		if(WEXITSTATUS(status) != 0) {
			return EXIT_FAILURE;
		}
	}
	printf("Total matches: %d\n", matches);
    close(sort_to_parent[0]);
	free(buffer);
    return EXIT_SUCCESS;
}
