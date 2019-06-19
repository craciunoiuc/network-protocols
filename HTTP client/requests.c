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
#include "./requests.h"

// Construieste un mesaj HTTP de tip GET sau POST in mod generalizat. Campurile
// care sunt goale (ce apartin doar de GET sau de POST) contin NULL.
char *compute_any_request(const char *host,const char *request, const char *url,
        const char *url_params, const char *form_data, const char *type,
        const char **cookies, const int nrCookies, const char* authorizeToken)
{
    if (strncmp(request, "GET", 3) == 0)
    {
        return compute_get_request(host, url, url_params, cookies, nrCookies, 
                                   authorizeToken);
    }
    if (strncmp(request, "POST", 4) == 0)
    {
        return compute_post_request(host, url, form_data, type, cookies, 
                                    nrCookies, authorizeToken);
    }
    return "";
}

// Functie preluata din laborator (modificata)
// Functia construieste pas cu pas o cerere de tip GET si intoarce mesajul
// compus pentru a fi trimis la server.
char *compute_get_request(const char *host, const char *url, const char *url_params, 
                    const char **cookies, int nrCookies, const char* authorizeToken)
{
    char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));

    // Mai intai se compune tipul cererii si se adauga,
    // daca exista, parametrii url.
    if (url_params != NULL)
    {
        sprintf(line, "GET %s?%s HTTP/1.1", url, url_params);
    }
    else
    {
        sprintf(line, "GET %s HTTP/1.1", url);
    }
    compute_message(message, line);

    // Se adauga host-ul destinatie (trebuie sa existe)
    sprintf(line, "Host: %s", host);
    compute_message(message, line);
    
    // Se adauga token-ul JWT daca exista
    if (authorizeToken != NULL)
    {
        sprintf(line, "Authorization: %s %s", AUTH_TYPE, authorizeToken);
        compute_message(message, line);
    }
    
    // Se adauga fiecare cookie la rand din vectorul de cookie-uri.
    if (nrCookies > 0) {
        sprintf(line, "Cookie: ");
        for (int i = 0; i < nrCookies; ++i)
        {
            strcat(line, cookies[i]);
            strcat(line, "; ");
        }
        compute_message(message, line);
    }

    // Se adauga linia de final si se returneaza mesajul.
    compute_message(message, "");
    free(line);
    return message;
}

// Functie preluata din laborator (modificata)
// Functia construieste o cerere de tip POST si intoarce mesajul ce urmeaza
// sa fie trimis la server.
char * compute_post_request(const char *host, const char *url,
                            const char *form_data, const char *type, 
                            const char **cookies, const int nrCookies,
                            const char *authorizeToken)
{

    char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));

    // Se adauga tipul cererii si URL-ul
    sprintf(line, "POST %s HTTP/1.1", url);
    compute_message(message, line);

    // Se adauga Host-ul destinatie
    sprintf(line, "Host: %s", host);
    compute_message(message, line);
    
    // Se adauga token-ul de autorizare (daca exista)
    if (authorizeToken != NULL)
    {
        sprintf(line, "Authorization: %s %s", AUTH_TYPE, authorizeToken);
        compute_message(message, line);
    }
    
    // Se adauga pe rand fiecare cookie in mesaj
    if (nrCookies > 0) {
        sprintf(line, "Cookie: ");
        for (int i = 0; i < nrCookies; ++i)
        {
            strcat(line, cookies[i]);
            strcat(line, "; ");
        }
        compute_message(message, line);
    }
    // Se adauga tipul continutului si lungimea ce urmeaza sa fie mai jos
    sprintf(line, "Content-Type: %s", type);
    compute_message(message, line);
    sprintf(line, "Content-Length: %ld", strlen(form_data));
    compute_message(message, line);

    // Se adauga linia de final de antet
    compute_message(message, "");

    // Se adauga datele din final de form si se returneaza mesajul
    compute_message(message, form_data);
    free(line);
    return message;
}
