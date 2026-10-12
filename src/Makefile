CC = gcc
CFLAGS = -Wall -Wextra -g
TARGET = mydb.exe

SRCS = main.c db_manager.c table_manager.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

%.o: %.c db_manager.h table_manager.h
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	.\$(TARGET)

clean:
	del /Q *.o $(TARGET) 2>nul

.PHONY: all run clean