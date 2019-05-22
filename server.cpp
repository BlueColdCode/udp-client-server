#include <utility>
#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <fstream>
#include <sstream>

#include <sys/types.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <memory.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <errno.h>
#include <stdlib.h>

using namespace std;

#include "server.hpp"
#include "server.test"

const int MAX_MESSAGE_LEN = 1024;

// Total threads 12.
const int thread_count = 10;

// Single writer in Run(). All others are read.
int job_done = false;

MessageServer::MessageServer(int port)
{
    if (OpenPort(port)) {
        cerr << "Message server init error." << endl;
    }
}

MessageServer::~MessageServer()
{
    ClosePort();
}

int MessageServer::OpenPort(int port)
{
    // Creating socket file descriptor
    if ((sockfd_ = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        cerr << "Socket creation failed" << endl;
        return -1;
    }

    struct sockaddr_in servaddr = {};
    servaddr.sin_family      = AF_INET; // IPv4
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port        = htons(port);

    if (bind(sockfd_, (const struct sockaddr *)&servaddr,
            sizeof(servaddr)) < 0 ) {
        cerr << "Bind failed: " << errno << endl;
        return -2;
    }

    cout << "Server port opened for messages." << endl;
    return 0;
}

void MessageServer::ClosePort()
{
    if (sockfd_ > 0) {
        cout << "Closing port" << endl;
        close(sockfd_);
    }
}

// Return -1 on simulation end. Otherwise always return 1 for server.
int MessageServer::GetMessage(int& cid, string& buffer)
{
    int n;

#if TEST
    if (dataStream[g_index]) {
        buffer = dataStream[g_index++];
    } else {
        return -1;
    }
#else
    {
        vector<char> buff(MAX_MESSAGE_LEN);
        struct sockaddr_in cliaddr;
        socklen_t len;
        
        n = recvfrom(sockfd_, &buff[0], buff.size(), MSG_WAITALL,
            (struct sockaddr *)&cliaddr, &len); 
        buff[n] = '\0';
        buffer.append(&buff[0]); // copy over.
    }
#endif
    return 1;
}

void MessageServer::AddClientMessage(int clientID, string& message)
{
    std::lock_guard<std::mutex> lock(storeLock_);
    clients_[clientID].insert(pair<int, string>(message.length(), message));
}

void MessageServer::FinishClient(int cid)
{
    // print DataStore messages to file.
    cout << "Client " << cid << " finished messaging." << endl;
    DataStore clientDataStore;
    do {
        lock_guard<mutex> lock(storeLock_);
        clientDataStore = clients_[cid];
        clients_.erase(cid);
    } while (0);
    
    stringstream ss;
    ss << "./server-data/messages-" << cid << ".txt";
    ofstream ofs (ss.str(), ofstream::out);
    int count = clientDataStore.size() * 9 / 10;
    for (DataStoreIter it = clientDataStore.begin();
         it != clientDataStore.end(); it++) {
        if (count-- <= 0) {
            break;
        }
        ofs << it->second << endl;
    }
    ofs.close();
}

// thread function.
void MessageServer::ProcessMessage()
{
    bool work_to_do = true;
    
    while(!job_done) {
        string buffer;

        if (work_to_do) {
            // Critical section.
            lock_guard<mutex> lock(messageLock_);
            if (messageQ_.empty()) {
                work_to_do = false;
                continue;
            }
            // Copy out the head-of-line.
            buffer = messageQ_.front();
            messageQ_.pop();
        } else {
            usleep(5);
            work_to_do = true; // try again.
            continue;
        }
        
        // Message parsing and storing.
        int cid = stoi(buffer.substr(0, buffer.find_first_of(':')));
        cout << "Received ("
             << buffer.length() << ") from "
             << cid << " --|"
             << buffer << "|--" << endl;
        buffer.erase(0, buffer.find_first_of(':') + 1);

        if (buffer.length() == 0) {
            FinishClient(cid);
        } else {
            AddClientMessage(cid, buffer);
        }
    }
}

void MessageServer::Run()
{
    string buffer;
    int cid;
    while(GetMessage(cid, buffer) != -1) {
        // Critical section.
        lock_guard<mutex> lock(messageLock_);
        messageQ_.push(buffer);
        buffer = "";
    }
    cout << "All messages done." << endl;

    // Wait until all messages are processed.
    do {
        lock_guard<mutex> lock(messageLock_);
        if (messageQ_.empty()) {
            break;
        }
        usleep(1000);
    } while (1);
    job_done = true;
}

int main(int argc, char* argv[])
{
    if (argc != 2) {
        cout << "Usage: " << argv[0] << " <port>" << endl;
        return -1;
    }

    MessageServer ms(stoi(argv[1]));
    thread t[thread_count+1];
    
    // UDP message parser/storer/saver.
    for (int i = 0; i < thread_count; i++) {
        t[i] = thread(&MessageServer::ProcessMessage, &ms);
    }
    // UDP message receiver.
    t[thread_count] = thread(&MessageServer::Run, &ms);

    for (int i = 0; i < thread_count+1; i++) {
        t[i].join();
    }
    return 0;
}
