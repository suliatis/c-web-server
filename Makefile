CFLAGS = -Wall -Wextra -Wconversion -Wshadow -pedantic

app:
	gcc $(CFLAGS) main.c -o server.o

serve: app
	./server.o
