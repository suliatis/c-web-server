.PHONY = build, app, run, clean

CC = gcc
CFLAGS = -Wall -Wextra -Wconversion -Wshadow -pedantic -Iinclude -MMD

SRCS = $(wildcard src/*.c)
OBJS = $(SRCS:src/%.c=build/%.o)

build:
	mkdir -p build

build/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

app: build $(OBJS)
	$(CC) $(OBJS) -o build/app

run: app
	./build/app

clean:
	rm -rf build

-include $(OBJS:.o=.d)
