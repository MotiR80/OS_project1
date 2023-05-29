#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/time.h>

struct Client{
    char name[10];
    int lastAnswer = -1;
    int page = 0;
    int role = -1; // 1 for student and 2 for TA
    int last_q_index;
    int socket = 0;

    void set(char str[]){
        int i;
        for (i = 0; str[i] != ':'; ++i) {
            name[i] = str[i];
        }
        if (i <= 10)
            name[i] = '\0';
    }

    void reset(){
        memset(name, '\0', sizeof(name));
        lastAnswer = -1;
        page = 0;
        role = -1;
        socket = 0;
    }
};

struct Users{
    char userPass[20];

    void set(char str[]){
        strcpy(userPass, str);
    }
};
struct Question{
    char q[256];
    bool status;
    int port;
    int cid;

    Question(){
        memset(q, '\0', 256);
        status = false;
        port = 12000;
    }
    void remove(){
        memset(q, '\0', 256);
        status = false;
        port = 12000;
    }
    void set(char str[], int i, int id){
        strcpy(q, str);
        status = true;
        port += i;
        cid = id;
    }
};

Client clients[10];

Users students[3];
Users TAs[2];

Question waiting[10];
Question answering[10];

int answerd = 0;


int setupServer(int port) {
    struct sockaddr_in address;
    int server_fd;
    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    bind(server_fd, (struct sockaddr *)&address, sizeof(address));

    listen(server_fd, 5);

    return server_fd;
}

int acceptClient(int server_fd) {
    int client_fd;
    struct sockaddr_in client_address;
    int address_len = sizeof(client_address);
    client_fd = accept(server_fd, (struct sockaddr *)&client_address, (socklen_t*) &address_len);

    return client_fd;
}
//-------------------------------------------------------

int login(char str[], int);

void page_handler(char[], int );

void write_to_file(const char* , int , const char*, const char*);

int get_question_number(const char*);
//-------------------------------------------------------
int main(int argc, char const *argv[]) {
    students[0].set("ali:1234");
    students[1].set("mamad:1234");
    students[2].set("amir:1234");
    TAs[0].set("mohsen:1234");
    TAs[1].set("morteza:1234");
    //******************************************
    answerd = get_question_number("../answered.txt");
    int server_fd, new_socket, max_sd, port;

    char buffer[1024] = {0};
    fd_set master_set, working_set;

    port = atoi(argv[0]);
    server_fd = setupServer(8888);

    FD_ZERO(&master_set);
    max_sd = server_fd;
    FD_SET(server_fd, &master_set);

    write(1, "Server is running\n", 18);

    while (1) {
        working_set = master_set;
        select(max_sd + 1, &working_set, NULL, NULL, NULL);

        if (FD_ISSET(server_fd, &working_set)) {


            if((new_socket = acceptClient(server_fd)) <= 0){
                perror("accept: ");
                exit(EXIT_FAILURE);
            }
            for (int i = 0; i < sizeof(clients)/ sizeof(Client); ++i) {
                if (clients[i].socket == 0) {
                    clients[i].socket = new_socket;
                    page_handler(buffer, i);
                    break;
                }
            }
            FD_SET(new_socket, &master_set);
            if (new_socket > max_sd)
                max_sd = new_socket;
            printf("New client connected. fd = %d\n", new_socket);
        }

        else { // client sending msg
            for (int i = 0; i < sizeof(clients)/ sizeof(Client); i++) {
                if (FD_ISSET(clients[i].socket, &working_set)) {
                    int bytes_received;
                    bytes_received = recv(clients[i].socket, buffer, 1024, 0);

                    if (bytes_received == 0) { // EOF
                        printf("client fd = %d closed\n", clients[i].socket);
                        close(clients[i].socket);
                        FD_CLR(clients[i].socket, &master_set);
                        clients[i].reset();
                        continue;
                    }
                    page_handler(buffer, i);
                    memset(buffer, 0, 1024);
                }
            }
        }
    }
}

