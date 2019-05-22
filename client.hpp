#ifndef CLIENT_HPP_
#define CLIENT_HPP_

class MessageClient {
public:
    MessageClient(char* server, int port, int id);
    ~MessageClient();
    void Run();

private:
    int OpenPort();
    void ClosePort();
    int GetFileName(string& filename);

    // Local database.
    int sockfd_;
    int clientID_;
    int port_;
    string serverName_;
};

#endif // CLIENT_HPP_
