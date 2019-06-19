// Copyright - Craciunoiu Cezar 324CA
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <iostream>
#include "messageTypes.h"
#include "./messageParser.h"

// Dimensiune maxima posibila mesaj
#define BUFLEN 1573

// Macro pentru inchiderea programului - luat din laborator
#define DIE(assertion, call_description)	\
	do {									\
		if (assertion) {					\
			fprintf(stderr, "(%s, %d): ",	\
					__FILE__, __LINE__);	\
			perror(call_description);		\
			exit(EXIT_FAILURE);				\
		}									\
	} while(0)

// Functie pentru cand s-au introdus parametrii gresit
void usage(char *file) {
	fprintf(stderr, "Usage: %s <ID_CLIENT> <IP_SERVER> <PORT_SERVER>\n", file);
	exit(0);
}

int main(int argc, char *argv[]) {
	int sockfd, n, ret;
	struct sockaddr_in serv_addr;
    struct timeval timeout;
	char buffer[BUFLEN];
	int nrRetries = 0;

	if (argc < 3) {
		usage(argv[0]);
	}

	// Se deschide socketul
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "socket");

	// Se completeaza datele serverului
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[3]));
	ret = inet_aton(argv[2], &serv_addr.sin_addr);
	DIE(ret == 0, "inet_aton");

	// Se conteaza clientul la server
	ret = connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
	DIE(ret < 0, "connect");

	// Se trimite id-ul clientului
	ret = send(sockfd, argv[1], strlen(argv[1]), 0);
    DIE(ret < 0, "send");

	fd_set read_fds;
	int fdmax;

	// Se adauga tastatura si socketul ca descriptori
	FD_SET(STDIN_FILENO, &read_fds);
	FD_SET(sockfd, &read_fds);
	fdmax = sockfd;

	// Se seteaza timeout-ul pentru select
    timeout.tv_sec = 5;
    timeout.tv_usec = 100000;

	// Daca serverul a patit ceva, dupa 10 incercari se inchide conexiunea
	while (nrRetries < 10) {
		fd_set tmp_fds = read_fds;
		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, &timeout);
		DIE(ret < 0, "select");
		memset(buffer, 0, BUFLEN);

		// Daca s-a primit informatie de la tastatura, se citeste si se
		// construieste mesajul in functie de comanda data. Daca s-a scris exit
		// se iese din bucla. Pentru subscribe se verifica daca SF e diferit
		// de 0/1.
		if (FD_ISSET(STDIN_FILENO, &tmp_fds)) {
			memset(buffer, 0, sizeof(buffer));
			fgets(buffer, BUFLEN - 1, stdin);
			commTCP msg;
			memset(&msg, 0, sizeof(commTCP));
			if (strncmp(buffer, "exit\n", 5) == 0) {
				strncpy(msg.command, buffer, 9);
                ret = send(sockfd, (char *)&msg, sizeof(commTCP), 0);
                DIE(ret < 0, "sendExit");
				break;
			}

			if (strncmp(buffer, "subscribe ", 10) == 0) {
				strncpy(msg.command, buffer, 9);
				int i = 10;
				for (; buffer[i] != ' '; ++i) {
					msg.topic[i - 10] = buffer[i];
				}
				if (buffer[i + 1] == '1') {
					msg.SF = 1;
				} else {
					if (buffer[i + 1] != '0') {
						printf("Wrong command, SF is 0/1\n");
						continue;
					}
				}
                ret = send(sockfd, (char *)&msg, sizeof(commTCP), 0);
                DIE(ret < 0, "sendSub");
				printf("Subscribed topic\n");
				continue;
			}

			if (strncmp(buffer, "unsubscribe ", 12) == 0) {
				strncpy(msg.command, buffer, 11);
				uint32_t i = strlen("unsubscribe");
				for (; i < sizeof(msg.topic) + 11 || buffer[i] != '\0'; ++i) {
					if (buffer[i] == '\n') {
						buffer[i] = '\0';
					}
					msg.topic[i - 12] = buffer[i];
				}
                ret = send(sockfd, (char *)&msg, sizeof(commTCP), 0);
                DIE(ret < 0, "unsub");
				printf("Unsubscribed topic\n");
				continue;
			}
		}

		// Daca s-au primit date de la server se verifica daca:
		// 1. Serverul se inchide
		// 2. Serverul a refuzat conexiunea
		// 3. Serverul nu mai raspunde
        if (FD_ISSET(sockfd, &tmp_fds)) {
			memset(buffer, 0, sizeof(buffer));
            n = recv(sockfd, buffer, sizeof(buffer), 0);
            DIE(n < 0, "recv");
			if (n == 0) {
				nrRetries++;
				continue;
			}
			if (strncmp(buffer, "__reject__", 10) == 0) {
				printf("Id too long or already taken, try again\n");
				break;
			}
			if (strncmp(buffer, "__shutdown__", 12) == 0) {
				break;
			}

			// Se desparte mesajul daca a fost concatenat si apoi se afiseaza
			// in functie de tipul de date primit. 4 inseamna ca a este un rest
			// de mesaj venit inainte.
			for (auto& elem : MessageParser::parse(std::string(buffer, n), n)) {
				switch(elem.first.tip_date) {
					case 0: {
						INT aux = MessageParser::parseINT(elem.second);
						std::cout << inet_ntoa(elem.first.address.sin_addr) << ":" 
								  << ntohs(elem.first.address.sin_port) << " - " 
								  << elem.first.topic << " - "
								  << "INT - " 
								  << ((aux.sign == 1)?"-":" ") << aux.data << std::endl;
						break;
					}
					case 1: {
						SHORT_REAL aux = MessageParser::parseSHORT_REAL(elem.second);
						std::cout << inet_ntoa(elem.first.address.sin_addr) << ":" 
								  << ntohs(elem.first.address.sin_port) << " - " 
								  << elem.first.topic << " - "
								  << "SHORT_REAL - ";
						printf("%.2f\n", aux.data / 100.0);
						break;
					}
					case 2: {
						FLOAT aux = MessageParser::parseFLOAT(elem.second);
						std::cout << inet_ntoa(elem.first.address.sin_addr) << ":" 
								  << ntohs(elem.first.address.sin_port) << " - " 
								  << elem.first.topic << " - "
								  << "FLOAT - " 
								  << ((aux.sign == 1)?"-":" ");
						printf("%.*f\n", aux.power, 
								aux.data / MessageParser::raisePower(aux.power));
						break;
					}
					case 3: {
						std::cout << inet_ntoa(elem.first.address.sin_addr) << ":" 
								  << ntohs(elem.first.address.sin_port) << " - " 
								  << elem.first.topic << " - "
								  << "STRING - " 
								  << MessageParser::parseSTRING(elem.second).str;
						if (elem.first.complete) {
							std::cout << std::endl;
						}
						break;
					}
					// Date fara sens -> restul dintr-un mesaj mai vechi
					case 4: {
						std::cout << elem.second << std::endl;
					}
					default: {
						break;
					}
				}
			}
			nrRetries = 0;
		}
	}
	close(sockfd);
	return 0;
}
