CC = gcc
CFLAGS = -Wall -Wextra -g -pthread

TARGET = app
OBJS = main.o pthreadfuncs.o
OUTPUT = output.log

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

log:
	cat $(OUTPUT)

clean:
	rm -f $(TARGET) $(OBJS) $(OUTPUT)

.PHONY: clean run
