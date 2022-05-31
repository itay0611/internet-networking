#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <cstdlib>
#include <algorithm>
#include <initializer_list>
#include <ctime>
#include <chrono>
#include <vector>
#include <time.h>
#include <thread>
#include <mutex>

#define SERV1 "192.168.0.101"
#define SERV2 "192.168.0.102"
#define SERV3 "192.168.0.103"
#define PORT 80
#define BUFFER_LEN 256

#define MUSIC 'M'
#define VIDEO 'V'
#define PICTURE 'P'

using std::cout;
using std::cerr;
using std::endl;
using std::cin;
using std::vector;
using std::localtime;
using std::time_t;

bool availableThreads[5] = {true, true, true, true, true};
std::mutex mtx;
const std::string servers_ip[3] = {"192.168.0.101", "192.168.0.102", "192.168.0.103"};

int getAvailableThread() {
    for (int i = 0; i < 5; i++) {
        if (availableThreads[i]) {
            return i;
        }
    }
    return -1;
}

void error(const char *msg) {
    cerr << msg << endl;
    exit(0);
}

int findBestServer(double server1_expect_time, double server2_expect_time, double server3_except_time) {
    if (server1_expect_time < server2_expect_time) {
        if (server1_expect_time < server3_except_time) {
            return 0;
        }
        else {
            return 2;
        }
    }
    else {
        if (server2_expect_time < server3_except_time) {
            return 1;
        }
        else {
            return 2;
        }
    }
}

void message_handler(int sockfd, vector<time_t>& servers_empty_times, vector<int>& servers_fds, int thread_idx) {
    int message_time, video_server_execution_time, music_server_execution_time;
    char message_type;
    time_t curr_time;
    char buf[2];
    char answer[4096];

    struct sockaddr_in addr;
    socklen_t addr_size = sizeof(struct sockaddr_in);
    int res = getpeername(sockfd, (struct sockaddr *)&addr, &addr_size);
    // TODO: add recv from client, find out which server to send to, send to server, recv answer from server, send to client

    // TODO: add the recv
    if (recv(sockfd, buf, sizeof(buf), 0) < 0) {
        error("Could not receive message");
    }
    message_type = buf[0];
    message_time = buf[1];

    // AFTER THE PARSING OF THE MESSAGE

    switch(message_type) {
        case VIDEO:
            video_server_execution_time = message_time;
            music_server_execution_time = 3 * message_time;
            break;
        case MUSIC:
            music_server_execution_time = message_time;
            video_server_execution_time = 2 * message_time;
            break;
        case PICTURE:
            video_server_execution_time = message_time;
            music_server_execution_time = 2 * message_time;
            break;
        default:
            // should not get here
            error("UNDEFINED MESSAGE");
            break;
    }
    // TODO: lock
    mtx.lock();
    time(&curr_time);
    servers_empty_times[0] = std::max(servers_empty_times[0],curr_time);
    servers_empty_times[1] = std::max(servers_empty_times[1],curr_time);
    servers_empty_times[2] = std::max(servers_empty_times[2],curr_time);

    int index = findBestServer(servers_empty_times[0] + video_server_execution_time, servers_empty_times[1] + video_server_execution_time, servers_empty_times[2] + music_server_execution_time);
    if(index == 2) {
        servers_empty_times[index] += music_server_execution_time;
    } else {
        servers_empty_times[index] += video_server_execution_time;
    }
    cout << " " << curr_time << ": recieved request " << message_type << message_time;
    cout << " from " << inet_ntoa(addr.sin_addr) << ", sending to " << servers_ip[i] << "-----" << endl;
    // TODO: unlock
    mtx.unlock();

    cout << " " << curr_time <<
    // TODO: send the message to the best server using server_fds[index]
    if (send(servers_fds[index], buf, sizeof(buf), 0) < 0) {
        error("Can't send the message to the server");
    }

    // TODO: recv the answer from the server
    if (recv(servers_fds[index], answer, sizeof(answer), 0) < 0) {
        error("Can't receive the message from the server");
    }

    // TODO: send the answer to the client
    if (send(sockfd, answer, sizeof(answer), 0) < 0) {
        error("Can't send the message to the client");
    }

    // lock
    mtx.lock();
    availableThreads[thread_idx] = true;
    // unlock
    mtx.unlock();
    close(sockfd);
}


