// Copyright - Craciunoiu Cezar 324CA
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include "./messageTypes.h"
#include "./topic.h"
#include <iostream>
#include <map>
#include <vector>
#include <ctime>

// Daca sunt mai mult de 1 milion clienti probabil mai este nevoie de un server
#define MAX_CLIENTS 1000000

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

// Functie eroare input
void usage(char *file) {
	fprintf(stderr, "Usage: %s <PORT_DORIT>\n", file);
	exit(0);
}

// Verifica daca o valoare din map e unica
bool uniqueValue(std::map<int, std::pair<std::string, bool>>& assocs, std::string id) {
    for (auto& elem : assocs) {
        if (elem.second.first.compare(id) == 0) {
            return false;
        }
    }
    return true;
}

int main(int argc, char *argv[]) {
	int sockfdTCP, newsockfdTCP, portno, sockfdUDP;
	char buffer[MSG_SIZE + 100];
    std::map<std::string, Topic*> topics;
    std::map<int, std::pair<std::string, bool>>  clients;
	struct sockaddr_in serv_addr, cli_addr;
    struct timeval timeout;
	int n, i, ret;
	socklen_t clilen = sizeof(cli_addr);
	fd_set read_fds;
	fd_set tmp_fds;
	int fdmax;
    int yes;

    // Verificare pornire corecta
	if (argc < 2) {
		usage(argv[0]);
	}

    // Se goleste multimea de descriptori de citire
    // si multimea temporara
    FD_ZERO(&read_fds);
    FD_ZERO(&tmp_fds);
    FD_SET(STDIN_FILENO, &read_fds);

    // Se initializeaza socketii pentru TCP si UDP
    sockfdUDP = socket(AF_INET, SOCK_DGRAM, 0);
    sockfdTCP = socket(AF_INET, SOCK_STREAM, 0);
    DIE(sockfdTCP < 0 || sockfdUDP < 0, "socket");

    // Se initializeaza portul serverului
    portno = atoi(argv[1]);
    DIE(portno == 0, "atoi");

    // Se retin datele serverului
    memset((char *) &serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(portno);
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    // Se dezactiveaza algoritmul Neagle
    ret = setsockopt(sockfdUDP, SOL_SOCKET, TCP_NODELAY, &yes, sizeof(int));
    DIE(ret < 0, "setsockopt");

    // Se face bind pentru cele 2 porturi
    ret = bind(sockfdTCP, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr));
    DIE(ret < 0, "bindTCP");
    ret = bind(sockfdUDP, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr));
    DIE(ret < 0, "bindUDP");

    // I se spune serverului sa asculte pe portul pentru TCP
    ret = listen(sockfdTCP, MAX_CLIENTS);
    DIE(ret < 0, "listen");

    // Se adauga cei doi socketi in multimea de descriptori
    fdmax = std::max(sockfdTCP, sockfdUDP);
    FD_SET(sockfdTCP, &read_fds);
    FD_SET(sockfdUDP, &read_fds);

    // Se initializeaza timeout-ul pentru select
    timeout.tv_sec = 1;
    timeout.tv_usec = 100000;

	while (true) { 
		tmp_fds = read_fds;
		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, &timeout);
		DIE(ret < 0, "select");

        // Se trece prin fiecare descriptor si se verifica daca este setat
		for (i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &tmp_fds)) {

                // Daca s-au primit date pe socketul UDP
                if (i == sockfdUDP && ret > 0) {
                    commUDP msg;
                    memset(&msg, 0, sizeof(msg));
                    if (ret > 0) {
                        ret = recvfrom(sockfdUDP, (char *)&msg, sizeof(commUDP), 
                            0, (struct sockaddr *)&cli_addr, &clilen);
                    } else {
                        cli_addr.sin_port = -1;
                    }
                    std::string topicAux;

                    // Daca topicul primit are fix 50 de caractere se mai
                    // adauga un null la finalul stringului
                    if (msg.topic[49] == 0) {
                        topicAux = std::string(msg.topic);
                    } else {
                        topicAux = std::string(msg.topic, TOPIC_SIZE).append("\0");
                    }

                    // Se construieste mesajul pentru a fi trimis catre TCP
                    toTCP aux;
                    memset(&aux, 0 , sizeof(toTCP));
                    memcpy(aux.continut, msg.continut, MSG_SIZE);
                    aux.tip_date = msg.tip_date;
                    switch(aux.tip_date) {
                        case 0:{
                            aux.size = sizeof(INT);
                            break;
                        }
                        case 1:{
                            aux.size = sizeof(SHORT_REAL);
                            break;
                        }
                        case 2:{
                            aux.size = sizeof(FLOAT);
                            break;
                        }
                        case 3:{
                            // Se verifica daca mesajul are fix 1500 caractere
                            // sau e mai mic pentru a pune dimensiunea
                            if (aux.continut[MSG_SIZE - 1] == 0) {
                                aux.size = strlen(aux.continut);
                            } else {
                                aux.size = MSG_SIZE;
                            }
                            break;
                        }
                        default: break;
                    }
                    std::string continutAux(msg.continut, aux.size);
                    if (aux.size == MSG_SIZE) {
                        continutAux.append("\0");
                    }
                    aux.address = cli_addr;
                    aux.size += sizeof(struct sockaddr_in);
                    memcpy(aux.topic, topicAux.c_str(), topicAux.length());

                    // Daca topicul unde s-a trimis exista, se trece prin
                    // fiecare abonat si se trimite mesajul daca este online.
                    // Daca trimiterea a esuat inseamna ca acel client s-a
                    // deconectat fara sa spuna si este trecut ca offline.
                    // Daca topicul nu exista este creat.
                    if (topics.count(topicAux) > 0) {
                        topics.at(topicAux)->addMsg(continutAux, msg.tip_date, cli_addr);
                        for (auto& client : *(*topics.find(topicAux)).second->getClients()) {
                            if (client.second.online) {
                                ret = send(client.second.sockfd, &aux, aux.size + 
                                            TOTCP_METADATA + TOPIC_EXTENDED + 1, MSG_NOSIGNAL);
                                if (ret < 0) {
                                    topics.at(topicAux)->setStatusClient(client.first, false);
                                    clients.erase(client.second.sockfd);
                                    clients.insert({client.second.sockfd, {client.first, false}});
                                    ret = 0;
                                    errno = 0;
                                    continue;
                                }
                                topics.at(topicAux)->incrementClient(client.first);
                            }
                        }
                    } else {
                        topics.insert({topicAux, 
                                    new Topic(topicAux, continutAux, msg.tip_date, cli_addr)});

                    }
                    continue;
                }

                // Daca s-au primit date de la tastatura, se verifica daca
                // se doreste inchiderea serverului. Daca da, se elibereaza
                // memoria si se trimite la fiecare client un anunt ca
                // serverul se inchide.
                if (i == STDIN_FILENO && ret > 0) {
                    memset(buffer, 0, sizeof(buffer));
                    fgets(buffer, 100, stdin);
                    if (strncmp(buffer, "exit\n", 5) == 0) {
                        for (auto& elem : topics) {
                            delete elem.second;
                        }
                        int size = strlen("__shutdown__");
                        char closing[size];
                        memcpy(closing, "__shutdown__", size);
                        for (auto& client : clients) {
                            if (client.second.second) {
                                ret = send(client.first, closing, size, MSG_NOSIGNAL);
                                DIE(ret < 0, "send");
                            }
                        }
                        close(sockfdTCP);
                        close(sockfdUDP);
                        return 0;
                    }
                    continue;
                }

                // Daca se intra pe aceasta ramura inseamna ca a venit o cerere
                // de conexiune noua si serverul o accepta.
				if (i == sockfdTCP) {
					newsockfdTCP = accept(sockfdTCP, (struct sockaddr *) &cli_addr, &clilen);
					DIE(newsockfdTCP < 0, "accept");

					// Se adauga asemanator socketul in multimea descriptorilor
					FD_SET(newsockfdTCP, &read_fds);
					if (newsockfdTCP > fdmax) { 
						fdmax = newsockfdTCP;
					}

                    // Se primesc date de la client precum ID-ul sau
                    ret = recv(newsockfdTCP, buffer, sizeof(commTCP), 0);
                    DIE(ret < 0, "recv");
                    buffer[ret + 1] = '\0';
                    std::string clientID(buffer);

                    // Se verifica daca clientul are nume prea lung pentru a se
                    // refuza conexiunea
                    if (clientID.length() > 10) {
                        ret = send(newsockfdTCP, "__reject__", strlen("__reject__"), MSG_NOSIGNAL);
                        DIE(ret < 0, "rejectFail");
                        close(newsockfdTCP);
						FD_CLR(newsockfdTCP, &read_fds);
                        continue;
                    }

                    // Se verifica daca clientul de se conecteaza este nou sau
                    // este unul vechi care s-a intors
                    if (uniqueValue(clients, clientID)) {
                        clients.insert({newsockfdTCP, {clientID, true}});
                    } else {
                        for (auto it = clients.begin(); it != clients.end(); ++it) {
                            if (it->second.first.compare(clientID) == 0) {
                                for (auto& topic : topics) {
                                    if (topic.second->getClients()->count(clientID) > 0) {
                                        clientData save = topic.second->getClients()->at(clientID);
                                        topic.second->removeClient(clientID);
                                        if (save.SF == 1) {
                                            for (auto it = save.nrMsg +
                                                topic.second->getStore()->begin();
                                                it < topic.second->getStore()->end(); ++it) {
                                                toTCP aux;
                                                memset(&aux, 0, sizeof(toTCP));
                                                aux.tip_date = it->first.first;
                                                memcpy(aux.continut, it->second.c_str(), MSG_SIZE + 1);
                                                switch(aux.tip_date) {
                                                    case 0:{
                                                        aux.size = sizeof(INT);
                                                        break;
                                                    }
                                                    case 1:{
                                                        aux.size = sizeof(SHORT_REAL);
                                                        break;
                                                    }
                                                    case 2:{
                                                        aux.size = sizeof(FLOAT);
                                                        break;
                                                    }
                                                    case 3:{
                                                        aux.size = strlen(aux.continut);
                                                        break;
                                                    }
                                                    default: break;
                                                }
                                                aux.address = it->first.second;
                                                aux.size += sizeof(struct sockaddr_in);
                                                memcpy(aux.topic, topic.first.c_str(), 
                                                                  topic.first.length());
                                                ret = send(newsockfdTCP, &aux, aux.size + 
                                                TOTCP_METADATA + TOPIC_EXTENDED + 1, MSG_NOSIGNAL);
                                                DIE(ret < 0, "send");
                                            }
                                        }
                                        topic.second->addClient(clientID, newsockfdTCP, save.SF);
                                    }
                                }
                                clients.erase(it);
                                break;
                            }
                        }
                        clients.insert({newsockfdTCP, {clientID, true}});

                    }
					printf("New client (%s) connected from %s:%d.\n", clientID.c_str(),
							inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
				} else {
					// S-au primit date pe una din conexiunile deja active si
                    // serverul trebuie sa le proceseze
                    commTCP msg;
                    memset(&msg, 0, sizeof(commTCP));
                    n = recv(i, (char *)&msg, sizeof(commTCP), 0);
                    DIE(n < 0, "recv");

                    // Daca clientul a anuntat ca se inchide, serverul il
                    // marcheaza ca offline.
                    if (strncmp(msg.command, "exit", 4) == 0) {
                        for (auto it = clients.begin(); it != clients.end(); ++it) {
                            if (it->first == i) {
                                printf("Client (%s) disconnected.\n", it->second.first.c_str());
                                auto itAux = *it;
                                clients.erase(it);
                                clients.insert({itAux.first, {itAux.second.first, false}});
                                break;
                            }
                        }
						close(i);
						FD_CLR(i, &read_fds);
                        continue;
                    }
                    
                    // Daca clientul vrea sa se aboneze la un topic se
                    // verifica daca exista sau nu si se adauga daca e nevoie.
                    if (strncmp(msg.command, "subscribe", 9) == 0) {
                        std::string topicAux;

                        if (msg.topic[TOPIC_SIZE - 1] == 0) {
                            topicAux = std::string(msg.topic);
                        } else {
                            topicAux = std::string(msg.topic, TOPIC_SIZE).append("\0");
                        }
                        if (topics.count(topicAux) > 0) {
                            topics.at(topicAux)->addClient(clients.at(i).first, i, msg.SF);
                        } else {
                            topics.insert({topicAux, new Topic(topicAux, clients.at(i).first, i, msg.SF)});
                        }
                        continue;
                    }

                    // Daca se doreste dezabonarea de la un topic se verifica
                    // mai intai daca acel topic exista. Daca da, pur si simplu
                    // se elimina, altfel, se afiseaza o eroare in server, dar
                    // se poate considera ca clientul nu mai e abonat:
                    // un topic nu exista -> nu are abonati
                    if (strncmp(msg.command, "unsubscribe", 11) == 0) {
                        std::string topicAux(msg.topic);
                        if (topics.count(topicAux) > 0) {
                            topics.at(topicAux)->removeClient(clients.at(i).first);
                        } else {
                            printf("No topic named %s\n", topicAux.c_str());
                        }
                    }
				}
			}
		}
	}
	close(sockfdTCP);
    close(sockfdUDP);
	return 0;
}
