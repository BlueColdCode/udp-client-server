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

#include "client.hpp"

const int MAX_MESSAGE_LEN = 1024;

MessageClient::MessageClient(char* server, int port, int id):
    serverName_(server), port_(port), clientID_(id)
{
    if (OpenPort()) {
        cerr << "Message client init error." << endl;
        exit(-1);
    }
}

MessageClient::~MessageClient()
{
    ClosePort();
}

int MessageClient::OpenPort()
{
    // Creating socket file descriptor
    if ((sockfd_ = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failed");
        return -1;
    }
   
    struct sockaddr_in cliaddr = {};
    cliaddr.sin_family      = AF_INET; // IPv4
    cliaddr.sin_addr.s_addr = INADDR_ANY;
    cliaddr.sin_port        = 0;

    if (bind(sockfd_, (struct sockaddr *)&cliaddr, sizeof(cliaddr)) < 0) {
        cerr << "bind failed" << errno << endl;
        return -2;
    }
   
    cout << "Client port opened to messages." << endl;
    return 0;
}

void MessageClient::ClosePort()
{
    if (sockfd_ > 0) {
        cout << "Closing port" << endl;
        close(sockfd_);
    }
}

int MessageClient::GetFileName(string& filename)
{
    stringstream ss;
    ss << "./client-data/messages-" << clientID_ << ".txt";
    filename = ss.str();
    return 0;
}

void MessageClient::Run()
{
    // Get server sockaddr
    struct sockaddr_in servaddr = {};
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port_);

    struct hostent *he = gethostbyname(serverName_.c_str());
    if (!he) {
        cout << "Could not obtain address of " << serverName_ << endl;
        return;
    }
    memcpy((void *)&servaddr.sin_addr, he->h_addr_list[0], he->h_length);

    // Create predefined client filename.
    string filename;
    GetFileName(filename);
    ifstream ifs(filename.c_str(), ifstream::in);

    // Message buffer for each line of the file.
    string buffer;
    int n, len;
    stringstream pref;
    pref << clientID_ << ":";
    while (getline(ifs, buffer)) {
        buffer.insert(0, pref.str());
        cout << "Sending: " << buffer << ">" << endl;
        sendto(sockfd_, buffer.c_str(), buffer.length(),
            MSG_CONFIRM, (const struct sockaddr *) &servaddr,
            sizeof(servaddr));
        // Random delay before sending the next.
        usleep((rand() % 1000000) + 1);
    }
    // Last message only includes the prefix.
    buffer = pref.str();
    sendto(sockfd_, buffer.c_str(), buffer.length(),
        MSG_CONFIRM, (const struct sockaddr *) &servaddr,
        sizeof(servaddr));
    
    // clean up.
    ifs.close();
}

int main(int argc, char* argv[])
{
    if (argc != 4) {
        cout << "Usage: " << argv[0]
             << " <server name/ip> <server port> <client ID>"
             << endl;
        return -1;
    }

    unsigned int cid = stoi(argv[3]);
    srand(cid);
    
    MessageClient mc(argv[1], stoi(argv[2]), cid);
    mc.Run();
    return 0;
}
