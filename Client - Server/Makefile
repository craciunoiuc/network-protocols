
build:
	g++ server.cpp -o server -Wall -Wextra
	g++ subscriber.cpp -o subscriber -Wall -Wextra

subscriber: subscriber.o
	g++ subscriber.o -o subscriber

subscriber.o:
	g++ subscriber.cpp -c

server: server.o
	g++ server.o -o server

server.o:
	g++ server.cpp -c

clean:
	rm -rf *.o subscriber server

