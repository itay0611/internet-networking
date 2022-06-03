import socket
import threading
from datetime import datetime, timedelta

SERV1 = "192.168.0.101"
SERV2 = "192.168.0.102"
SERV3 = "192.168.0.103"
PORT = 80

MUSIC = 'M'
VIDEO = 'V'
PICTURE = 'P'

lock = threading.Lock()


def find_best_server(serv1_expect_time, serv2_expect_time, serv3_expect_time, type):
    times = [serv1_expect_time.replace(microsecond=0), serv2_expect_time.replace(microsecond=0), serv3_expect_time.replace(microsecond=0)]
    min_time = min(times)
    if times.count(min_time) == 1:
        return times.index(min_time)
    indices = [index for index, item in enumerate(times) if item == min_time]
    if type == MUSIC:
        return max(indices)
    return min(indices)



def get_message_type(data):
    if data[0] == MUSIC:
        return MUSIC
    if data[0] == VIDEO:
        return VIDEO
    return PICTURE


def handle_client(client_sock, client_address, servers_list, servers_ips, servers_empty_time):
    # receive the message from the client
    client_data = client_sock.recv(2).decode()  # we know that the message is 2 bytes
    message_type = get_message_type(client_data)
    message_time = int(client_data[1])

    # after the parsing of the message

    if message_type == MUSIC:
        music_server_execution_time = message_time
        video_server_execution_time = 2 * message_time
    elif message_type == VIDEO:
        video_server_execution_time = message_time
        music_server_execution_time = 3 * message_time
    else:  # PICTURE
        video_server_execution_time = message_time
        music_server_execution_time = 2 * message_time

    with lock:
        curr_time = datetime.now()
        # TODO: check if max works
        servers_empty_time[0] = max(servers_empty_time[0], curr_time)
        servers_empty_time[1] = max(servers_empty_time[1], curr_time)
        servers_empty_time[2] = max(servers_empty_time[2], curr_time)

    video_time_change = timedelta(seconds=video_server_execution_time)
    music_time_change = timedelta(seconds=music_server_execution_time)


    server_index = find_best_server(servers_empty_time[0] + video_time_change, servers_empty_time[1] + video_time_change, servers_empty_time[2] + music_time_change, message_type)
    with lock:
        if server_index == 2:
            servers_empty_time[2] = servers_empty_time[2] + music_time_change
        else:
            servers_empty_time[server_index] = servers_empty_time[server_index] + video_time_change

    print(" " + curr_time.strftime("%H:%M:%S") + ": received request " + client_data + " from " + client_address[0] + ", sending to " + servers_ips[server_index] + "-----")
    # print("from " + client_address[0] + ", sending to " + servers_ips[server_index] + "-----")

    # sending the message to the selected server
    servers_list[server_index].send(client_data.encode())

    # receiving the answer from the server
    answer = servers_list[server_index].recv(1024)

    # sending it to the client
    client_sock.send(answer)

    # closing the socket of the client
    client_sock.close()


def main():
    servers_ips = [SERV1, SERV2, SERV3]
    # connecting to the 3 servers
    serv1_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    serv2_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    serv3_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    print(": LB started-----lb1#")
    print(": Connecting to servers-----lb1#")

    serv1_socket.connect((SERV1, PORT))
    serv2_socket.connect((SERV2, PORT))
    serv3_socket.connect((SERV3, PORT))

    servers_list = [serv1_socket, serv2_socket, serv3_socket]
    servers_empty_time = []
    # curr_time = datetime.now().strftime("%H:%M:%S")
    curr_time = datetime.now()
    servers_empty_time += [curr_time] * 3


    # creating the listening socket

    listening_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    listening_sock.bind(("", PORT))
    listening_sock.listen(5)  # TODO: maybe need to add more size here
    threads = []
    while True:
        client_sock, address = listening_sock.accept()
        thr = threading.Thread(target=handle_client, args=(client_sock, address, servers_list, servers_ips, servers_empty_time))
        threads.append(thr)
        thr.start()
        # TODO: add join?

    listening_sock.close()



    serv1_socket.close()
    serv2_socket.close()
    serv3_socket.close()

if __name__ == "__main__":
    main()