void page_handler(char str[], int id){
    char buffer[1024];
    clients[id].lastAnswer = atoi(str);

    if (clients[id].page == 0){
        strcpy(buffer, "Are you student or teaching assistant? (Enter options number)\n1.Student\n2.Teaching assistant\n\n0.Exit\n");
        clients[id].page = 1;
    }
    else if (clients[id].page == 1){
        if (clients[id].lastAnswer == 0){}
        else{
            clients[id].role = clients[id].lastAnswer;
            strcpy(buffer, "1.Login\n\n0.Back\n");
            clients[id].page = 2;
        }
    }
    else if (clients[id].page == 2){
        if (clients[id].lastAnswer == 0){
            clients[id].page = 0;
            clients[id].role = -1;
            page_handler(str, id);
            return;
        }
        else if (clients[id].lastAnswer == 1){
            strcpy(buffer, "Enter your user and pass in (user:pass) format:\n");
            clients[id].page = 3;
        }
    }
    else if (clients[id].page == 3){
        int errno = login(str, id);
        //login failed
        if (errno == 0){
            strcpy(buffer, "Wrong user or pass. Enter your user and pass again in (user:pass) format:\n");
            clients[id].page = 3;
        }
        else if (errno == 1){
            if (clients[id].role == 1) {
                strcpy(buffer,
                       "You logged in successfully.\n\n1.Ask a question\n2.Listen to other online rooms\n\n0.Logout\n");
            }
            else if (clients[id].role == 2) {
                strcpy(buffer, "You logged in successfully.\n\n1.Get questions list\n0.Logout\n\n");
            }
            clients[id].page = 4;
        }
    }
    else if (clients[id].page == 4){
        if (clients[id].lastAnswer == 0){
            clients[id].page = 0;
            clients[id].role = -1;
            page_handler(str, id);
            return;
        }
        else if (clients[id].role == 1){
            if (clients[id].lastAnswer == 1){
                strcpy(buffer, "Type your question:\n");
                clients[id].page = 5;
            }
            else if (clients[id].lastAnswer == 2){
                strcpy(buffer, "Choose a room:\n");
                for (int i = 0; i < sizeof(answering)/ sizeof(Question); ++i) {
                    if (answering[i].status) {
                        sprintf(buffer + strlen(buffer), "%d.", i);
                        sprintf(buffer + strlen(buffer), "%d", answering[i].port);
                        strcpy(buffer + strlen(buffer), "\n");
                    }
                }
                clients[id].page = 6;
            }
        }
        else if (clients[id].role == 2){
            if (clients[id].lastAnswer == 1){
                strcpy(buffer, "Choose a question:\n");
                for (int i = 0; i < sizeof(waiting)/ sizeof(Question); ++i) {
                    if (waiting[i].status) {
                        sprintf(buffer + strlen(buffer), "%d.", i);
                        strcpy(buffer + strlen(buffer), waiting[i].q);
                        strcpy(buffer + strlen(buffer), "\n");
                    }
                }
                clients[id].page = 7;
            }
        }
    }
    else if (clients[id].page == 5){
        for (int i = 0; i < sizeof(waiting)/ sizeof(Question); ++i) {
            if (!waiting[i].status){
                waiting[i].set(str, i, id);
                clients[id].last_q_index = i;
                break;
            }
        }
        strcpy(buffer, "Wait until a teaching assistant answers your question.\n");
        clients[id].page = 9;
    }
    else if (clients[id].page == 6){

        sprintf(buffer, "%d\n", answering[clients[id].lastAnswer].port);
        clients[id].page = 8;
    }
    else if (clients[id].page == 7){

        sprintf(buffer, "%d\n", waiting[clients[id].lastAnswer].port);
        answering[clients[id].lastAnswer].set(str, clients[id].lastAnswer, id);
        send(clients[waiting[clients[id].lastAnswer].cid].socket, buffer, sizeof(buffer), 0);
        waiting[clients[id].lastAnswer].remove();
        clients[id].page = 8;
    }
    else if (clients[id].page == 8){

        if (clients[id].role == 1) {
            strcpy(buffer,
                   "Finished.\n\n1.Ask a question\n2.Listen to other online rooms\n0.Logout\n");
        }
        else if (clients[id].role == 2) {
            strcpy(buffer, "Finished.\n\n1.Get questions list\n0.Logout\n");
        }
        clients[id].page = 4;
    }
    else if (clients[id].page == 9){
        if (strcmp(str, "yes")) {
            strcpy(buffer, "Write the correct answer:\n");
            clients[id].page = 10;
        }
        else{
            clients[id].page = 8;
            answering[clients[id].last_q_index].remove();
            waiting[clients[id].last_q_index].set(str, clients[id].lastAnswer, id);
            page_handler(str, id);
            return;
        }
    }
    else if (clients[id].page == 10){
        strcpy(buffer, "Thank you for your response.\n\n1.Ask a question\n2.Listen to other online rooms\n0.Logout\n");
        write_to_file("../answered.txt", answerd, answering[clients[id].last_q_index].q, str);
        answering[clients[id].last_q_index].remove();
        clients[id].page = 4;
    }

    send(clients[id].socket, buffer, sizeof(buffer), 0);
    sprintf(buffer,"%d", clients[id].page);
    send(clients[id].socket, buffer, sizeof(buffer), 0);
}

