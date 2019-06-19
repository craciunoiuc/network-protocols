// Copyright Craciunoiu Cezar 324CA
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "./helpers.h"

// Functie preluata din laborator
// Functia afiseaza mesajul de eroare si inchide executia programului.
void error(const char *msg)
{
    perror(msg);
    exit(0);
}

// Functie preluata din laborator
// Functie formateaza adauga o linie la mesaj si il formateaza pentru HTTP
void compute_message(char *message, const char *line)
{
    strcat(message, line);
    strcat(message, "\r\n");
}

// Functie preluata din laborator
// Functia deschide o conexiune(TCP/UDP) la un IP:Port de un anumit tip, punand
// flagurile necesare conexiunii.
int open_connection(char *host_ip, int portno, int ip_type, int socket_type, int flag)
{
    struct sockaddr_in serv_addr;
    int sockfd = socket(ip_type, socket_type, flag);

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = ip_type;
    serv_addr.sin_port = htons(portno);
    inet_aton(host_ip, &serv_addr.sin_addr);
    if (sockfd < 0)
    {
        error("ERROR opening socket");
    }

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        error("ERROR connecting");
    }
    return sockfd;
}

// Functie preluata din laborator
// Functia inchide conexiunea de pe un anumt socket.
void close_connection(int sockfd)
{
    close(sockfd);
}

// Functie preluata din laborator
// Functia scrie pe un socket informatia din mesaj ce urmeaza sa fie trimisa.
void send_to_server(int sockfd, char *message)
{
    int bytes, sent = 0;
    int total = strlen(message);
    do
    {
        bytes = write(sockfd, message + sent, total - sent);
        if (bytes < 0)
        {    
            error("ERROR writing message to socket");
        }
        if (bytes == 0)
        {
            break;
        }
        sent += bytes;
    } while (sent < total);
}

// Functie preluata din laborator
// Functia primeste un mesaj inapoi pe socket si il construieste intr-o
// locatie de memorie noua la care intoarce apoi un pointer.
char *receive_from_server(int sockfd)
{
    char *response = calloc(BUFLEN, sizeof(char));
    int total = BUFLEN;
    int received = 0;
    do
    {
        int bytes = read(sockfd, response + received, total - received);
        if (bytes < 0)
        {
            error("ERROR reading response from socket");
        }
        if (bytes == 0)
        {
            break;
        }
        received += bytes;
    } while (received < total);

    if (received == total)
    {
        error("ERROR storing complete response from socket");
    }
    return response;
}

// Functie preluata din laborator DNS
// Functia este folosita la Etapa 5 pentru a interoga server-ul de DNS.
// Aceasta intoarce o structura de tip ip_port cu primul rezultat intors
// de server-ul de DNS.
ip_port get_ip(const char* name)
{
	int ret;
	struct addrinfo hints, *result, *p;

	memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;

	ret = getaddrinfo(name, NULL, &hints, &result);
    if (ret != 0)
    {
        printf("getaddrinfo: %s\n", gai_strerror(ret));
        exit(1);
    }

	while (result != NULL)
    {
		p = result;
		char ip[100];
		struct sockaddr_in* addr = (struct sockaddr_in*) p->ai_addr;
		if (inet_ntop(p->ai_family, &(addr->sin_addr), ip, sizeof(ip)) != NULL)
        {
            ip_port ret;
            strcpy(ret.ip, ip);
            ret.port = ntohs(addr->sin_port);
	        freeaddrinfo(result);
            return ret;
        }
		result = result->ai_next;
	}
	freeaddrinfo(result);
    ip_port fail;
    fail.port = -1;
    return fail;
}

// Versiune modificata a functiei de receive pentru Etapa 5. Server-ul de vreme
// intoarce rezultatul intr-o singura trimitere, iar functia nemodificata
// asteapta un mesaj de lungime 0 care nu se primeste niciodata.
char *receive_from_weather_server(int sockfd)
{
    char *response = calloc(BUFLEN, sizeof(char));
    int total = BUFLEN;
    int received = 0;
    int bytes = read(sockfd, response + received, total - received);
    if (bytes < 0)
    {
        error("ERROR reading response from socket");
    }
    received += bytes;
    if (received == total)
    {
        error("ERROR storing complete response from socket");
    }
    return response;
}

// Functia primeste un string in care trebuie sa se cauta si scrie in cookies
// toate string-urile care incep de la cookieSearch si se termina la ';'. Adica
// salveaza fiecare cookie in parte din mesaj.
void parseCookies (char *toParse, char **cookies, int *nr_cookies)
{
    const char * cookieSearch = "Set-Cookie: ";
    while(1)
    {
        char *cookieAux = strstr(toParse, cookieSearch);
        if (cookieAux == NULL)
        {
            break;
        }
        cookieAux += strlen(cookieSearch);
        toParse = cookieAux;
        cookies[*nr_cookies] = (char *)calloc(COOKIE_DIM, sizeof(char));
        for (int i = 0; cookieAux[i] != ';'; ++i)
        {
            cookies[*nr_cookies][i] = cookieAux[i];
        }
        (*nr_cookies)++;
    }
}
