/*******************************************************************************
 * Name        : sort.c
 * Author      : Conor McGullam
 * Date        : 2/25/2021
 * Description : Uses quicksort to sort a file of either ints, doubles, or
 *               strings.
 * Pledge      : I pledge my honor that I have abided by the Stevens Honor System.
 ******************************************************************************/
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "quicksort.h"

#define MAX_STRLEN     64 // Not including '\0'
#define MAX_ELEMENTS 1024

typedef enum {
    STRING,
    INT,
    DOUBLE
} elem_t;

/**
 * Reads data from filename into an already allocated 2D array of chars.
 * Exits the entire program if the file cannot be opened.
 */
size_t read_data(char *filename, char **data) {
    // Open the file.
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        fprintf(stderr, "Error: Cannot open '%s'. %s.\n", filename,
                strerror(errno));
        free(data);
        exit(EXIT_FAILURE);
    }

    // Read in the data.
    size_t index = 0;
    char str[MAX_STRLEN + 2];
    char *eoln;
    while (fgets(str, MAX_STRLEN + 2, fp) != NULL) {
        eoln = strchr(str, '\n');
        if (eoln == NULL) {
            str[MAX_STRLEN] = '\0';
        } else {
            *eoln = '\0';
        }
        // Ignore blank lines.
        if (strlen(str) != 0) {
            data[index] = (char *)malloc((MAX_STRLEN + 1) * sizeof(char));
            strcpy(data[index++], str);
        }
    }

    // Close the file before returning from the function.
    fclose(fp);

    return index;
}


/**
 * Basic structure of sort.c:
 *
 * Parses args with getopt.
 * Opens input file for reading.
 * Allocates space in a char** for at least MAX_ELEMENTS strings to be stored,
 * where MAX_ELEMENTS is 1024.
 * Reads in the file
 * - For each line, allocates space in each index of the char** to store the
 *   line.
 * Closes the file, after reading in all the lines.
 * Calls quicksort based on type (int, double, string) supplied on the command
 * line.
 * Frees all data.
 * Ensures there are no memory leaks with valgrind. 
 */
int main(int argc, char **argv) {
	int iflag = 0;
	int dflag = 0;
	char *fname = NULL;
	int opt;
	opterr = 0;
	
	if(argc == 1) {
		fprintf(stderr, "Usage: ./sort [-i|-d] filename\n");
		fprintf(stderr, "   -i: Specifies the file contains ints.\n");
		fprintf(stderr, "   -d: Specifies the file contains doubles.\n");
		fprintf(stderr, "   filename: The file to sort.\n");
		fprintf(stderr, "   No flags defaults to sorting strings.\n");
		return EXIT_FAILURE;
	}

	while ((opt = getopt(argc, argv, "id")) != -1) {
		switch (opt) {
			case 'i':
				iflag += 1;
				break;
			case 'd':
				dflag += 1;
				break;
			case '?':
				fprintf(stderr, "Error: Unknown option '-%c' received.\n", optopt);
				fprintf(stderr, "Usage: ./sort [-i|-d] filename\n");
				fprintf(stderr, "   -i: Specifies the file contains ints.\n");
				fprintf(stderr, "   -d: Specifies the file contains doubles.\n");
				fprintf(stderr, "   filename: The file to sort.\n");
				fprintf(stderr, "   No flags defaults to sorting strings.\n");
				return EXIT_FAILURE;
		}
	}
	if(iflag + dflag > 1) {
		fprintf(stderr, "Error: Too many flags specified.\n");
		return EXIT_FAILURE;
	}
	if(argc - iflag - dflag > 2) {
		fprintf(stderr, "Error: Too many files specified.\n");
		return EXIT_FAILURE;
	}
	if(argc - iflag - dflag < 2) {
		fprintf(stderr, "Error: No input file specified.\n");
		return EXIT_FAILURE;
	}
	fname = argv[argc-1];
	char **data = (char **)malloc(MAX_ELEMENTS * (MAX_STRLEN+1));
	size_t ind = read_data(fname, data);
	
	//int input
	if(iflag == 1) {
		int **array = (int **)malloc(MAX_ELEMENTS * (sizeof(int)));
		for(int i = 0; i < ind; ++i) {
			int *temp = (int *)malloc(sizeof(int));
			sscanf(data[i], "%d", temp);
			array[i] = temp;
		}
		
		quicksort((void *)array, ind, sizeof(int), int_cmp);
		for(int i = 0; i < ind; ++i) {
			printf("%d\n", *array[i]);
		}
		
		for(int i = 0; i < ind; ++i) {
			free(array[i]);
			free(data[i]);
		}
		free(array);
		free(data);
	
	//double input
	}else if(dflag == 1) {
		double **array = (double **)malloc(MAX_ELEMENTS * (sizeof(double)));
		for(int i = 0; i < ind; ++i) {
			double *temp = (double *)malloc(sizeof(double));
			sscanf(data[i], "%lf", temp);
			array[i] = temp;
		}

		quicksort((void *)array, ind, sizeof(double), dbl_cmp);
		for(int i = 0; i < ind; ++i) {
			printf("%lf\n", *array[i]);
		}
		
		for(int i = 0; i < ind; ++i) {
			free(array[i]);
			free(data[i]);
		}
		free(array);
		free(data);
	
	//string input (char array)
	}else {
		quicksort((void *)data, ind, MAX_STRLEN+1, str_cmp);
		for(int i = 0; i < ind; ++i) {
			printf("%s\n", data[i]);
		}
		
		for(int i = 0; i < ind; ++i) {
			free(data[i]);
		}
		free(data);
	}
	
    return EXIT_SUCCESS;
}
