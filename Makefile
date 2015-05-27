LFLAGS =
default: main

main.o: main.c
	gcc -c -g main.c -o main.o $(LFLAGS)

main: main.o
	gcc -g main.o -o main $(LFLAGS)

clean:
	-rm -rf main
	-rm -rf main.o
