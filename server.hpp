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

    // Thread to get UDP packets from socket.
    void Run();
    // Thread to process UDP message from a FIFO queue.
    void ProcessMessage();
    // Thread to print message proc-rate every 10 seconds.
    void Speedometer();

private:
    // Opens socket fd for receiving packets.
    int OpenPort(int port);
    // Clean up socket.
    void ClosePort();

    // Retrieves one message from the socket, and save it in
    // buffer. The buffer length is returned as a result.
    int GetMessage(string& buffer);
    
    // Add a message into the sorted DataStore for the client. This
    // handles both initial message insertion and additional message
    // buildups.
    void AddClientMessage(int clientID, string& message);
    
    // Client has indicated the end of message stream, so wrap up and
    // output the messages to a file.
    void FinishClient(int cid);

    // Message FIFO queue
    queue<string> messageQ_;
    mutex messageLock_;
    condition_variable work_to_do_;
    
    // Local database.
    Clients clients_;
    mutex storeLock_;
    int sockfd_;
};

#endif // SERVER_HPP_
