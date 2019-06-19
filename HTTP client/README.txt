=================================Tema 3 - HTTP=================================
Nume: Craciunoiu Cezar
Grupa: 324CA

Implementarea temei rezolva cele 5 etape propuse care verifica diferite
interactiuni de nivel aplicatie. In tema s-a folosit o biblioteca pentru
a face parse la fisiere de tip JSON numita "Parson".
De asemenea, in program s-au folosit mai multe functii de la laborator, la care
s-a mai adaugat eventual. Prezentarea temei se va face pe etape. La fiecare
etapa se extrag cookie-urile.

In prima etapa se cere direct din enunt sa se faca o cerere de tip GET la un
IP si URL deja dat. Pentru a realiza acest lucru se folosesc fisierele helpers
si requests.
    - In helpers se afla in principal functii folosite pentru conexiunea la
    diferite servere (HTTP/DNS). Functiile deschid conexiunea, trimit mesajul
    dorit, primesc raspunsul de la server si apoi inchid conexiunea.

    - In requests se afla functii pentru construirea unui mesaj de tip GET/POST
    pentru serverul de HTTP. Functiile formateaza mesajul ce se doreste trimis
    pentru a respecta formatul de HTTP GET/POST. Mesajul e apoi trimis mai
    departe la server.

In a doua etapa mai intai se parse-aza JSON-ul primit si se extrag campurile
dorite din el. Din campul data mai intai se extrage numele si valoarea fiecarui
camp in parte si apoi se pun impreuna acestea pentru a fi puse ca date
in formularul ce va fi trimis cu POST. Mesajul e compus "generic" fara a se
cunoaste ce reprezinta parametrii din formular (se presupune doar ca trebuiesc
pusi in formular).

In etapa a treia se parse-aza din nou JSON-ul primit. Se extrag ca anterior
campurile de baza pentru orice request. Se trece la interpretarea campurilor
din data. Se salveaza din nou, in parte, campurile din data si se verifica
daca exista imbricare (se presupune ca acolo unde exista, adancimea e maxim 2,
iar datele trebuiesc puse impreuna sub forma nume=valoare&...).
Dupa ce s-a trecut prin date, se cauta token-ul JWT ce se doreste pus si campul
ce va reprezenta datele ce vor fi data ca parametrii de URL.
Se adauga de asemenea raspunsul la cele 2 ghicitori.

In etapa a patra doar se extrag datele de baza si se trimite o cerere la server
cu token-ul si cookie-urile corespunzatoare

In etapa cinci se interpreteaza mai intai JSON-ul de la etapa a patra. In mod
asemanator se extrag datele din campul data si se compun datele necesare
pentru queryParams. Dupa aceea, se cauta in in datele scoase din data,
campurile "basic" pentru server-ul de vreme. Pentru a se interoga server-ul de
vreme este nevoie mai intai de adresa IP a acestuia.
Se interogheaza serverul de DNS si se obtine IP-ul, apoi, se compune mesajul si
se interogheaza serverul de vreme. Acesta intoarce un JSON.
Se preia JSON-ul primit si se trimite mai departe la server exact asa cum se
primeste.

La final se elibereaza toata memoria utilizata.

Bibliografie:
    Parson - https://github.com/kgabis/parson
