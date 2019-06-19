// Copyright Craciunoiu Cezar 324CA
#ifndef _HELPERS_
#define _HELPERS_

#define BUFLEN 5000 // Lungimea maxima a unui mesaj
#define LINELEN 1000 // Lungimea maxima a unei linii ce urmeaza sa fie scrisa
#define IP_SERVER "185.118.200.35" // IP server pe care se realizeaza tema
#define PORT_SERVER 8081 // Port server pe care se realizeaza tema

#define COOKIE_DIM  256 // Lungimea maxima a unui cookie
#define MAX_COOKIES 128 // Numarul maxim de cookie-uri
#define MAX_FIELDS  128 // Numarul maxim de campuri din campul data
#define MAX_FORMSZ  256 // Dimensiunea maxima a campului de form   
#define MAX_ANSWSZ  512 // Dimensiunea maxima a raspunsului (Etapa 3)
#define MAX_TOKENSZ 900 // Dimensiunea maxima a token-ului JWT
#define HTTP_PORT   80  // Port default HTTP
#define HTTPS_PORT  443 // PORT default HTTPS
#define MAX_DNSADDR 64  // Dimensiunea maxima a adresei cu nume pentru DNS

// Macro de extragere a informatiei ce se afla la fiecare in fiecare mesaj
// venit de la server
#define EXTRACT_BASICINFO(fields, data, url, method, type)  \
	do {									                \
	    fields = json_value_get_object(root_value);         \
        data   = json_object_get_object(fields, "data");    \
        url    = json_object_get_string(fields, "url");     \
        method = json_object_get_string(fields, "method");  \
        type   = json_object_get_string(fields, "type");    \
	} while(0)

// Structura in care se salveaza datele primite de la server-ul DNS
typedef struct {
    char ip[128];
    int port;
} ip_port;

// Inchide programul
void error(const char *msg);

// Formateaza mesajul pentru HTTP
void compute_message(char *message, const char *line);

// Deschide o conexiune TCP/UDP
int open_connection(char *host_ip, int portno, int ip_type, int socket_type,
                    int flag);

// Inchide conexiunea deschisa anterior
void close_connection(int sockfd);

// Trimite un mesaj la server
void send_to_server(int sockfd, char *message);

// Primeste un mesaj de la server
char *receive_from_server(int sockfd);

// Primeste un mesaj de la server-ul DNS
ip_port get_ip(const char *name);

// Primeste un mesaj de la server-ul de vreme
char *receive_from_weather_server(int sockfd);

// Extrage cookie-urile dintr-un mesaj
void parseCookies (char *toParse, char **cookies, int *nr_cookies);

#endif
