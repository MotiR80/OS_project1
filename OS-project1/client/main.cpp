#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/time.h>

int currPage;
int role; //1.student:asker 2.TA 3.student:listener
bool connection_state = true;


int connectServer(int);
bool connectClient(int);
void alarm_handler(int);

int main(int argc, char const *argv[]) {
    char buff[1024] = {0};

    if (argc != 1){
        sprintf(buff, "You should enter 1 parameter, but %d.\n", argc);
        write(1, buff, strlen(buff));
        exit(0);
    }

    int fd, port;
    int room_port;


    port = atoi(argv[0]);
    fd = connectServer(8888);
    read(fd, buff, 1024);
    write(1, buff, strlen(buff));
    read(fd, buff, 1024);
    currPage = atoi(buff);

    ssize_t nread;
    while (1) {
        nread = read(0, buff, 1024);
        buff[nread-1] = '\0';

        if (currPage == 1)
            role = atoi(buff);

        send(fd, buff, strlen(buff), 0);
        nread = read(fd, buff, 1024);

        buff[nread] = '\0';
        room_port = atoi(buff);
        write(1, buff, strlen(buff));

        read(fd, buff, 1024);
        currPage = atoi(buff);

        if (currPage == 4){
            if (role == 1)
                if (atoi(buff) == 1)
                    role = 1;
                else if (atoi(buff) == 2)
                    role = 3;
        }
        else if (currPage == 8){
            connectClient(room_port);
            send(fd, buff, strlen(buff), 0);
            nread = read(fd, buff, 1024);
            buff[nread] = '\0';
            write(1, buff, strlen(buff));
            read(fd, &currPage, sizeof(currPage));
        }
        else if (currPage == 9){
            nread = read(fd, buff, 1024);
            buff[nread] = '\0';
            bool state = connectClient(atoi(buff));
            if (state) {
                strcpy(buff, "yes");
                send(fd, buff, strlen(buff), 0);
            }
            else{
                strcpy(buff, "no");
                send(fd, buff, strlen(buff), 0);
            }
        }



    }

}

int connectServer(int port) {
int fd;
char buff[100];
struct sockaddr_in server_address;

fd = socket(AF_INET, SOCK_STREAM, 0);

server_address.sin_family = AF_INET;
server_address.sin_port = htons(port);
server_address.sin_addr.s_addr = inet_addr("127.0.0.1");

if (connect(fd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) { // checking for errors
strcpy(buff, "Error in connecting to server\n");
write(1, buff, strlen(buff));
exit(0);
}
else{
strcpy(buff, "Connected to server successfully.\n");
write(1, buff, strlen(buff));
}

return fd;
}

bool connectClient(int port){
    int sock, broadcast = 1, opt = 1;
    char buffer[1024] = {0};
    ssize_t nread;
    strcpy(buffer, "Room opened. To exit the room type \\end\n\n");
    write(1, buffer, strlen(buffer));
    signal(SIGALRM, alarm_handler);
    siginterrupt(SIGALRM, 1);


    struct sockaddr_in bc_address;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));
    setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));

    bc_address.sin_family = AF_INET;
    bc_address.sin_port = htons(port);
    bc_address.sin_addr.s_addr = inet_addr("255.255.255.255");

    bind(sock, (struct sockaddr *)&bc_address, sizeof(bc_address));

    fd_set master_set, working_set;
    int max_sd;

    FD_ZERO(&master_set);
    FD_SET(sock, &master_set);
    FD_SET(STDIN_FILENO, &master_set);
    max_sd = sock;
    if (STDIN_FILENO > max_sd)
        max_sd = STDIN_FILENO;

    if (role == 2)
        alarm(60);

    bool echo = false;

    while (1) {
        memset(buffer, '\0', 1024);
        working_set = master_set;
        select(max_sd + 1, &working_set, NULL, NULL, NULL);

        if (!connection_state){
            strcpy(buffer, "\\break\n");
            sendto(sock, buffer, strlen(buffer), 0,(struct sockaddr *)&bc_address, sizeof(bc_address));
            return connection_state;
        }

        if (FD_ISSET(STDIN_FILENO, &working_set) && role != 3){
            nread = read(0, buffer, 1024);
            buffer[nread] = '\0';
            sendto(sock, buffer, strlen(buffer), 0,(struct sockaddr *)&bc_address, sizeof(bc_address));
            if (role == 2)
                alarm(0);
            buffer[nread-1] = '\0';
            if (!strcmp(buffer, "\\end"))
                return true;
            echo = true;
        }
        else{
            if (!echo) {
                nread = read(sock, buffer, 1024);
                buffer[nread] = '\0';
                write(1, buffer, strlen(buffer));
                if (!strcmp(buffer, "\\end"))
                    return true;
                else if (!strcmp(buffer, "\\break")) {
                    connection_state = false;
                    return connection_state;
                }
            }
            echo = false;
        }
    }
}

void alarm_handler(int sig) {
    connection_state = false;
}