// Copyright - Craciunoiu Cezar 324CA
#ifndef MESSAGE_TYPES
#define MESSAGE_TYPES

#define MSG_SIZE 1500
#define TOPIC_SIZE 50
#define TOTCP_METADATA 4
#define TOPIC_EXTENDED 52

typedef struct {
    char topic[TOPIC_SIZE];
    uint8_t tip_date;
    char continut[MSG_SIZE];
} commUDP;

typedef struct {
    char command[12];
    uint8_t SF;
    char topic[TOPIC_SIZE];
} commTCP;

typedef struct {
    uint8_t tip_date;
    uint8_t buffer = 0;
    uint16_t size = 0;
    char topic[TOPIC_SIZE + 2];
    struct sockaddr_in address;
    char continut[MSG_SIZE];
} toTCP;

typedef struct {
    uint8_t tip_date = 0;
    char sign;
    uint32_t data;
} INT;

typedef struct {
    uint8_t tip_date = 1;
    uint8_t buffer = 0;
    uint16_t data;
} SHORT_REAL;

typedef struct {
    uint8_t tip_date = 2;
    uint8_t buffer = 0;
    char sign;
    uint8_t power;
    uint32_t data;
} FLOAT;

typedef struct {
    uint8_t tip_date = 3;
    uint16_t string_size = 0;
    char str[MSG_SIZE + 1];
} STRING;

typedef struct {
    uint8_t tip_date;
    bool complete = true;
    struct sockaddr_in address;
    char topic[TOPIC_EXTENDED];
} metaData;

typedef struct {
    int sockfd;
    bool online = true;
    uint8_t SF;
    uint16_t nrMsg = 0;
} clientData;
#endif //MESSAGE_TYPES
