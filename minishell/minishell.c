/*******************************************************************************
 * Name        : minishell.c
 * Author      : Conor McGullam
 * Date        : 4/14/2021
 * Description : Creates a shell with cd and exit commands as well as forks and signals.
 * Pledge      : I pledge my honor that I have abided by the Stevens Honor System.
 ******************************************************************************/
#include <errno.h>
#include <limits.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define BRIGHTBLUE "\x1b[34;1m"
#define DEFAULT "\x1b[0m"

volatile sig_atomic_t signal_val = 0;

void catch_signal(int sig) {    
	signal_val = sig;
}

int main() {
	struct sigaction action;    
	memset(&action, 0, sizeof(struct sigaction));    
	action.sa_handler = catch_signal;
	if (sigaction(SIGINT, &action, NULL) == -1) {        
		fprintf(stderr, "Error: Cannot register signal handler. %s.\n", strerror(errno));     
		return EXIT_FAILURE;    
	}
	
	//holds current working directory
	char *cwd;
	if((cwd = (char *)malloc(PATH_MAX+1)) == NULL) {
		fprintf(stderr, "Error: malloc() failed. %s.\n", strerror(errno));
		return EXIT_FAILURE;
	}
	if(getcwd(cwd, PATH_MAX+1) == NULL) {
		fprintf(stderr, "Error: Cannot get current working directory. %s.\n", strerror(errno));
		return EXIT_FAILURE;
	}
	
	printf("[%s%s%s]$ ", BRIGHTBLUE, cwd, DEFAULT);
	fflush(stdout);
	
	//holds user input
	char *in;
	if((in = (char *)malloc(PATH_MAX+1)) == NULL) {
		fprintf(stderr, "Error: malloc() failed. %s.\n", strerror(errno));
		return EXIT_FAILURE;
	}
	in = memset(in, 0, PATH_MAX+1);
	
	while(1) {
		if (signal_val == SIGINT) {            
			printf("\n[%s%s%s]$ ", BRIGHTBLUE, cwd, DEFAULT);
			fflush(stdout);        
			signal_val = 0;
		}
		if(read(0, in, PATH_MAX+1) == -1) {
			if(errno != EINTR) {
				fprintf(stderr, "Error: Failed to read from stdin. %s.\n", strerror(errno));
			}
		} else {
			for(int i = 0; i < PATH_MAX+1; i++) {
				if(in[i] == '\n') {
					in[i] = '\0';
				}
			}
			if((strcmp(in, "cd") == 0) || (strncmp(in, "cd ", 3) == 0)) {
				int success = 1;
				if(strlen(in) >= 2) {
					char *parse = in;
					parse = parse+2;
					int quote = 0, sp_til_first = 0, i = 0, nonspace = 0, no_more = 0;
					char *newin;
					if((newin = (char *)malloc(PATH_MAX+1)) == NULL) {
						fprintf(stderr, "Error: malloc() failed. %s.\n", strerror(errno));
						success = 0;
					} else {
						newin = memset(newin, 0, PATH_MAX+1);
					}
					while(*parse != '\0' && success == 1) {
						//have to strip quotes, chdir can handle whitespace
						if(*parse == ' ') {
							if(quote % 2 == 1) { 
								newin[i] = *parse;
								i++;
							} else if(nonspace == 0) {
								sp_til_first++;
							} else if(nonspace == 1 && quote % 2 == 0) {
								no_more = 1;
							}
						} else if(*parse == '"') {
							if(nonspace == 0) {
								nonspace = 1;
							}
							quote += 1;
						} else {
							if(no_more == 1) {
								fprintf(stderr, "Error: Too many arguments to cd.\n");
								success = 0;
								break;
							} else if(nonspace == 0) {
								nonspace = 1;
							}
							newin[i] = *parse;
							i++;
						}
						parse++;
					}
					if(quote % 2 == 1) {
						fprintf(stderr, "Error: Malformed command.\n");
						success = 0;
						free(newin);
					} else if(success == 1) {
						newin[i] = '\0';
						in = memset(in, 0, PATH_MAX+1);
						strcpy(in, newin);
						free(newin);
					}
					if(success == 0) {
						cwd = getcwd(cwd, PATH_MAX+1);
						printf("[%s%s%s]$ ", BRIGHTBLUE, cwd, DEFAULT);
						fflush(stdout);
					}
				}
				if(success == 1) {
					char *temp = in;
					if((strlen(in) == 0) || (*temp == '~')) {
						uid_t user = getuid();
						struct passwd *pass;
						if((pass = getpwuid(user)) == NULL) {
							fprintf(stderr, "Error: Cannot get passwd entry. %s.\n", strerror(errno));
							cwd = getcwd(cwd, PATH_MAX+1);
							printf("[%s%s%s]$ ", BRIGHTBLUE, cwd, DEFAULT);
							fflush(stdout);
						}else {
							cwd = memset(cwd, 0, PATH_MAX+1);
							if(*temp == '~') {
								if(*(temp+1) == '\0') {
									strcpy(cwd, pass->pw_dir);
								} else {
									strcpy(cwd, strcat(pass->pw_dir, temp+1));
								}
							} else {
								strcpy(cwd, pass->pw_dir);
							}
							if(chdir(cwd) == -1) {
								fprintf(stderr, "Error: Cannot change directory to '%s'. %s.\n", cwd, strerror(errno));
								cwd = getcwd(cwd, PATH_MAX+1);
								printf("[%s%s%s]$ ", BRIGHTBLUE, cwd, DEFAULT);
								fflush(stdout);
							} else {
								cwd = getcwd(cwd, PATH_MAX+1);
								printf("[%s%s%s]$ ", BRIGHTBLUE, cwd, DEFAULT);
								fflush(stdout);
							}
						}
					} else {
						if(chdir(temp) == -1) {
							fprintf(stderr, "Error: Cannot change directory to '%s'. %s.\n", temp, strerror(errno));
							printf("[%s%s%s]$ ", BRIGHTBLUE, cwd, DEFAULT);
							fflush(stdout);
						} else {
							getcwd(cwd, PATH_MAX+1);
							printf("[%s%s%s]$ ", BRIGHTBLUE, cwd, DEFAULT);
							fflush(stdout);
						}
					}
				}
			}else if(strcmp(in, "exit") == 0) {
				break;
			}else {
				pid_t pid;
				if ((pid = fork()) < 0) {
					fprintf(stderr, "Error: fork() failed. %s.\n", strerror(errno));
					return EXIT_FAILURE;
				} else if(pid == 0) {
					// We're in the child
					char *temp = in;
					char *arg;
					if((arg = (char *)malloc(PATH_MAX+1)) == NULL) {
						fprintf(stderr, "Error: malloc() failed. %s.\n", strerror(errno));
						exit(EXIT_FAILURE);
					}
					char **in2;
					if((in2 = (char **)malloc(PATH_MAX+1)) == NULL) {
						fprintf(stderr, "Error: malloc() failed. %s.\n", strerror(errno));
						exit(EXIT_FAILURE);
					}
					arg = memset(arg, 0, 4096);
					char **temp2 = in2;
					int j = 0;
					while(*temp != '\0') {
						if(*temp == ' ') {
							j = 0;
							*temp2 = strdup(arg);
							temp2++;
							memset(arg, 0, 4096);
						} else {
							arg[j] = *temp;
							j++;
						}
						temp++;
					}
					*temp2 = strdup(arg);
					temp2++;
					*temp2 = NULL;
					if(execvp(in2[0], in2) == -1) {
						fprintf(stderr, "Error: exec() failed. %s.\n", strerror(errno));
						free(cwd);
						free(in);
						free(arg);
						char **temp3 = in2;
						while(*temp3 != NULL) {
							free(*temp3);
							temp3++;
						}
						free(in2);
						exit(EXIT_FAILURE);
					}
				}
				int status;
				if(wait(&status) == -1) {
					fprintf(stderr, "Error: wait() failed. %s.\n", strerror(errno));
				}
				printf("[%s%s%s]$ ", BRIGHTBLUE, cwd, DEFAULT);
				fflush(stdout);
			}
		}
		in = memset(in, 0, PATH_MAX+1);
	}
	free(cwd);
	free(in);
	return EXIT_SUCCESS;
}