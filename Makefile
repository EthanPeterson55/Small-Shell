CC = gcc
CFLAGS = -Wall -Wextra -std=c11

# Source files
SRCS = shell.c
OBJS = $(SRCS:.c=.o)

# Executable
EXEC = shell

# Default target
all: $(EXEC)

# Compile C files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Link object files to create the executable
$(EXEC): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(EXEC)

# Clean target
clean:
	rm -f $(OBJS) $(EXEC)
