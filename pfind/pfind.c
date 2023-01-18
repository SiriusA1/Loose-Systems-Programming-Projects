/*******************************************************************************
 * Name        : pfind.c
 * Author      : Conor McGullam
 * Date        : 3/16/2021
 * Description : Finds files with specfic permission strings.
 * Pledge      : I pledge my honor that I have abided by the Stevens Honor System.
 ******************************************************************************/
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>

int find_perms(char *dir_name, int *perm) {
	DIR *dp;
	struct dirent *dent;
	struct stat sb;
	/* open directory so we can traverse it. */
	/* NOTE: opendir returns NULL if there was some error, and sets `errno` appropriately */
	if ((dp = opendir(dir_name)) == NULL) {
		fprintf(stderr, "Error: Cannot open directory '%s'. %s.\n", dir_name, strerror(errno));
		return 1;    
	}
	
	/* We want to iterate until readdir returns NULL. This can mean one of two things:     
	*   1. we have reached the end of the directory.     
	*   2. there was an error.     
	* As such, to distinguish between an error and the end of the directory, we set     
	* errno to 0 before we readdir, then again at the end of every loop. Then     
	* when we exit, we check if errno is 0. If it is not zero, then an error occured      
	* in readdir. */    
	errno = 0;
	strcat(dir_name, "/");
	while ((dent = readdir(dp)) != NULL) {
		if((strcmp(dent->d_name, ".") != 0) && (strcmp(dent->d_name, "..") != 0)) {
			char dir_add[PATH_MAX+1];
			strcpy(dir_add, dir_name);
			strcat(dir_add, dent->d_name);
			/* check if file can be stat-ed */
			if (lstat(dir_add, &sb) < 0) {
				fprintf(stderr, "Error: Cannot stat file '%s'. %s.\n", dir_add, strerror(errno));
				continue;        
			}
		
			/* if directory, then recursive call */
			if (S_ISDIR(sb.st_mode)) {
				char dir_new[PATH_MAX+1];
				strcpy(dir_new, dir_add);
				find_perms(dir_new, perm);        
			}
		
			/* check permission string */
			int new_perms = (sb.st_mode & S_IRWXU)|(sb.st_mode & S_IRWXG)|(sb.st_mode & S_IRWXO);
			int perm_bin[9];
			for(int i = 8; i > -1; --i) {
				perm_bin[i] = new_perms%2;
				new_perms = new_perms / 2;
			}
			int same = 0;
			for(int i = 0; i < 9; ++i) {
				if(perm[i] != perm_bin[i]) {
					same = 1;
					break;
				}
			}
			if(same == 0) {
				printf("%s\n", dir_add);
			}
			//free(perm_bin);
			errno = 0;  
		}			
	}
	closedir(dp);
	return 0;
}

int main(int argc, char **argv) {
	int dflag = 0;
	int pflag = 0;
	int hflag = 0;
	char *diropt;
	char *permopt;
	int opt;
	opterr = 0;
	struct stat dirstatbuf;
	int perm[9];
	
	if(argc == 1) {
		fprintf(stderr, "Usage: ./pfind -d <directory> -p <permissions string> [-h]\n");
		return EXIT_FAILURE;
	}
	
	while ((opt = getopt(argc, argv, "d:p:h")) != -1) {
		switch (opt) {
			case 'h':
				hflag += 1;
				break;
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
	
	if(hflag == 1) {
		fprintf(stderr, "Usage: ./pfind -d <directory> -p <permissions string> [-h]\n");
		return EXIT_SUCCESS;
	}
	if(dflag == 0) {
		fprintf(stderr, "Error: Required argument -d <directory> not found.\n");
		return EXIT_FAILURE;
	}
	if(pflag == 0) {
		fprintf(stderr, "Error: Required argument -p <permissions string> not found.\n");
		return EXIT_FAILURE;
	}
	
	//stat-ing directory
	if(strcmp(diropt, "-p") == 0) {
		fprintf(stderr, "Error: Required argument -d <directory> not found.\n");
		return EXIT_FAILURE;
	}
	if (lstat(diropt, &dirstatbuf) < 0) {
		fprintf(stderr, "Error: Cannot stat '%s'. %s.\n", diropt,
				strerror(errno));
		return EXIT_FAILURE;
	}
	
	//parsing permstring into binary array
	if(strcmp(permopt, "-h") == 0) {
		fprintf(stderr, "Error: Required argument -p <permissions string> not found.\n");
		return EXIT_FAILURE;
	}
	if(strlen(permopt) != 9) {
		fprintf(stderr, "Error: Permissions string '%s' is invalid.\n", permopt);
		return EXIT_FAILURE;
	}else {
		char *temp = permopt;
		for(int i = 0; i < 9; ++i) {
			if(i % 3 == 0) {
				if((*temp != 'r') && (*temp != '-')) {
					fprintf(stderr, "Error: Permissions string '%s' is invalid.\n", permopt);
					return EXIT_FAILURE;
				}
				if(*temp == 'r') {
					perm[i] = 1;
				} else {
					perm[i] = 0;
				}
			}else if(i % 3 == 1) {
				if((*temp != 'w') && (*temp != '-')) {
					fprintf(stderr, "Error: Permissions string '%s' is invalid.\n", permopt);
					return EXIT_FAILURE;
				}
				if(*temp == 'w') {
					perm[i] = 1;
				} else {
					perm[i] = 0;
				}
			}else {
				if((*temp != 'x') && (*temp != '-')) {
					fprintf(stderr, "Error: Permissions string '%s' is invalid.\n", permopt);
					return EXIT_FAILURE;
				}
				if(*temp == 'x') {
					perm[i] = 1;
				} else {
					perm[i] = 0;
				}
			}
			temp = temp+1;
		}
	}
	
	//start reading in dir
	//have to get the rest of the previous directory all the way back to the root
	char curr_path[PATH_MAX+1];
	realpath(diropt, curr_path);
	find_perms(curr_path, perm);
	
	return EXIT_SUCCESS;
}