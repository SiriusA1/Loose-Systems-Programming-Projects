mtsieve: mtsieve.o
	gcc mtsieve.o -o mtsieve -lm -pthread
mtsieve.o: mtsieve.c
	gcc -Wall -Werror -pedantic-errors -c mtsieve.c
clean:
	rm -f *.o mtsieve mtsieve.exe
