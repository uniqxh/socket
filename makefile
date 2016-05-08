CC = gcc -g
epoll:
	$(CC) -o epoll epoll.c

clean:
	rm -f epoll *~
