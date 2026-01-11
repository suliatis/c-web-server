app:
	gcc main.c -o server.o

serve: app
	./server.o
