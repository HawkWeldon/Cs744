Cflag = -Wno-implicit-function-declaration
cc = gcc
all: final clean

hu.o : hu.c
	$(cc) $(Cflag) -c hu.c

helloworld.o : ../helloworld/helloworld.c
	$(cc) $(Cflag) -c ../helloworld/helloworld.c

final: hu.o helloworld.o
	$(cc) $(Cflag) -o final hu.o helloworld.o
	chmod +x final

clean:
	rm hu.o helloworld.o