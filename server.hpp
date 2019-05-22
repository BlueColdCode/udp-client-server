#ifndef SERVER_HPP_
#define SERVER_HPP_

using namespace std;

typedef multimap<int, string> DataStore;
typedef DataStore::iterator   DataStoreIter;
typedef map<int, DataStore>   Clients;

class MessageServer {
public:
    MessageServer(int port);
    ~MessageServer();
    void Run();

private:
    // Opens socket fd for receiving packets.
    int OpenPort(int port);
    // Clean up socket.
    void ClosePort();

    // Retrieves one message from the socket, and save it in
    // buffer. The clientID is returned as a result.
    int GetMessage(int sockfd, int& cid, string& buffer);
    
    // Add a message into the sorted DataStore for the client. This
    // handles both initial message insertion and additional message
    // buildups.
    void AddClientMessage(int clientID, string& message);
    
    // Client has indicated the end of message stream, so wrap up and
    // output the messages to a file.
    void FinishClient(int cid);
    
    // Local database.
    Clients clients_;
    int sockfd_;
};

#endif // SERVER_HPP_
