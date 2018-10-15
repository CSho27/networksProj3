all: proj3.o
	gcc -o proj3 proj3.o

proj3.o: proj3.c
	gcc -Wall -Werror -g -c proj3.c
	
clean:
	rm -f *.o
	rm -f proj3