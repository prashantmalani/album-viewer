LFLAGS =
default: main

main.o: main.c
	gcc -c -g main.c -o main.o $(LFLAGS)

huffman.o: huffman.c
	gcc -c -g huffman.c -o huffman.o $(LFLAGS)

main: main.o huffman.o
	gcc -g main.o huffman.o -o main $(LFLAGS)

clean:
	-rm -rf main
	-rm -rf main.o
	-rm -rf huffman.o
