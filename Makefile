LFLAGS = -lm
default: main

main.o: main.c
	gcc -c -g main.c -o main.o $(LFLAGS)

jpeg_header.o: jpeg_header.c
	gcc -c -g jpeg_header.c -o jpeg_header.o $(LFLAGS)

huffman.o: huffman.c
	gcc -c -g huffman.c -o huffman.o $(LFLAGS)

scandata.o: scandata.c
	gcc -c -g scandata.c -o scandata.o $(LFLAGS)

write_to_bmp.o: write_to_bmp.c
	gcc -c -g write_to_bmp.c -o write_to_bmp.o $(LFLAGS)

main: main.o jpeg_header.o huffman.o scandata.o write_to_bmp.o
	gcc -g main.o jpeg_header.o huffman.o scandata.o write_to_bmp.o -o main $(LFLAGS)

clean:
	-rm -rf main
	-rm -rf main.o
	-rm -rf jpeg_header.o
	-rm -rf huffman.o
	-rm -rf scandata.o
	 rm -rf write_to_bmp.o
