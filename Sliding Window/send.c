// Copyright - 2019 [Craciunoiu Cezar]
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "link_emulator/lib.h"
#include "./utils.h"

#define HOST "127.0.0.1"
#define PORT 10000

int main(int argc,char** argv){
  	init(HOST,PORT);
	
	msg t, r; // folosite la trimis/primit mesaje
	msg_struct aux; // folosita doar la cititul/scrisul in campul payload
  	int result; // folosita la verificarea daca transmisia a reusit
	
	// Se calculeaza marimea ferestrei glisante
  	long BDP = 1000 * (atol(argv[3]) * atol(argv[2]));
	int W = BDP / (8 * sizeof(msg));
	if (W > COUNT) {
		W = COUNT;
	}
  	int fd = open(argv[1], O_RDONLY);
	
	// Se initializeaza fereastra glisanta si vectorul in care se salveaza
	// mesajele pierdute/corupte de multe ori. De obicei aceasta sta goala, dar
	// s-a luat o marime generoasa pentru a evita cazurile mai rar intalnite
	msg slidingData[W];
	msg retransmitData[COUNT / 5];
	memset(retransmitData, 0, sizeof(retransmitData));
  	 
	// In prima parte se trimite un mesaj cu metadate: numele fisierului,
	// timpul de timeout si marimea ferestrei. Daca mesajul este bun
	// receptorul trimite inapoi ACK/NACK in functie de care se retrimite
	// sau nu mesajul pana cand este sigur ca s-a primit cum trebuie.
	memset(&aux, 0, sizeof(msg_struct));
  	strcpy(aux.payload, "recv_fileX");
	aux.index = W;
  	t.len = atoi(argv[3]);
	aux.control1 = calculateHash(aux.payload, sizeof(aux.payload)) ^ 
				   calculateHash((char *) &aux.index, sizeof(aux.index)) ^
				   calculateHash((char *) &t.len, sizeof(t.len));
	aux.control2 = aux.control1;
	memcpy(t.payload, &aux, sizeof(aux));
  	send_message(&t);
	result = recv_message(&r);
	while (strcmp(r.payload, "ACK") != 0) {
  		memset(&r, 0, sizeof(msg));
		result = recv_message(&r);
		if (result <= 0) {	
			send_message(&t);
		} else {
			if (result > 0 && r.len > 0) {
				break;
			} else {
				send_message(&t);
			}
		}
	}

	// Urmeaza partea trimiterii in rafala. Se ia un index pentru a contoriza
	// cadrele, o variabila finished care marcheaza sfarsitul fisierului si
	// inca o variabila burst ce reprezinta marimea rafalei. In aceasta parte
	// nu se primesc pachete - doar se trimit, de aceea singura verificare
	// facuta este cea daca fisierul s-a terminat.
	short index = 0;
	char finished = 0;
	int burst = W;
	while (burst--) {
		memset(&t, 0, sizeof(msg));
		int bytesRead = read(fd, aux.payload, sizeof(aux.payload));
		aux.index = index;
		t.len = bytesRead;
		aux.control1 = calculateHash(aux.payload, sizeof(aux.payload)) ^ 
						calculateHash((char *) &aux.index, sizeof(aux.index)) ^
						calculateHash((char *) &t.len, sizeof(t.len));
		aux.control2 = aux.control1;
		memcpy(t.payload, &aux, sizeof(msg_struct));
		slidingData[index % W] = t; 
		if (bytesRead == 0) {
			finished = 1;
			aux.payload[0] = 'F';
			aux.payload[1] = 'I';
			aux.payload[2] = 'N';
			t.len = 3;
			aux.index = index;
			aux.control1 = calculateHash(aux.payload, sizeof(aux.payload)) ^ 
						   calculateHash((char *) &aux.index, sizeof(aux.index)) ^
						   calculateHash((char *) &t.len, sizeof(t.len));
			aux.control2 = aux.control1;
			memcpy(t.payload, &aux, sizeof(msg_struct));
			send_message(&t);
			send_message(&t);
			send_message(&t);
			break;
		}
		index++;
		send_message(&t);
	}

	// In continuare se mai trimit pachete pana la terminarea fisierului sau
	// pana la epuizarea constantei COUNT (ca in laborator)
	int remaining = COUNT - W;
	if (finished == 1) {
		remaining = 0;
	}

	// Cat timp mai pot fi trimise pachete se asteapta un mesaj. Daca mesajul
	// nu vine sunt sanse foarte mari ca receiver-ul a dat crash.
	//
	// -- Daca s-a primit un pachet confirmat (in len inapoi e pus index-ul
	// pozitiv daca a fost bun sau negativ invers) atunci se scoate din
	// array-ul de retrimitere si se citeste mai departe. Se construieste
	// mesajul si daca s-a ajuns la final se trimite o rafala de mesaje
	// si se opreste bucla. Daca nu, se adauga la cele de retrimis (in caz
	// de orice) si se trimite, crescandu-se index-ul.
	//
	// -- Daca se doreste retrimiterea atunci cadrul are in len valoare
	// negativa si se incearca luatul acestuia din fereastra. Daca la pozitia
	// respectiva nu se mai afla cadrul se cauta in fereastra de retrimitere.
	// Daca nu se gaseste inseamna ca a aparut o eroare si transmisia se inchide
	// pentru a evita ciclarea infinita.
	while (remaining--) {
		memset(&r, 0, sizeof(msg));
		int result = recv_message_timeout(&r, 1500);
		if (result <= 0) {
			finished = 1;
			close(fd);
			return -1;
		}
		if (r.len >= 0 && r.payload[0] != 'N') {
			removeRetransmit(retransmitData, r.len, sizeof(retransmitData) / sizeof(msg));
			int bytesRead = read(fd, aux.payload, sizeof(aux.payload));
			t.len = bytesRead;
			
			aux.index = index;
			aux.control1 = calculateHash(aux.payload, sizeof(aux.payload)) ^ 
						   calculateHash((char *) &aux.index, sizeof(aux.index)) ^
						   calculateHash((char *) &t.len, sizeof(t.len));
			aux.control2 = aux.control1;
			memcpy(t.payload, &aux, sizeof(msg_struct));
			slidingData[index % W] = t;
			if (bytesRead == 0) {
				finished = 1;
				aux.payload[0] = 'F';
				aux.payload[1] = 'I';
				aux.payload[2] = 'N';
				t.len = 3;
				aux.index = index;
				aux.control1 = calculateHash(aux.payload, sizeof(aux.payload)) ^ 
							   calculateHash((char *) &aux.index, sizeof(aux.index)) ^
							   calculateHash((char *) &t.len, sizeof(t.len));
				aux.control2 = aux.control1;
				memcpy(t.payload, &aux, sizeof(msg_struct));
				send_message(&t);
				send_message(&t);
				send_message(&t);
				break;
			}
			addRetransmit(retransmitData, &t, sizeof(retransmitData)/sizeof(msg));
			index++;
			send_message(&t);
		} else {
			t = slidingData[-r.len % W];
			memcpy(&aux, t.payload, sizeof(msg_struct));
			if (aux.index != -r.len) {
				t = retransmit(retransmitData, -r.len, sizeof(retransmitData)/sizeof(msg));
			} else {
				addRetransmit(retransmitData, &t, sizeof(retransmitData)/sizeof(msg));
			}
			if (t.len == -__INT32_MAX__) {
				close(fd);
				return -1;
			}
			memcpy(&aux, t.payload, sizeof(msg_struct));
			send_message(&t);
		}
	}

	// In ultima parte doar se primesc mesaje si se retrimit cele care au fost
	// marcate ca eronate/pierdute. Daca receiver-ul nu mai raspunde sau a
	// raspuns cu mesajul 'F' si lungime 0 inseamna ca a incheiat transimisa.
	while (1) {
		memset(&r, 0, sizeof(msg));
		int result = recv_message_timeout(&r, 1000);
		if (result <= 0) {
			finished = 1;
			break;
		}
		if (r.len == 0 && r.payload[0] == 'F') {
			finished = 1;
			break;
		} else {
			if (r.len < 0) {
				t = slidingData[-r.len % W];
				memcpy(&aux, t.payload, sizeof(msg_struct));
				if (aux.index != -r.len) {
					t = retransmit(retransmitData, -r.len, sizeof(retransmitData)/sizeof(msg));
				} else {
					addRetransmit(retransmitData, &t, sizeof(retransmitData)/sizeof(msg));
				}

				memcpy(&aux, t.payload, sizeof(msg_struct));
				send_message(&t);
			}
		}
	}

  close(fd);
  return 0;
}
