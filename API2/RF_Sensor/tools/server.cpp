#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <time.h>

#define BUFFER_LOG_SIZE 1000*11*4
#define PORT 1515

char fileName[300];
char fileName2[300];
char command[400];
char buffer[BUFFER_LOG_SIZE] = { 0 };
int filesCount = 0;

void getFileName(char * fn){
    time_t rawtime;
    struct tm * timeinfo;

    time ( &rawtime );
    timeinfo = localtime ( &rawtime );

    sprintf(fn,"../../data/BomFuturo_%d%d%d%d%d%d_%d.bin", timeinfo->tm_year,
                                    timeinfo->tm_mon,
                                    timeinfo->tm_mday,
                                    timeinfo->tm_hour,
                                    timeinfo->tm_min,
                                    timeinfo->tm_sec,
                                    filesCount );
}

void getFileNameTXT(char * fn){
    sprintf(fn,"%s.txt",fileName);
}

int main(int argc, char const* argv[])
{
    int server_fd, new_socket, valread;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
   

    while (true)
    {    
    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
 
    // Forcefully attaching socket to the port 8080
    if (setsockopt(server_fd, SOL_SOCKET,
                   SO_REUSEADDR | SO_REUSEPORT, &opt,
                   sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
 
    // Forcefully attaching socket to the port 8080
    if (bind(server_fd, (struct sockaddr*)&address,
             sizeof(address))
        < 0) {
        printf("bind failed");
        continue;
    }
    if (listen(server_fd, 3) < 0) {
        printf("listen");
        continue;
    }
    if ((new_socket
         = accept(server_fd, (struct sockaddr*)&address,
                  (socklen_t*)&addrlen))
        < 0) {
        printf("acept");
        continue;
    }


    
    FILE * file;
    FILE * file2;
    
    getFileName(fileName);
    getFileNameTXT(fileName2);

    file = fopen(fileName,"w+");

    int total = 0;
    int count = 0;

    printf("\n");
    while((valread = read(new_socket, buffer, BUFFER_LOG_SIZE))>0){
        fwrite(buffer,1,valread,file);
        printf("%d ",valread);
        total+=valread;
    }
    printf(" = %d ",total);
    fclose(file);

    sprintf(command,"./bin2txt %s",fileName);
    system(command);
    filesCount++;
    

    // send(new_socket, hello, strlen(hello), 0);
    // printf("Hello message sent\n");
 
    // closing the connected socket
    close(new_socket);
    // closing the listening socket
    shutdown(server_fd, SHUT_RDWR);
    }
    return 0;
}