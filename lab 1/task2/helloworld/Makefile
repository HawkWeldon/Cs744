Cflag = -Wno-implicit-function-declaration
CC = gcc
all: final clean

helloworld.o : helloworld.c
	$(CC) $(Cflag) -c helloworld.c

hw.o : hw.c
	$(CC) $(Cflag) -c hw.c

final: helloworld.o hw.o
	$(CC) $(Cflag) -o final helloworld.o hw.o
	chmod +x final

clean:
	rm helloworld.o hw.o