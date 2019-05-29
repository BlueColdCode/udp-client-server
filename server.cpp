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
#include <chrono>
#include <condition_variable>

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
int messageCount = 0;

// Total threads 6.
const int thread_count = 3;

// Single writer in Run(). All others are read.
bool job_done = false;

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
int MessageServer::GetMessage(string& buffer)
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
    lock_guard<mutex> lock(storeLock_);
    clients_[clientID].insert(pair<int, string>(message.length(),
            move(message)));
}

void MessageServer::FinishClient(int cid)
{
    // print DataStore messages to file.
    // cout << "Client " << cid << " finished messaging." << endl;
    DataStore clientDataStore;
    do {
        lock_guard<mutex> lock(storeLock_);
        clientDataStore = move(clients_[cid]);
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
    while(!job_done) {
        string buffer;

        if (1) {
            unique_lock<mutex> lock(messageLock_);
            if (messageQ_.empty()) {
                // Wait for work to arrive.
                work_to_do_.wait(lock);
                continue;
            }
            // Copy out the head-of-line.
            buffer = move(messageQ_.front());
            messageQ_.pop();
        }
        
        // Message parsing and storing.
        int cid = stoi(buffer.substr(0, buffer.find_first_of(':')));
        // cout << "Received (" << buffer.length() << ") from "
        //     << cid << " --|" << buffer << "|--" << endl;
        buffer.erase(0, buffer.find_first_of(':') + 1);

        if (buffer.length() == 0) {
            FinishClient(cid);
        } else {
            AddClientMessage(cid, buffer);
        }
    }
}

// This will be the last thread to exit.
void MessageServer::Speedometer()
{
    chrono::seconds sec(10);
    int lastCount = messageCount;
    
    while (!job_done) {
        this_thread::sleep_for(sec);
        cout << "Message processing rate: "
             << (messageCount - lastCount) / 10.0 << " msg/sec" << endl;
        lastCount = messageCount;
    }
}

void MessageServer::Run()
{
    string buffer;

    while(GetMessage(buffer) != -1) {
        messageCount++;
        // Critical section.
        {
            lock_guard<mutex> lock(messageLock_);
            messageQ_.push(move(buffer));
        }
        work_to_do_.notify_all();
    }
    cout << "All messages done." << endl;

    // Wait until all messages are processed.
    while(1) {
        usleep(1000);
        if (!job_done) {
            lock_guard<mutex> lock(messageLock_);
            if (messageQ_.empty()) {
                job_done = true;
                work_to_do_.notify_all();
                break;
            }
        }
    }
}

int main(int argc, char* argv[])
{
    if (argc != 2) {
        cout << "Usage: " << argv[0] << " <port>" << endl;
        return -1;
    }

    MessageServer ms(stoi(argv[1]));
    thread t[thread_count+2];
    
    // UDP message parser/storer/saver.
    for (int i = 0; i < thread_count; i++) {
        t[i] = thread(&MessageServer::ProcessMessage, &ms);
    }
    // UDP message receiver.
    t[thread_count] = thread(&MessageServer::Run, &ms);
    t[thread_count+1] = thread(&MessageServer::Speedometer, &ms);

    for (int i = 0; i < thread_count+2; i++) {
        t[i].join();
    }
    return 0;
}
