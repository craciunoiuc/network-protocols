// Copyright - Craciunoiu Cezar 324CA
#ifndef MESSAGEPARSER
#define MESSAGEPARSER

#include <vector>
#include <string>
#include <cstring>
#include <cstdlib>
#include "./messageTypes.h"

class MessageParser {
 private:
    MessageParser(){}

 public:
    static std::vector<std::pair<metaData, std::string>> parse(std::string toParse, uint32_t len) {
        std::vector<std::pair<metaData, std::string>> parsed;

        // Daca a venit un mesaj eronat (continutul <= 1) se ignora
        if (len < TOTCP_METADATA + TOPIC_EXTENDED + sizeof(struct sockaddr_in) + 1) {
            return parsed;
        }
        uint32_t i = 0;
        do {
            metaData data;
            memset(&data, 0, sizeof(metaData));
            memcpy(&data.address, std::string(toParse, i + TOTCP_METADATA + TOPIC_EXTENDED, 
                            sizeof(struct sockaddr_in)).c_str(), sizeof(struct sockaddr_in));
            data.tip_date = toParse[i];
            memcpy(data.topic, std::string(toParse, i + TOTCP_METADATA, TOPIC_EXTENDED).c_str(),
                    TOPIC_EXTENDED);
            switch(toParse[i]) {
                // Se desparte mesajul de tip int de restul
                case 0: {
                    if (len - sizeof(INT) > i) {
                        parsed.push_back({data, std::string(toParse, i + 
                            TOTCP_METADATA + TOPIC_EXTENDED + sizeof(struct sockaddr_in), 
                            sizeof(uint32_t) + sizeof(char))});
                    }
                    i += sizeof(INT) + TOTCP_METADATA + 
                            TOPIC_EXTENDED + sizeof(struct sockaddr_in);
                    break;
                }
                // Se desparte mesajul de tip short_real de restul
                case 1: {
                    parsed.push_back({data, std::string(toParse, i + 
                        TOTCP_METADATA + TOPIC_EXTENDED + 
                        sizeof(struct sockaddr_in), sizeof(uint16_t))});
                    i += sizeof(SHORT_REAL) + TOTCP_METADATA + 
                        TOPIC_EXTENDED + sizeof(struct sockaddr_in);
                    break;
                }
                //  Se desparte mesajul de tip float de restul celor venite
                case 2: {
                    parsed.push_back({data, std::string(toParse, i + 
                        TOTCP_METADATA + TOPIC_EXTENDED + sizeof(struct sockaddr_in), 
                        sizeof(uint32_t) + sizeof(uint8_t) + sizeof(char))});
                    i += sizeof(FLOAT) + TOTCP_METADATA + TOPIC_EXTENDED + 
                         sizeof(struct sockaddr_in);
                    break;
                }
                // Daca este string se construieste mai intai dimensiunea lui
                // si apoi se construeiste si stringul in functie de dimensiune
                // Daca cumva TCP a segmentat un mesaj (a dat in doua), atunci
                // se verifica daca se mai asteapta date sau nu (daca se pune)
                // \n sau nu.
                case 3: {
                    uint16_t size;
                    memcpy(&size, (toParse.c_str() + i + 2), sizeof(uint16_t));
                    if (size > len - i - TOTCP_METADATA - TOPIC_EXTENDED - 1) {
                        data.complete = false;
                    } else {
                        data.complete = true;
                    }
                    parsed.push_back({data, std::string(toParse, i + TOTCP_METADATA + 
                        TOPIC_EXTENDED + sizeof(struct sockaddr_in), 
                        size - sizeof(struct sockaddr_in)).append("\0")});
                    i += size + TOTCP_METADATA + TOPIC_EXTENDED;
                    break;
                }
                // A venit restul dintr-un mesaj segmentat, se adauga.
                default: {
                    metaData defaultData;
                    defaultData.tip_date = 4;
                    char restOfMessage[MSG_SIZE];
                    int j = 0;
                    while (toParse[i] != 0 && toParse[i] != 1 && 
                            toParse[i] != 2 && toParse[i] != 3) {
                        restOfMessage[j] = toParse[i];
                        j++;
                        i++;
                    }
                    restOfMessage[j] = 0;
                    parsed.push_back({defaultData, std::string(restOfMessage, 0, j).append("\0")});
                    break;
                }
            }
            i++;
        } while(i < len);
        return parsed;
    }

    // Construeiste INT-ul in format human-readable
    static INT parseINT(std::string& toParse) {
        INT aux;
        aux.tip_date = 0;
        aux.sign = toParse[0];
        memcpy(&aux.data, std::string(toParse.begin() + 1, toParse.end()).c_str(), 
               sizeof(uint32_t));
        aux.data = ntohl(aux.data);
        return aux;
    }

    // Construieste SHORT_REAL-ul in format human-readable
    static SHORT_REAL parseSHORT_REAL(std::string& toParse) {
        SHORT_REAL aux;
        memcpy(&aux.data, toParse.c_str(), sizeof(uint16_t));
        aux.data = ntohs(aux.data);
        return aux;
    }

    // Functia construieste FLOAT-ul in format human-readable
    static FLOAT parseFLOAT(std::string& toParse) {
        FLOAT aux;
        aux.sign = toParse[0];
        aux.power = toParse.back();
        memcpy(&aux.data, std::string(toParse.begin() + 1, toParse.end() - 1).c_str(),
                 sizeof(uint32_t));
        aux.data = ntohl(aux.data);
        return aux;
    }

    // Functie ce construieste un STRING
    static STRING parseSTRING(std::string& toParse) {
        STRING aux;
        aux.string_size = toParse.length();
        memcpy(&aux.str, toParse.c_str(), aux.string_size + 1);
        return aux;
    }

    // Construieste pe 10^n
    static float raisePower(int n) {
        float result = 1.0;
        while(n--)
            result *= 10;
        return result;
    }
};

#endif //MESSAGEPARSER
