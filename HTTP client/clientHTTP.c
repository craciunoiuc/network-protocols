// Copyright Craciunoiu Cezar 324CA
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "./helpers.h"
#include "./requests.h"
#include "./parson.h"

// Codul a fost impartit in 5 etape
int main()
{
    char *message;
    char *response;
    int sockfd;
    const char *emptyCookie[1];
    emptyCookie[0] = "";

    // Pentru prima etapa se face un request de tip GET cu url-ul dat in enunt.
    // Conexiunea cu server-ul se realizeaza utilizand functiile din
    // laboratorul de HTTP, care au fost modificate pentru a fi mai generice.
    puts("ETAPA 1\n");
    sockfd = open_connection(IP_SERVER, PORT_SERVER, AF_INET, SOCK_STREAM, 0);
    message = compute_any_request(IP_SERVER, "GET", "/task1/start", 
                                  NULL, NULL, NULL, emptyCookie, 0, NULL);
    puts(message);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    close_connection(sockfd);
    puts(response);
    free(message);

    // Etapa 2 este putin mai lunga ca prima si este luata pe pasi
    puts("\nETAPA 2\n");

    // Mai intai se salveaza toate cookie-urile primite de la server
    int nr_cookies = 0;
    char *cookies[MAX_COOKIES];
    parseCookies(response, cookies, &nr_cookies);

    // Se trece la partea cu date de tip JSON (care incep cu '{')
    const char *toParse = strchr(response, '{');
    
    // Se instantiaza un obiect de tip JSON din biblioteca Parson
    JSON_Value *root_value;
    root_value = json_parse_string(toParse);
    if (json_value_get_type(root_value) != JSONObject)
    {
        json_value_free(root_value);
        return -1;
    }

    // Se construieste un obiect JSON din string-ul parsat si se extrag cele
    // 4 date ce ar trebui sa existe (aproape) mereu. Daca un camp de date nu
    // exista, functiile intorc NULL (caz care e verificat in 
    // compunerea ulterioara a mesajului)
    JSON_Object* fields;
    JSON_Object* data;
    const char *url;
    const char *method;
    const char *type;
    EXTRACT_BASICINFO(fields, data, url, method, type);

    // Se verifica cate date are campul data pentru a se trece prin ele
    int data_size = json_object_get_count(data);
    const char* params[MAX_FIELDS]; // nume campuri
    const char* values[MAX_FIELDS]; // valori campuri
    
    // Se adauga fiecare element gasit in data in cei doi vectori de string-uri
    for (int i = 0;  i < data_size; ++i)
    {
        params[i] = json_object_get_name(data, i);
        values[i] = json_object_get_string(data, params[i]);
    }

    // Se compune mesajul ce urmeaza sa fie pus in form_data cu forma
    // params[i]=xxx&values[i]=yyy
    char *form_data = (char *)calloc(MAX_FORMSZ, sizeof(char));
    for (int i = 0; i < data_size; ++i)
    {
        strcat(form_data, params[i]);
        strcat(form_data, "=");
        strcat(form_data, values[i]);
        if (i < data_size - 1)
        {
            strcat(form_data, "&");
        }
    }
    // Se construieste mesajul, care se afiseaza si se trimite la server.
    // Apoi, se afiseaza raspunsul primit de la server.
    sockfd = open_connection(IP_SERVER, PORT_SERVER, AF_INET, SOCK_STREAM, 0);
    message = compute_any_request(IP_SERVER, method, url, "", form_data, 
                                type, (const char **) cookies, nr_cookies, NULL);
    puts(message);
    send_to_server(sockfd, message);
    free(response);
    response = receive_from_server(sockfd);
    close_connection(sockfd);
    puts(response);

    // Se elibereaza memoria folosita la aceasta etapa
    free(message);
    free(form_data);
    for (int i = 0; i < nr_cookies; ++i)
    {
        free(cookies[i]);
    }
    json_value_free(root_value);

    // La etapa 3 se procedeaza asemanator cu etapa 2, mai adaugandu-se alte
    // cateva campuri.
    puts("\nETAPA 3\n");

    // Se salveaza in mod asemanator cookie-urile si se 
    // initializeaza obiectul JSON.
    nr_cookies = 0;
    parseCookies(response, cookies, &nr_cookies);
    toParse = strchr(response, '{');
    root_value = json_parse_string(toParse);
    if (json_value_get_type(root_value) != JSONObject)
    {
        json_value_free(root_value);
        return -1;
    }

    // Se extrag din nou valorile campurilor 
    EXTRACT_BASICINFO(fields, data, url, method, type);

    // Pentru fiecare element din data se extrage numele si valoarea.
    data_size = json_object_get_count(data);
    for (int i = 0;  i < data_size; ++i)
    {
        params[i] = json_object_get_name(data, i);
        values[i] = json_object_get_string(data, params[i]);

        // Daca valoarea este NULL inseamna ca mai este un nivel de imbricare
        // deci aici se afla campul qparams.
        if (values[i] == NULL)
        {
            JSON_Object * qparams = json_object_get_object(data, params[i]);
            char *stringConstruct = (char *)calloc(MAX_FORMSZ, 1);

            // Toate datele din qparams sunt luate si scrise sub forma de
            // parametru de url, adica: nume=valoare&...
            for (size_t j = 0; j < json_object_get_count(qparams); ++j)
            {
                strcat(stringConstruct, json_object_get_name(qparams, j));
                strcat(stringConstruct, "=");
                strcat(stringConstruct, json_object_get_string(
                        qparams, json_object_get_name(qparams, j)));
                if (j < json_object_get_count(qparams) - 1)
                {
                    strcat(form_data, "&");
                }
            }

            // La final datele formatate din qparams sunt puse in values[i]
            // aceea reprezentand valoarea lui qparams
            values[i] = stringConstruct;
        }
    }

    const char *token;
    char *answers = (char *)calloc(MAX_ANSWSZ, 1);

    // Se trece prin toate datele parse-ate si se cauta campurile token si
    // queryParams: daca se da de ele se salveaza valorile corespunzatoare,
    // altfel campurile o sa ramana NULL si nu o sa fie puse in mesaj
    for (int i = 0; i < data_size; ++i)
    {
        if (strcmp(params[i], "token") == 0)
        {
            token = values[i];
            continue;
        }
        if (strcmp(params[i], "queryParams") == 0)
        {
            sprintf(answers, "raspuns1=omul&raspuns2=numele&%s", values[i]);
            free((char *)values[i]);
        }
    }

    // Asemanator, se deschide o conexiune, se construieste mesajul se trimite
    // la server si se asteapta raspunsul care se afiseaza. La final se
    // elibereaza datele folosite.
    sockfd = open_connection(IP_SERVER, PORT_SERVER, AF_INET, SOCK_STREAM, 0);
    message = compute_any_request(IP_SERVER, method, url, answers, NULL,
                        type, (const char **) cookies, nr_cookies, token);
    puts(message);
    send_to_server(sockfd, message);
    free(response);
    response = receive_from_server(sockfd);
    close_connection(sockfd);
    puts(response);
    free(message);
    free(answers);
    for (int i = 0; i < nr_cookies; ++i)
    {
        free(cookies[i]);
    }
    
    // Se salveaza token-ul penru etapele urmatoare 
    char *tokenSave = (char *)calloc(MAX_TOKENSZ, sizeof(char));
    strcpy(tokenSave, token);
    json_value_free(root_value);
    token = tokenSave;

    // Etapa 4 
    puts("\nETAPA 4\n");

    // Si pentru aceasta etapa se extrag cookie-urile si campurile de la
    // inceputul fiecarui raspuns primit de la server
    nr_cookies = 0;
    parseCookies(response, cookies, &nr_cookies);
    toParse = strchr(response, '{');
    root_value = json_parse_string(toParse);
    if (json_value_get_type(root_value) != JSONObject)
    {
        json_value_free(root_value);
        return -1;
    }
    EXTRACT_BASICINFO(fields, data, url, method, type);
    
    sockfd = open_connection(IP_SERVER, PORT_SERVER, AF_INET, SOCK_STREAM, 0);
    message = compute_any_request(IP_SERVER, method, url, NULL, "", 
                type, (const char **) cookies, nr_cookies, token);
    puts(message);
    send_to_server(sockfd, message);
    free(response);
    response = receive_from_server(sockfd);
    close_connection(sockfd);
    puts(response);

    free(message);
    for (int i = 0; i < nr_cookies; ++i)
    {
        free(cookies[i]);
    }
    json_value_free(root_value);

    // Etapa 5 este putin mai lunga si este formata din 4 subparti. 
    // 1. - se parse-aza mesajul primit la etapa anterioara.
    // 2. - se interogheaza server-ul de DNS
    // 3. - se interogheaza server-ul de vreme
    // 4. - se trimit datele la server-ul temei
    puts("\nETAPA 5\n");
    nr_cookies = 0;
    parseCookies(response, cookies, &nr_cookies);
    toParse = strchr(response, '{');
    root_value = json_parse_string(toParse);
    if (json_value_get_type(root_value) != JSONObject)
    {
        json_value_free(root_value);
        return -1;
    }
    EXTRACT_BASICINFO(fields, data, url, method, type);
    
    char * toFree;

    // Se extrag datele din campul data si se formateaza ca mai inainte.
    data_size = json_object_get_count(data);
    for (int i = 0;  i < data_size; ++i)
    {
        params[i] = json_object_get_name(data, i);
        values[i] = json_object_get_string(data, params[i]);
        if (values[i] == NULL)
        {
            JSON_Object * qparams = json_object_get_object(data, params[i]);
            char *stringConstruct = (char *)calloc(MAX_FORMSZ, 1);
            toFree = stringConstruct;
            for (size_t j = 0; j < json_object_get_count(qparams); ++j)
            {
                strcat(stringConstruct, json_object_get_name(qparams, j));
                strcat(stringConstruct, "=");
                strcat(stringConstruct, json_object_get_string(
                        qparams, json_object_get_name(qparams, j)));
                if (j < json_object_get_count(qparams) - 1)
                {
                    strcat(stringConstruct, "&");
                }
            }
            values[i] = stringConstruct;
        }
    }

    const char *meteoUrl = "";
    const char *meteoMethod = "";
    const char *meteoType = "";
    const char *meteoUrlParams = "";

    // Se trece prin valorile care au fost parse-ate si se cauta elementele
    // de baza pentru interogarea server-ului meteo.
    for (int i = 0; i < data_size; ++i)
    {
        if (strcmp(params[i], "url") == 0)
        {
            meteoUrl = values[i];
            continue;
        }
        if (strcmp(params[i], "method") == 0)
        {
            meteoMethod = values[i];
            continue;
        }
        if (strcmp(params[i], "type") == 0)
        {
            meteoType = values[i];
            continue;
        }
        if (strcmp(params[i], "queryParams") == 0)
        {
            meteoUrlParams = values[i];
            continue;
        }
    }

    // Se copiaza numele site-ului pentru care se vrea IP-ul
    char *meteoDNS = (char *)calloc(MAX_DNSADDR, sizeof(char));
    for (int i = 0; meteoUrl[i] != '/'; ++i)
    {
        meteoDNS[i] = meteoUrl[i];
    }

    // Se interogheaza un server de DNS
    ip_port meteo = get_ip(meteoDNS);
    if (meteo.ip[0] == 0) 
    {
        puts("ERROR");
        return -1;
    }
    free(meteoDNS);
    meteo.port = HTTP_PORT;

    // Se deschide o conexiune cu server-ul meteo si se afiseaza mesajul
    // trimis/primit.
    sockfd = open_connection(meteo.ip, meteo.port, AF_INET, SOCK_STREAM, 0);
    message = compute_any_request(meteo.ip, meteoMethod, strchr(meteoUrl, '/'),
            meteoUrlParams, "", meteoType, (const char**) cookies, 0, NULL);
    puts(message);
    send_to_server(sockfd, message);
    free(response);
    response = receive_from_weather_server(sockfd);
    close_connection(sockfd);
    puts(response);
    free(message);
    free(toFree);

    // In final, se trimite raspunsul de la server-ul meteo mai departe la
    // adresa etapei 5
    sockfd = open_connection(IP_SERVER, PORT_SERVER, AF_INET, SOCK_STREAM, 0);
    message = compute_any_request(IP_SERVER, method, url, NULL, strchr(response, '{'),
                                    type, (const char **) cookies, nr_cookies, token);
    puts(message);
    send_to_server(sockfd, message);
    free(response);
    response = receive_from_server(sockfd);
    close_connection(sockfd);
    puts(response);
    for (int i = 0; i < nr_cookies; ++i)
    {
        free(cookies[i]);
    }
    free(tokenSave);
    free(response);
    free(message);
    json_value_free(root_value);
    return 0;
}
