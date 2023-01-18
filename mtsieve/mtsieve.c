/*******************************************************************************
 * Name        : mtsieve.c
 * Author      : Conor McGullam
 * Date        : 4/23/2021
 * Description : Multithreaded Sieve of Eratosthenes
 * Pledge      : I pledge my honor that I have abided by the Stevens Honor System.
 ******************************************************************************/
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sysinfo.h>
#include <unistd.h>

volatile int total_count = 0;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
int *lowprimes;
int lowprimes_len = 0;

typedef struct arg_struct {
	int start;
	int end;
} thread_args;

void* find_primes(void *ptr) {
	struct arg_struct *args = (struct arg_struct *)ptr;
	int highprimes_len = (args->end) - (args->start) + 1;
	int *highprimes = (int *)malloc(sizeof(int) * highprimes_len);
	highprimes = memset(highprimes, 0, (sizeof(int) * highprimes_len));
	for(int p = 2; p < lowprimes_len; ++p) {
		if(lowprimes[p] == 0) {
			int i = ((int)ceil((double)(args->start) / p) * p) - (args->start);
			if(args->start <= p) {
				i = i + p;
			}
			while(i < highprimes_len) {
				highprimes[i] = 1;
				i += p;
			}
		}
	}
	for(int i = 0; i < highprimes_len; i++) {
		if(highprimes[i] == 0) {
			int temp = i + args->start;
			int count = 0;
			while(temp > 0) {
				if(temp % 10 == 3) {
					count++;
				}
				if(count >= 2) {
					int retval = 0;
					if((retval = pthread_mutex_lock(&lock)) != 0) {
						fprintf(stderr, "Error: Cannot lock mutex. %s", strerror(retval));
					}
					total_count++;
					if((retval = pthread_mutex_unlock(&lock)) != 0) {
						fprintf(stderr, "Error: Cannot unlock mutex. %s", strerror(retval));
					}
					break;
				}
				temp = temp / 10;
			}
		}
	}
	free(highprimes);
	pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
	int sflag = 0, eflag = 0, tflag = 0;
	int opt;
	int lowprime;
	int highprime; 
	int numthreads;
	char *endptr;
	opterr = 0;
	
	if(argc < 4) {
		fprintf(stderr, "Usage: ./mtsieve -s <starting value> -e <ending value> -t <num threads>\n");
		return EXIT_FAILURE;
	}
	
	while ((opt = getopt(argc, argv, "s:e:t:")) != -1) {
		switch (opt) {
			case 's':
				sflag += 1;
				lowprime = strtol(optarg, &endptr, 10);
				if(*endptr != '\0') {
					fprintf(stderr, "Error: Invalid input '%s' received for parameter '-%c'.\n", optarg,
					's');
					return EXIT_FAILURE;
				} else if((lowprime == LONG_MAX || lowprime == LONG_MIN) && errno == ERANGE) {
					fprintf(stderr, "Error: Integer overflow for parameter '-%c'.\n", 's');
					return EXIT_FAILURE;
				} else if(lowprime > 2147483647) {
					fprintf(stderr, "Error: Integer overflow for parameter '-%c'.\n", 's');
					return EXIT_FAILURE;
				}
				break;
			case 'e':
				eflag += 1;
				highprime = strtol(optarg, &endptr, 10);
				if(*endptr != '\0') {
					fprintf(stderr, "Error: Invalid input '%s' received for parameter '-%c'.\n", optarg,
					'e');
					return EXIT_FAILURE;
				} else if((highprime == LONG_MAX || highprime == LONG_MIN) && errno == ERANGE) {
					fprintf(stderr, "Error: Integer overflow for parameter '-%c'.\n", 'e');
					return EXIT_FAILURE;
				} else if(highprime > 2147483647) {
					fprintf(stderr, "Error: Integer overflow for parameter '-%c'.\n", 'e');
					return EXIT_FAILURE;
				}
				break;
			case 't':
				tflag += 1;
				numthreads = strtol(optarg, &endptr, 10);
				if(*endptr != '\0') {
					fprintf(stderr, "Error: Invalid input '%s' received for parameter '-%c'.\n", optarg,
					't');
					return EXIT_FAILURE;
				} else if((numthreads == LONG_MAX || numthreads == LONG_MIN) && errno == ERANGE) {
					fprintf(stderr, "Error: Integer overflow for parameter '-%c'.\n", 't');
					return EXIT_FAILURE;
				} else if(numthreads > 2147483647) {
					fprintf(stderr, "Error: Integer overflow for parameter '-%c'.\n", 't');
					return EXIT_FAILURE;
				}
				break;
			case '?':
				if (optopt == 'e' || optopt == 's' || optopt == 't') {
					fprintf(stderr, "Error: Option -%c requires an argument.\n", optopt);
				} else if (isprint(optopt)) {
					fprintf(stderr, "Error: Unknown option '-%c'.\n", optopt);
				} else {
					fprintf(stderr, "Error: Unknown option character '\\x%x'.\n",
					optopt);
				}
				return EXIT_FAILURE;
		}
	}
	for (int index = optind; index < argc; index++) {
		fprintf(stderr, "Error: Non-option argument '%s' supplied.\n", argv[index]);
		return EXIT_FAILURE;
	}
	//s errors
	if(sflag < 1) {
		fprintf(stderr, "Error: Required argument <starting value> is missing.\n");
		return EXIT_FAILURE;
	} else if(lowprime < 2) {
		fprintf(stderr, "Error: Starting value must be >= 2.\n");
		return EXIT_FAILURE;
	}
	//e errors
	if(eflag < 1) {
		fprintf(stderr, "Error: Required argument <ending value> is missing.\n");
		return EXIT_FAILURE;
	} else if(highprime < 2) {
		fprintf(stderr, "Error: Ending value must be >= 2.\n");
		return EXIT_FAILURE;
	} else if(highprime < lowprime) {
		fprintf(stderr, "Error: Ending value must be >= starting value.\n");
		return EXIT_FAILURE;
	}
	//t errors
	if(tflag < 1) {
		fprintf(stderr, "Error: Required argument <num threads> is missing.\n");
		return EXIT_FAILURE;
	} else if(numthreads < 1) {
		fprintf(stderr, "Error: Number of threads cannot be less than 1.\n");
		return EXIT_FAILURE;
	} else if(numthreads > 2*get_nprocs()) {
		fprintf(stderr, "Error: Number of threads cannot exceed twice the number of processors(%d).\n",
		get_nprocs());
		return EXIT_FAILURE;
	}
	
	//finding low primes
	int lowlim = sqrt(highprime);
	lowprimes = (int *)malloc(sizeof(int) * (lowlim+1));
	lowprimes_len = lowlim+1;
	lowprimes = memset(lowprimes, 0, (lowlim+1)*sizeof(int));
	lowprimes[0] = 1;
	lowprimes[1] = 1;
	for(int i = 2; i < lowlim+1; ++i) {
		if(lowprimes[i] == 0) {
			if(i*i < lowlim+1) {
				int j = i*i;
				while(j < lowlim+1) {
					lowprimes[j] = 1;
					j += i;
				}
			}
		}
	}
	
	//setting thread args
	int nums = highprime - lowprime + 1;
	if(numthreads > nums) {
		numthreads = nums;
	}
	pthread_t threads[numthreads];
	struct arg_struct thread_args[numthreads];
	int rem = nums % numthreads;
	int temp = lowprime;
	//creating threads
	for(int i = 0; i < numthreads; ++i) {
		thread_args[i].start = temp;
		temp += (nums / numthreads)-1;
		if(i < rem) {
			temp++;
		}
		thread_args[i].end = temp;
		int retval;
		if ((retval = pthread_create(&threads[i], NULL, find_primes, (void *)&thread_args[i])) != 0) {
			fprintf(stderr, "Error: Cannot create producer thread. %s.\n", strerror(retval));
			return EXIT_FAILURE;
		}
		temp++;
	}
	
	//wait for threads
	int retval;
	for (int i = 0; i < numthreads; i++) {
        if((retval = pthread_join(threads[i], NULL)) != 0) {
			fprintf(stderr, "Error: cannot join thread %d. %s", i, strerror(retval));
		}
    }
	printf("%d segments:\n", numthreads);
	for(int i = 0; i < numthreads; ++i) {
		printf("   [%d, %d]\n", thread_args[i].start, thread_args[i].end);
	}
	printf("Total primes between %d and %d with two or more '3' digits: %d\n", lowprime,
	highprime, total_count);
	free(lowprimes);
	return EXIT_SUCCESS;
}