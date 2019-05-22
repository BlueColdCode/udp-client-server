#include <utility>
#include <iostream>
#include <string>
#include <map>
#include <vector>
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

int MessageServer::GetMessage(int sockfd, int& cid, string& buffer)
{
#if TEST
    if (dataStream[g_index]) {
        buffer = dataStream[g_index++];
        cid = stoi(buffer.substr(0, buffer.find_first_of(':')));
        int len = buffer.length() - buffer.find_first_of(':') - 1;

        cout << "Read (" << buffer.length() << "): " << buffer << endl;
        cout << "CID: " << cid << " Message length: " << len << endl;
        return len;
    } else {
        return -1;
    }
    
#else

    vector<char> buff(MAX_MESSAGE_LEN);
    struct sockaddr_in cliaddr;
    int n;
    socklen_t len;
    
    n = recvfrom(sockfd_, &buff[0], buff.size(), MSG_WAITALL,
        (struct sockaddr *)&cliaddr, &len); 
    buff[n] = '\0';
    buffer.append(buff.begin(), buff.end()); // copy over.
    return n;
#endif
}

void MessageServer::AddClientMessage(int clientID, string& message)
{
    clients_[clientID].insert(pair<int, string>(message.length(), message));
}

void MessageServer::FinishClient(int cid)
{
    // XXX print DataStore messages to file.
    cout << "Client " << cid << " finished messaging." << endl;

    stringstream ss;
    ss << "./server-data/messages-" << cid << ".txt";
    ofstream ofs (ss.str(), ofstream::out);
    int count = clients_[cid].size() * 9 / 10;
    for (DataStoreIter it = clients_[cid].begin();
         it != clients_[cid].end(); it++) {
        if (count-- <= 0) {
            break;
        }
        ofs << it->second << endl;
    }
    ofs.close();
}

void MessageServer::Run()
{
    string buffer;
    int cid, len;
    for (len = GetMessage(sockfd_, cid, buffer);
         len != -1;
         len = GetMessage(sockfd_, cid, buffer)) {
        if (len == 0) {
            FinishClient(cid);
        } else {
            AddClientMessage(cid, buffer);
        }
        buffer = "";
    }
    cout << "All messages done." << endl;
}

int main(int argc, char* argv[])
{
    if (argc != 2) {
        cout << "Usage: " << argv[0] << " <port>" << endl;
        return -1;
    }
    
    MessageServer ms(stoi(argv[1]));
    ms.Run();
    return 0;
}
