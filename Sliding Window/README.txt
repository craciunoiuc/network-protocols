========================= Tema 1 - Fereastra Glisanta =========================
Nume: Craciunoiu Cezar
Grupa: 324CA

In tema s-a implementat un protocol de transmisie a datelor, de nivelul
legatura de date care se foloseste de de o fereastra glisanta pentru a citi
si scrie datele in fisiere. Fereastra glisanta e folosita pentru a trimite
rafale de date pentru a se folosi intreaga conexiune disponibila.
Executia v-a fi explicata in in paralel pentru sender si receiver:
La inceput se trimite un singur cadru cu informatii de conexiune si fisier.
	- Sender-ul trimite numele fisierului in care o sa se scrie, marimea
	ferestrei si timp mediu cat dureaza unui cadru sa ajunga la destinatie. El
	retrimite aceste date pana cand sunt acceptate de receiver.
	
	- Receiver-ul asteapta o perioada generoasa de timp primul cadru si, daca
	se primeste se verifica cu algoritmul de hash daca acesta este bine primit.
	Cand un cadru e respins, se trimite o valoare negativa in campul len,
	altfel, daca e acceptat, valoarea e pozitiva.

Partea a 2-a a programului e reprezentata de trimiterea in rafala a datelor
cat dimensiunea ferestrei de catre sender:
	- Sender-ul citeste din fisier, calculeaza hash-ul, incrementeaza indexul
	si trimite cadrul pana cand fereastra este plina adica legatura de date
	este plina. Mereu cand se citeste din fisier se verifica daca s-a terminat
	citirea.

	- Receiver-ul nu face nimic in acest timp, in principal pentru ca datele
	inca nu au ajuns la el (cand ajunge primul cadru termina de trimis rafala
	sender-ul).

Partea a 3-a implica ambele parti ale protocolului:
	- Sender-ul asteapta sa primeasca o confirmare/infirmare pentru a continua
	executia. Daca cadrul e confirmat se citeste mai departe din fisier si se
	repeta procesul ca mai sus. Daca cadrul e infirmat, adica s-a pierdut sau
	corupt pe drum, sender-ul retrimite acel cadru. Cadrele pierdute/corupte se
	tin intr-o structura de date reprezentata printr-un vector care se umple
	si se goleste de fiecare data cand un cadru e infirmat/confirmat.

	- Receiver-ul asteapta sa primeasca un cadru, si daca primeste timeout,
	recere acel cadru. Altfel, el verifica daca continutul cadrului este valid
	si daca este isi incrementeaza unul din indecsi si apoi verifica daca se
	mai poate scrie in fisier (adica daca indexul de scriere este ocupat). Daca
	se poate scrie, se goleste fereastra pana se ajunge la un cadru lipsa. Daca
	un cadru a fost pierdut/corupt, se cere retrimiterea acestuia. Deoarece
	exista posibilitatea ca un cadru retrimis sa se piarda din nou si din nou
	se iau precautii cum ar fi o ferestra mai mare respectiv verificarea sa nu
	se suprascrie datele.

Partea a 4-a - sfarsitul protocolului:
	- Sender-ul a terminat de citit in a 4-a parte si tot ce face e sa astepte
	confirmarile de la receiver si sa retrimita cadrele care au fost infirmate.
	O data ce primeste mesajul ca receiver-ul a reusit sa termine scrierea cu
	succes isi inchide el el conexiunea respectiv fisierul din care citea.
	
	- Receviver-ul se comporta exact la fel ca in partea a 3-a, deoarece,
	pentru el, cadrele se primesc si se cer in exact acelasi mod. Si la acesta
	la final se incheie conexiunea si se elibereaza memoria.

Precizari implementare:
	-- Receiver-ul nu asteapta foarte mult dupa sender, daca a trecut timpul
	cat pentru 10 cadre il anunta pe sender si incheie conexiunea pentru a
	evita ciclarea.
	-- Sender-ul se foloseste aproape mereu de array-ul de retrimitere a
	datelor. Astfel, daca se cer niste date care nu se afla in acesta el
	considera ca a aparut o eroare la receiver si incheie conexiunea dupa ce
	il anunta pe receiver, tot pentru a evita ciclarea sau scrierea eronata a
	datelor.

	-- Receiver-ul tine intentionat o fereastra mai mare, deoarece atunci cand
	datele se pierd consecutiv (de exemplu cadrul 5 este trimis de 3-4 ori si
	de fiecare data este pierdut) si fereastra este relativ mare (> 100) se
	pot ajunge la foarte multe date nescrise care incep sa se suprapuna. Cum
	scrierea la anumite pozitii din fisier nu este permisa raman ca optiuni
	redimensionarea ferestrei sau alegerea ei direct cu o valoare mare, in
	cazurile care conteaza asta insemnand acelasi lucru.

Dupa cum se precizeaza si mai sus, protocolul incearca sa combine
corectitudinea cu viteza maxima, astfel incat daca protocolul cicleaza sau alte
erori au aparut, el alege sa se opreasca singur si "mai bine sa mai incerce o
data".

Precizari checker:
	-- Desi se evita eroare de mai jos netrimitandu-se cadre de lungime 0,
	checkerul afiseaza totusi eroarea cateodata:
	"
	./checker.sh: line 250: 13154 Floating point exception(core dumped) 
	./link_emulator/link speed="${SPEEDS[$i]}" delay="${DELAYS[$i]}" 
	loss="${LOSSES[$i]}" corrupt="${CORRUPTS[$i]}" reorder="${REORDERS[$i]}" 
	&> /dev/null
	"
	Am rulat de peste 20 de ori cu checkerul manual teste cu corupere si nu am
	primit eroarea cu floating point. Recomand daca mai apare sa se ruleze
	manual testul.
	
	-- Pe ultimul test apare eroarea de segmentation fault in checker, aici nu
	pot spune ce cauzeaza acest lucru deoarece nu imi pot da seama daca eroarea
	este din link/send/recv sau o combinatie de toate 3.

	-- Decizia de a incheia transmisiunea la timeout a fost luata si fiindca
	de multe ori cele 2 programe (send/recv) isi opreau singure conexiunea
	(desi ambele inca erau pornite, nu se mai trimiteau mesaje pe legatura de
	date oricat de multe ar fi fost / la orice interval).

Referinte:

	-- Functie hash:
	https://stackoverflow.com/questions/1970548/simple-hash-function-1-byte-output-from-string-input