int main() {
    int serv1_sock, serv2_sock, serv3_sock;
    int video_server_execution_time;
    int music_server_execution_time;
    struct sockaddr_in serv1_addr, serv2_addr, serv3_addr;

    time_t server1_empty_time;
    time(&server1_empty_time);
    time_t server2_empty_time = server1_empty_time;
    time_t server3_empty_time = server1_empty_time;
    time_t curr_time;
    time(&curr_time);
    cout << curr_time << ": LB started-----lb1#" << endl;
    int message_time;
    char message_type;
    vector<int> servers_fds;
    vector<time_t> servers_empty_times = {server1_empty_time, server2_empty_time, server3_empty_time};
    cout << curr_time << ": Connecting to servers-----lb1#" << endl;
    serv1_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (serv1_sock < 0) {
        error("ERROR opening socket");
    }

    serv2_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (serv2_sock < 0) {
        error("ERROR opening socket");
    }

    serv3_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (serv3_sock < 0) {
        error("ERROR opening socket");
    }

    serv1_addr.sin_family = AF_INET;
    serv2_addr.sin_family = AF_INET;
    serv3_addr.sin_family = AF_INET;

    serv1_addr.sin_port = htons(PORT);
    serv2_addr.sin_port = htons(PORT);
    serv3_addr.sin_port = htons(PORT);

    inet_aton(SERV1, &serv1_addr.sin_addr.s_addr);
    inet_aton(SERV2, &serv2_addr.sin_addr.s_addr);
    inet_aton(SERV3, &serv3_addr.sin_addr.s_addr);

    if(connect(serv1_sock, serv1_addr, sizeof(serv1_addr)) < 0) {
        error("ERROR connecting");
    }
    if(connect(serv2_sock, serv2_addr, sizeof(serv2_addr)) < 0) {
        error("ERROR connecting");
    }
    if(connect(serv3_sock, serv3_addr, sizeof(serv3_addr)) < 0) {
        error("ERROR connecting");
    }

    servers_fds.push_back(serv1_sock);
    servers_fds.push_back(serv2_sock);
    servers_fds.push_back(serv3_sock);

    // now we are connected to the 3 servers

    int listen_sock_fd, newsockfd;
    socklen_t client_len;
    char buffer[BUFFER_LEN];
    struct sockaddr_in listen_addr, client_addr;
    listen_sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sock_fd < 0) {
        error("ERROR opening socket");
    }
    listen_addr.sin_family = AF_INET;
    listen_addr.sin_addr.s_addr = INADDR_ANY;
    listen_addr.sin_port = htons(PORT);

    if(bind(listen_sock_fd, listen_addr, sizeof(listen_addr)) < 0) {
        error("ERROR on binding");
    }

    listen(listen_sock_fd, 5);

    std::thread myThreads[5];

    //int i = 0;

    while(true) {
        cout << "lb1#";
        client_len = sizeof(client_addr);

        newsockfd = accept(listen_sock_fd, client_addr, client_len);
        if(newsockfd < 0) {
            error("ERROR on accept");
        }

        // TODO: lock
        mtx.lock();
        int t_index = getAvailableThread();
        if(t_index < 0) {
            error("No available threads");
        }
        availableThreads[t_index] = false;
        // TODO: unlock
        mtx.unlock();
        myThreads[t_index] = std::thread(message_handler, newsockfd, servers_empty_times, servers_fds, t_index);



        // TODO: maybe add join

        //i++;
    }

    for(int i = 0; i < 0; i++) {
        close(servers_fds[i]);
    }

    return 0;
}

