// Copyright - Craciunoiu Cezar 324CA
#ifndef TOPIC
#define TOPIC

#include <deque>
#include <string>
#include <map>

// Clasa topic pentru gestionarea mesajelor primite respectiv retinerea lor
class Topic {
 private:
    std::string topicName;
    std::deque<std::pair<std::pair<uint8_t, struct sockaddr_in>, std::string>>* store;
    std::map<std::string, clientData>* clients;

 public:
    // Constructor gol
    Topic(std::string topicName) {
        this->topicName = topicName;
        store = new std::deque<std::pair<std::pair<uint8_t, struct sockaddr_in>, std::string>>();
        clients = new std::map<std::string, clientData>();
    }

    // Constructor pentru cand cererea de topic nou a venit de la un client UDP
    Topic(std::string topicName, std::string& firstMsg, 
          uint8_t firstType, struct sockaddr_in address) {
        this->topicName = topicName;
        store = new std::deque<std::pair<std::pair<uint8_t, struct sockaddr_in>, std::string>>();
        clients = new std::map<std::string, clientData>();
        store->push_back({{firstType, address}, firstMsg});
    }
    // Constructor pentru cand cererea de topic nou a venit de la un client TCP
    Topic(std::string topicName, std::string clientId, int fdClient, uint8_t SF) {
        this->topicName = topicName;
        store = new std::deque<std::pair<std::pair<uint8_t, struct sockaddr_in>, std::string>>();
        clients = new std::map<std::string, clientData>();
        clientData clientAux;
        clientAux.sockfd = fdClient;
        clientAux.online = true;
        clientAux.SF = SF;
        clients->insert({clientId, clientAux});
    }

    ~Topic(){
        delete store;
        delete clients;
    }

    // Functie ce adauga un client si initializeaza ultimul mesaj primit cu cel
    // curent, adica dimensiunea lui store
    int addClient(std::string clientId, int fdClient, uint8_t SF) {
        if (clients->count(clientId) == 0) {
            clientData clientAux;
            clientAux.sockfd = fdClient;
            clientAux.online = true;
            clientAux.SF = SF;
            clientAux.nrMsg = store->size();
            clients->insert({clientId, clientAux});
        } else {
            return -1;
        }
        return 0;
    }

    // Elimina un client din topic
    int removeClient(std::string clientId) {
        if (clients->count(clientId) <= 0) {
            return -1;
        } else {
            clients->erase(clientId);
        }
        return 0;
    }

    // Adauga un mesaj la topic
    void addMsg(std::string& toAdd, uint8_t toAddType, 
                struct sockaddr_in address) {
        store->push_back({{toAddType, address}, toAdd});
    }

    // Modifica starea unui client - Online/Offline
    void setStatusClient(std::string client, bool status) {
        clientData aux = clients->at(client);
        aux.online = status;
        clients->erase(client);
        clients->insert({client, aux});
    }

    // Creste numarul de ordine ale mesajelor ale unui client
    void incrementClient(std::string client) {
        clientData aux = clients->at(client);
        aux.nrMsg++;
        clients->erase(client);
        clients->insert({client, aux});
    }

    std::deque<std::pair<std::pair<uint8_t, struct sockaddr_in>, std::string>>* getStore() {
        return store;
    }

    std::string getTopicName() {
        return topicName;
    }

    std::map<std::string, clientData>* getClients() {
        return clients;
    }
};

#endif // TOPIC
