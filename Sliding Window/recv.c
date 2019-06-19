// Copyright - 2019 [Craciunoiu Cezar]
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "link_emulator/lib.h"
#include "./utils.h"

#define HOST "127.0.0.1"
#define PORT 10001

int main(int argc,char** argv){
  	msg r, t; 	  		// folosite la primit/trimis mesaje
  	msg_struct aux; 	// folosit la accesul campurilor salvate in payload
  	int result = 0; 	// salveaza codul primirii fiecarui mesaj
  	int timeout = 0;	// timpul de timeout 
  	int originalW = 0;	// dimensiunea ferestrei sender-ului
  	int W = 0;			// dimensiunea ferestrei receiver-ului
	init(HOST,PORT);
	memset(&t, 0, sizeof(msg));
	
	// La inceput se asteapta primirea meta-datelor si se resping toate
	// pachetele eronate. Pentru verificarea pachetelor se aplica acelasi
	// hash pe mesaje si se compara cu octetii de control. Daca mesajul
	// este bun se salveaza datele si se trece mai departe
	while(result <= 0) {
		memset(&r, 0, sizeof(msg));
		result = recv_message_timeout(&r, 1200);
		if (result > 0) {
			memset(&aux, 0, sizeof(msg_struct));
			memcpy(&aux, r.payload, sizeof(msg_struct));
			if (aux.control1 == aux.control2 &&
				aux.control1 == (calculateHash(aux.payload, sizeof(aux.payload)) ^ 
						calculateHash((char *) &aux.index, sizeof(aux.index)) ^
						calculateHash((char *) &r.len, sizeof(r.len)))) {
				strcpy(t.payload, "ACK");
				t.len = strlen(t.payload) + 1;
				timeout = r.len;
				W = aux.index;
				send_message(&t);
				break;
			} else {
				result = -1;
				strcpy(t.payload, "NACK");
				t.len = -1;
			}
		} else {
			strcpy(t.payload, "NACK");
			t.len = -1;
		}
		send_message(&t);
	}

  	int fd = open(aux.payload, O_WRONLY | O_TRUNC | O_CREAT, 0700);
  	int index = 0;		// Indexul pana la care s-a scris in fisier
  	int stopIndex = -1; // Indexul cadrului la care s-a oprit sender-ul
	int bigIndex = 0;	// Indexul ultimului cadru ce ar trebui sa ajunga
	int retries = 0;	// Numarul de reincercari intr-un anumit punct
  	originalW = W;
  	W = COUNT;
	msg* slidingData = calloc(W, sizeof(msg)); // Fereastra receiver-ului

	// Deoarece codul receiver-ului este intr-o mare bucla while codul v-a fi
	// explicat pe parcurs:
  	while (1) {
		memset(&r, 0, sizeof(msg));
		result = recv_message_timeout(&r, timeout * 1.2);
		
		// Daca s-a scris pana la final inseamna ca programul s-a terminat cu
		// succes 
		if (stopIndex == index) {
			t.len = 0;
			t.payload[0] = 'F';
			send_message(&t);
			break;
		}

		// Daca s-a primit un mesaj se da reset la contorul pentru retry-uri
		if (result > 0) {
			retries = 0;

			// Daca lungimea mesajului e 3 inseamna ca s-a citit in sender
			// pana la final, dar se verifica si daca mesajul este de FIN,
			// daca este -> acela este ultimul cadru care trebuie primit
			if (r.len == 3) {
				memcpy(&aux, r.payload, sizeof(msg_struct));
				if (aux.payload[0] == 'F' && aux.payload[1] == 'I' && aux.payload[2] == 'N') {
					stopIndex = aux.index;
					continue;
				} else {
					continue;
				}
			}
			memcpy(&aux, r.payload, sizeof(msg_struct));

			// Daca nu este primul cadru si s-a scris primul cadru si cadrele
			// sunt "la zi" atunci inseamna ca un cadru s-a pierdut de mai mult
			// de 2 ori si deja indecsii nu il mai "tin minte" dar el trebuie totusi
			// primit asa ca trimite o cerere noua
			if (index != 0 && bigIndex != 0 && bigIndex != index && 
					aux.index != index && (bigIndex  % originalW == index % originalW)) {
				strcpy(t.payload, "NACK");
				t.len = -index;
				send_message(&t);
			}
			memcpy(&aux, r.payload, sizeof(msg_struct));

			// Daca cadrul sosit este corect
			if (aux.control1 == aux.control2 &&
				aux.control1 == (calculateHash(aux.payload, sizeof(aux.payload)) ^ 
								calculateHash((char *) &aux.index, sizeof(aux.index)) ^
								calculateHash((char *) &r.len, sizeof(r.len)))) {

				// Se incrementeaza doar atunci vin pachete noi (nu retrimise)
				if (bigIndex <= aux.index) {
					bigIndex++;
				}
				slidingData[aux.index % W] = r;

				// Daca pachetul primit este cel la care s-a oprit scrierea
				// se poate reincepe scrierea
				if (index == aux.index) {

					// Se scrie pana cand se da de un nou cadru lipsa si se
					// marcheaza ca goluri cadrele scrise
					while (slidingData[index % W].len != 0) {
						memcpy(&aux, slidingData[index % W].payload, 
							   sizeof(msg_struct));
						write(fd, aux.payload, slidingData[index % W].len);
						slidingData[index % W].len = 0;
						index++;
					}
				} else {
					
					// Daca indexul nu corespunde atunci se trimite cerere
					// pentru toate cadrele pana cand se ajunge "la zi"
					if (bigIndex <= aux.index) {
						while (bigIndex <= aux.index) {
							strcpy(t.payload, "NACK");
							t.len = -(bigIndex - 1);
							bigIndex++;
							send_message(&t);
						}
					}
				}
				
				// Daca pachetul ajuns nu este mai mare decat indexul asteptat
				// atunci se trimite ca pachetul a fost acceptat 
				if (aux.index <= bigIndex - 1) {
					memcpy(&aux, r.payload, sizeof(msg_struct));
					strcpy(t.payload, "ACK");
					t.len = aux.index;
					send_message(&t);
				}
			} else {
				bigIndex++;
				
				// Daca pachetul corupt este mai vechi (a ajuns mai tarziu),
				// se ignora
				if (aux.index < index) {
					continue;
				}

				// Daca pozitia la care voia sa se scrie e in afara
				// intervalului sau este ocupata inseamna ca s-a corupt indexul
				if ((aux.index > 0 && aux.index < W) &&
						(slidingData[aux.index % W].len != 0)) {
					continue;
				}

				// Daca indexul mesajului nu e prea indepartat (daca nu e mai
				// mare ca o fereastra intreaga din acel punct) inseamnca ca se
				// poate recere direct indexul, altfel se incearca cererea
				// bigIndex-ului (o cerere inseamna mereu len negat)
				if (aux.index < bigIndex + originalW) {
					t.len = -(aux.index);
				} else {
					t.len = -(bigIndex - 1);
				}

				strcpy(t.payload, "NACK");
				send_message(&t);
			}
		} else {

			// Daca s-a primit timeout se verifica daca s-a ajuns la limita
			// si se recere pachetul, sperand ca o sa ajunga. Aici se ajunge
			// de obicei daca a aparut o eroare sau s-a pierdut pachete in 
			// ultima faza a sender-ului (cand nu mai citeste)
			if (retries == 10) {
				break;
			}
			retries++;
			strcpy(t.payload, "NACK");
			t.len = -index;
			send_message(&t);
		}
  	}

	close(fd);
	free(slidingData);
  	return 0;
}
