minishell: minishell.o
	gcc minishell.o -o minishell
minishell.o: minishell.c
	gcc -Wall -Werror -pedantic-errors -c -g minishell.c
clean:
	rm -f *.o minishell minishell.exe
