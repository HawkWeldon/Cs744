# write details to make the cflask program

CC = gcc
CFLAGS = -w -O2 -pthread

TARGET = bin/cflask

SRCS = cflask.c functions.c http-parser.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	mkdir -p bin
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS)
	rm -f $(TARGET)

.PHONY: all clean