void write_to_file(const char* filename, int question_num, const char* question, const char* answer) {
    char question_num_str[12];
    snprintf(question_num_str, sizeof(question_num_str), "%d", question_num);

    int fd = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd < 0) {
        // file doesn't exist, create it
        fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) {
            perror("open failed");
            exit(EXIT_FAILURE);
        }
    }

    // write question number to file
    ssize_t n = write(fd, question_num_str, strlen(question_num_str));
    if (n < 0) {
        perror("write failed");
        close(fd);
        exit(EXIT_FAILURE);
    }

    // add period after question number
    char period = '.';
    n = write(fd, &period, sizeof(period));
    if (n < 0) {
        perror("write failed");
        close(fd);
        exit(EXIT_FAILURE);
    }

    // write question to file
    n = write(fd, question, strlen(question));
    if (n < 0) {
        perror("write failed");
        close(fd);
        exit(EXIT_FAILURE);
    }

    // add newline between question and answer
    char newline = '\n';
    n = write(fd, &newline, sizeof(newline));
    if (n < 0) {
        perror("write failed");
        close(fd);
        exit(EXIT_FAILURE);
    }

    // write answer to file
    n = write(fd, answer, strlen(answer));
    if (n < 0) {
        perror("write failed");
        close(fd);
        exit(EXIT_FAILURE);
    }

    // add newline at end of answer
    n = write(fd, &newline, sizeof(newline));
    if (n < 0) {
        perror("write failed");
        close(fd);
        exit(EXIT_FAILURE);
    }

    // close file
    if (close(fd) < 0) {
        perror("close failed");
        exit(EXIT_FAILURE);
    }
}

int get_question_number(const char* filename) {
    char buffer[256];
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        // file doesn't exist, create it
        fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) {
            perror("open failed");
            exit(EXIT_FAILURE);
        }
        // write initial question number to file
        char initial_question_num_str[] = "0.";
        ssize_t n = write(fd, initial_question_num_str, strlen(initial_question_num_str));
        if (n < 0) {
            perror("write failed");
            close(fd);
            exit(EXIT_FAILURE);
        }
        // close file
        if (close(fd) < 0) {
            perror("close failed");
            exit(EXIT_FAILURE);
        }
        // return initial question number
        return 0;
    }

    // read up to 255 characters from file
    ssize_t n = read(fd, buffer, 255);
    if (n < 0) {
        perror("read failed");
        close(fd);
        exit(EXIT_FAILURE);
    }

    // null-terminate buffer
    buffer[n] = '\0';

    // find the period that separates the question number from the question
    char* period_pos = strchr(buffer, '.');
    if (period_pos == NULL) {
        fprintf(stderr, "invalid file format\n");
        close(fd);
        exit(EXIT_FAILURE);
    }

    // extract the question number from the buffer
    char question_num_str[12];
    strncpy(question_num_str, buffer, period_pos - buffer);
    question_num_str[period_pos - buffer] = '\0';

    // convert question number string to integer
    int question_num = atoi(question_num_str);

    // close file
    if (close(fd) < 0) {
        perror("close failed");
        exit(EXIT_FAILURE);
    }

    return question_num+1;
}

int login(char str[], int id) {
    if (clients[id].role == 1) {
        for (int i = 0; i < sizeof(students)/ sizeof(Users); ++i) {
            if (!strcmp(students[i].userPass, str)) {
                clients[id].set(str);
                return 1;
            }
        }
    } else if (clients[id].role == 2) {
        for (int i = 0; i < sizeof(TAs)/ sizeof(Users); ++i) {
            if (!strcmp(TAs[i].userPass, str)) {
                clients[id].set(str);
                return 1;
            }
        }
    }
    return 0;
}