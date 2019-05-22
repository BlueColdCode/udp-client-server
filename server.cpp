#include <utility>
#include <iostream>
#include <string>
#include <map>

#include "server.hpp"

using namespace std;

typedef multimap<int, string> DataStore;
typedef DataStore::iterator   DataStoreIter;
typedef map<int, DataStore>   Clients;

#ifndef TEST
#define TEST 0
#endif

#if TEST
static int index = 0;
char const* dataStream[] = {
  "3:This is line 1",
  "2:This is line 22",
  "1:This is line 333",
  "1:This is line 44444",
  "3:This is line 1313131313131",
  "2:This is line 555555",
  "1:This is line 121212121212",
  "3:This is line 6666666",
  "1:This is line 7777777",
  "3:This is line 88888888",
  "1:This is line 999999999",
  "2:This is line 1010101010",
  "1:This is line 11111111111",
  "3:This is line 151515151515155",
  "2:This is line 14141414141414",
  "3:This is line 1",
  "2:This is line 22",
  "1:This is line 333",
  "2:This is line 44444",
  "1:This is line 1313131313131",
  "1:This is line 555555",
  "3:This is line 121212121212",
  "2:This is line 6666666",
  "1:This is line 7777777",
  "3:This is line 88888888",
  "3:",
  "2:This is line 999999999",
  "1:This is line 1010101010",
  "2:This is line 11111111111",
  "1:This is line 151515151515155",
  "1:",
  "2:This is line 14141414141414",
  "2:",
  0
};
#endif

class MessageServer {
public:
    MessageServer() { sockfd_ = OpenPort(0);}
    ~MessageServer() { ClosePort(sockfd_);}
    void Run() {
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
        }
        cout << "All messages done." << endl;
    }

private:
    // Returns socket fd for receiving packets.
    int OpenPort(int port) {
        // XXX socket();
        return 0;
    }
    
    // Clean up port.
    void ClosePort(int fd) {
        ; // XXX close(sockfd_);
    }
    
    // Retrieves one message from the socket, and save it in
    // buffer. The clientID is returned as a result.
    int GetMessage(int sockfd, int& cid, string& buffer) {
#if TEST
        if (dataStream[index]) {
            buffer = dataStream[index++];
            cid = stoi(buffer.substr(0, buffer.find_first_of(':')));
            int len = buffer.length() - buffer.find_first_of(':') - 1;

            cout << "Read (" << buffer.length() << "): " << buffer << endl;
            cout << "CID: " << cid << " Message length: " << len << endl;
            return len;
        } else {
            return -1;
        }
#endif
    }

    // Add a message into the sorted DataStore for the client. This
    // handles both initial message insertion and additional message
    // buildups.
    void AddClientMessage(int clientID, string& message) {
        clients_[clientID].insert(pair<int, string>(message.length(), message));
    }
    
    // Client has indicated the end of message stream, so wrap up and
    // output the messages to a file.
    void FinishClient(int cid) {
        // XXX print DataStore messages to file.
        cout << "Client " << cid << " finished messaging." << endl;
        int count = clients_[cid].size() * 9 / 10;
        for (DataStoreIter it = clients_[cid].begin(); it != clients_[cid].end(); it++) {
            count--;
            if (count <= 0) {
                break;
            }
            cout << it->second << endl;
        }
    }

    // Local database.
    Clients clients_;
    int sockfd_;
};

int main(int argc, char* argv[])
{
    MessageServer ms;
    ms.Run();
    return 0;
}
