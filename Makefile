CC=gcc
CFLAGS=-Wall
shell : shell.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm shell