=================================Tema 2 - PC===================================
                         --Aplicatie Client-Server--

Nume si prenume: Craciunoiu Cezar
Grupa: 324CA

In program este implementat un server si un client intre care se realizeaza o
conexiune TCP. Serverul cominica prin UDP cu client oferit de responsabilii de
tema. Arhiva are mai multe fisiere sursa si headere ce contin diferite parti
ale programului precum (fisierele sunt luate in ordine inversa a importantei):

- Fisierul "messageTypes.h":
	Acesta contine structuri in care sunt puse date pentru a se transimte cat
	mai usor pe retea. De asemenea aici mai sunt continute si reprezentari ale
	datelor din tabelul dat din enunt. Fisierul se foloseste peste tot.

- Fisierul "messageParser.h":
	Acesta contine o clasa ce nu poate fi instantiata cu functiile necesare
	transformarii unui buffer de char-uri venit de pe retea intr-un vector de
	informatii human-readable ce poate fi afisat la consola. Fisierul se
	foloseste in subscriber.

- Fisierul "topic.h":
	Acesta reprezinta o structura de date formata dintr-un map si un deque.
	Aici se retin clientii abonati respectiv mesajele care au fost trimise la
	acest topic. In aceasta clasa se mai afla si alte functii folositoare
	pentru modificarea informatiilor clientilor sau adaugarea unui nou mesaj
	sau client. Fisierul se foloseste in server.

- Fisierul "subscriber.cpp":
	Aici este implementat clientul TCP din program. Acesta realizeaza mai
	intai o conexiune cu serverul si apoi isi trimite ID-ul. Apoi, el intra
	intr-o bucla infinita si asteapta informatie de la server sau de la
	tastatura. Pentru datele de la tastatura, el asteapta informatie, apoi
	pune totul intr-un buffer. El citeste de acolo si construieste mesajul
	in functie de comanda, pe care il trimite la server. Daca se primesc date
	de la server, subscriber-ul foloseste functiile din "messageParser.h"
	pentru a parsa informatia si o afiseaza la consola. Mai pot aparea 2
	mesaje "__shutdown__"(serverul se inchide) si "__reject__"(id-ul nu este
	bun) ce duc la inchiderea subscriber-ului. Daca serverul s-a inchis
	necorespunzator bucla da timeout de zece ori si se inchide programul.

- Fisierul "server.cpp":
	Reprezinta si cea mai importanta parte a programului. Serverul deschide
	mai intai un socket UDP si unul TCP pe care sa comunice. Apoi, el intra
	intr-o bucla infinita in care multiplexeaza socketii pentru a primi date
	de pe oricare. Rezulta mai multe cazuri:
		-- Daca s-au primit date pe socketul UDP serverul salveaza datele
		transmitatorului si ce a trimis si adauga in topicul respectiv sau
		il creeaza daca e nevoie. Apoi el trimite la fiecare client abonat
		informatia primita (in caz ca un client s-a oprit necorespunzator, de
		exemplu ^C, primul mesaj send il trimite normal si se pierde. La al
		doilea observa ca s-a stricat conexiunea asa ca acel client e marcat
		ca deconectat/Offline).
		-- Daca s-au primit date de la tastatura si reprezinta comanda "exit"
		atunci serverul trimite mesajul "__shutdown__" la toti clientii si se
		inchide.
		-- Daca s-au primit date pe socketul TCP inseamna ca a venit o
		conexiune noua. Aceasta este acceptata si se citeste ID-ul primit.
		Daca nu este corespunzator conexiunea se inchide trimitandu-se
		"__reject__"(nu se verifica daca 2 clienti au acelasi nume deoarece
		acest lucru e precizat in enunt). Daca clientul ce a venit este unul
		mai vechi (ID-ul lui e deja retinut) atunci se trece prin fiecare
		topic se se verifica daca este la zi cu mesajele. I se trimite fiecare
		mesaj cu care a ramas in urma pana cand este la zi.
		-- Altfel, s-au primit date pe conexiunile TCP deja active, deci
		serverul trebuie sa proceseze comenzile:
			--- Daca primeste "exit" il marcheaza ca offline peste tot.
			--- Daca primeste "subscribe" il adauga la acel topic sau il
			creeaza daca e nevoie.
			--- Daca priemste "unsubscribe" incearca sa il scoata de la acel
			topic. Daca topicul nu exista se afieaza o eroare in server si
			cerinta e ignorata (un topic nu exista -> nu are abonati)

Alte precizari:
- In subscriber poate exista cazul in care TCP a segmentat un mesaj. In acest
caz restul de informatie se afiseaza direct. Deoarece informatia ce are sansele
sa fie fragmentata este sigur string, daca se primeste un mesaj fragmentat se
afiseaza prima parte fara '\n' si apoi restul cu '\n', recompunandu-se mesajul
la afisare.
- Pentru fiecare functie din API-ul "socket.h" se foloseste macro-ul DIE de la
laborator pentru evitarea starilor incerte.
- In cod s-a pastrat metrica: 80 caractere pe linie de comentarii respectiv
100 caractere pe linia de cod.
