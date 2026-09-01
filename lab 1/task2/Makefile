Cflag = -Wno-implicit-function-declaration
CC = gcc
all: make1 make2 run clean

make1 : 
	$(MAKE) -C hellouniverse

make2 :
	$(MAKE) -C helloworld

run:
	@echo "------------------------------------------------"
	@echo "Running final programs"
	./hellouniverse/final
	./helloworld/final 
	@echo "------------------------------------------------"
	
clean:
	rm hellouniverse/final helloworld/final