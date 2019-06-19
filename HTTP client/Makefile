CC=gcc
CFLAGS=-Wall -Wextra -O0
FILES=clientHTTP.c requests.c parson.c helpers.c

client: $(FILES)
		$(CC) -o clientHTTP $(FILES) $(CFLAGS)	
run: clientHTTP
		./clientHTTP

clean:
		rm -f *.o clientHTTP

