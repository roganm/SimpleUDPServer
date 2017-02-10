all: sws.c
	gcc -o sws sws.c -w
clean:
	$(RM) sws
