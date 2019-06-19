// Copyright - 2019 [Craciunoiu Cezar]
#ifndef UTILS
#define UTILS

#define COUNT 1000


// Structura ce este pusa in campul payload al mesajului
typedef struct {
	unsigned short index; // indexul mesajului din transmisie
	char payload[MSGSIZE - sizeof(short) - 2 * sizeof(char)]; // mesajul
	char control1; // byte de control pentru index si payload
	char control2; // byte de control pentru byte de control
} msg_struct;

// Functie de calcul a maximului
int max(int a, int b) {
  return (a > b) ? a : b;
}

// Functia cauta in vectorul de retransmitere mesajul dorit si il returneaza
// Daca nu se gaseste se returneaza o eroare. Functia se foloseste in cazul
// In care un pachet s-a pierdut de mai mult de o data si deja a fost
// suprascris in fereastra sender-ului.
msg retransmit(msg* retransmitArray, int index, int arrLen) {
	msg_struct aux;
	int i;
	for (i = 0; i < arrLen; ++i) {
		if (retransmitArray[i].len == 0) {
			continue;
		}
		memcpy(&aux, retransmitArray[i].payload, sizeof(msg_struct));
		if (aux.index == index) {
			return retransmitArray[i];
		}
	}
	msg error;
	error.len = -__INT32_MAX__;
	aux.index = __UINT16_MAX__;
	strcpy(aux.payload, "ERROR");
	memcpy(error.payload, &aux, sizeof(msg_struct));
	return error;
}

// Adauga un element in vectorul de retransmitere pe prima pozitie goala
void addRetransmit(msg* retransmitArray, msg* savedMsg, int arrLen) {
	int i;
	for (i = 0; i < arrLen; ++i) {
		if (retransmitArray[i].len == 0) {
			retransmitArray[i] = *savedMsg;
			return;
		}
	}
}

// Elimina elementul ce are indexul egal cu cel dorit
void removeRetransmit(msg* retransmitArray, int index, int arrLen) {
	msg_struct aux;
	int i;
	for (i = 0; i < arrLen; ++i) {
		if (retransmitArray[i].len == 0) {
			continue;
		}
		memcpy(&aux, retransmitArray[i].payload, sizeof(msg_struct));
		if (aux.index == index) {
			retransmitArray[i].len = 0;
			return;
		}
	}
}

// Calculeaza un hash simplu pe un char cu toti octetii din payload + alte
// elemente daca se doreste. S-a ales aceata varianta in loc de numararea
// bit-ilor setati deoarece nu se putea aloca in mod eficient un singur bit
// pentru verificarea corectitudinii, dar si pentru un calcul mai sigur al
// certitudinii. De asemenea, viteza ar trebui sa fie mai buna deoarece cel pe
// biti merge bit cu bit -> theta(len) mai buna ca theta(len * bitsPerByte)
char calculateHash(char* charArray, int len) {
	char hash = 0;
	int i;
	for (i = 0; i < len; ++i){
		hash = ((37 * hash) + charArray[i]) & 0xff;
	}
	return hash;
}

#endif //UTILS
