#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <sys/time.h> // timeval for timeout

#define True 1
#define False 0

char users[100][1024];  //max user online = 100.
int usersSd[100];       //the socketsd
int userNum = 0;        //currently user number
pthread_mutex_t mutex;

//======= the declaration of the main thread function ===============
void *connection_handler(void *); 
//====================================================================             
int IsRegistered(const char * acc){              //to tell whether the id was signed or not.
    char in[1024];
    FILE * fPtr;
    fPtr = fopen("register.txt", "r");
    if(fPtr == NULL){
        perror("register.txt doesn't exit: ");
        return -1;
    }
    else{
        memset(in,0,1024);
        while(!feof(fPtr)){                     //check line by line
            fscanf(fPtr, "%s", in);
            strtok(in, "#");
            if(strncmp(acc,in,9) ==0 ){
                fclose(fPtr);
                return 1;
            }
        }
    }
    fclose(fPtr);
    return -1;
}
//====================================================================             
int Register(char *buffer, char *mybal){

    FILE* fPtr;
    char *par , acc[10];

    bzero(&par,sizeof(par));
    //memset(par,0,10);
    par = strtok(buffer, "#");
    par = strtok(NULL, "#");
    strncat(acc,par,9);

    if(IsRegistered(par) == 1)
        return -1;
    fPtr = fopen("register.txt", "a");
    if(fPtr == NULL)
    {
        perror("register.txt doesn't exit: ");
        return -1;
    }
    else
    {
        fprintf(fPtr, "%s", acc);
        fprintf(fPtr, "%s", "#");
        fprintf(fPtr, "%s\n", mybal);
        fclose(fPtr);
        printf("Registration Accepted: %s\n", acc);
        return 1;
    }
    fclose(fPtr);
    return 1;
}
//====================================================================             
int GetAccBal(char * acc){
    char in[1024];
    FILE * fPtr;
    fPtr = fopen("register.txt", "r");
    if(fPtr == NULL){
        perror("register.txt doesn't exit: ");
        return -1;
    }
    else{
        memset(in,0,1024);
        while(fscanf(fPtr, "%s", in) == 1){
            strtok(in, "#");
            if(strncmp(acc,in,9) ==0 ){
                char *amount = strtok(NULL,"\n");
                fclose(fPtr);
                return atoi(amount);
            }
        }
        if (feof(fPtr)){
            fclose(fPtr);
            return -2;
        }
    }
    fclose(fPtr);
    return -1;
}
//====================================================================             
char* List(int sock, char* acc){
    char num[4];
    char *r;
    char bal[10];
    //printf("%s\n", acc);  //debug
    r = malloc(sizeof(char)*1024);
    memset(r,0,1024);
    strcat(r,"Account balance: ");
    sprintf(bal, "%d", GetAccBal(acc));
    strcat(r, bal);
    strcat(r, "\nOnline user number: ");
    sprintf(num,"%d",userNum);
    strcat(r, num);
    strcat(r,"\n");
    for(int i = 0; i < userNum; i++)
    {
        //printf("--%s--\n", users[i]); //Debug
        strcat(r,users[i]);
        strcat(r,"\n");
    }
    return r;
}
//====================================================================             
char* GetRemote(int sd){
	struct sockaddr_in local_sockaddr;
    char *r ;
    r = malloc(sizeof(char)*15);
    r = inet_ntoa(local_sockaddr.sin_addr);
    return r;
}
//====================================================================             

int Login(char *buffer, int sd){
    if(strncmp(buffer,"#",1) == 0) //case "#..."
        return -1;
    if(userNum > 100)
        return -1; // exceed max user

    char account[10], ip[50], port[10], msg[1024], user_acc[10];
    memset(msg,0,1024);
    strncpy(account, strtok(buffer, "#"),10);
    strcpy(port, strtok (NULL, "#"));
    strcpy(ip, GetRemote(sd));
    if(atoi(port)<1024 || atoi(port) > 65535 || strlen(account) > 9) //invalid userport
        return -1;
    if(IsRegistered(account) < 0)                                    //not registered
        return -1;
    for(int i=0; i < userNum; i++){                                  // log in again 
        strncpy(user_acc,users[i],10);
        strtok(user_acc,"#");
        if(strcmp(account, user_acc) == 0){
            return -1;
        }

    }
    strcat(msg,account);
    strcat(msg,"#");
    strcat(msg,ip);
    strcat(msg,"#");
    strcat(msg,port);
    strcpy(users[userNum], msg);
    usersSd[userNum]=sd;
    userNum ++;
    printf("%s logged in!\n", account);
    return userNum-1;
}
//====================================================================             
void UserExit(char * acc){
    printf("%s:\n", acc);
    if (userNum > 1){
        for(int i=0; i<userNum; i++){
            if(i == userNum-1){                             // the acc is not log in right now
                break;
            }
            char p[1024];
            strcpy(p,users[i]);                         
            strtok(p,"#");
            if(strcmp(acc,p)==0){                           // find the record of acc
                strcpy(users[i], users[userNum-1]);
                usersSd[i]=usersSd[userNum-1];
                break;
            }
        }
    }
    userNum --;
}

//============================ main function ========================================             

int main(int argc , char *argv[])
{
    int local_socket , client_sock , c;
    struct sockaddr_in server , client;
    pthread_mutex_init(&mutex, NULL);   //init thread mutex

    local_socket = socket(AF_INET , SOCK_STREAM , 0);
    if (local_socket == -1){
        perror("Socket create failure: ");
        abort();
    }
    //puts("Socket created");

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons( 8889 );


    if( bind(local_socket,(struct sockaddr *)&server , sizeof(server)) < 0){
        perror("bind failed. Error");
        abort();
    }
    //puts("Bind done");


    listen(local_socket , 20);

    puts("Waiting for incoming connections...");
    c = sizeof(struct sockaddr_in);
    pthread_t thread_id;
    while( (client_sock = accept(local_socket, (struct sockaddr *)&client, (socklen_t*)&c)) ){
        //puts("Connection accepted");
        printf("Connection from: %s:%d\n",inet_ntoa(client.sin_addr), ntohs(client.sin_port));

        if( pthread_create( &thread_id , NULL ,  connection_handler , (void*) &client_sock ) < 0){
            perror("Could not create thread");
            return 1;
        }
    }

    if (client_sock < 0){
        perror("Accept failed");
        return 1;
    }

    //close(local_socket);
    return 0;
}
//================================= the main thread function ===================================================
void *connection_handler(void *local_socket){
    int sock = *(int*)local_socket;
    int read_size;
    char buffer[1024], msg[1024], acc[10];
    int login = False;
    int id = -1;

    // ====== set timeout for connection=========
    struct timeval tv;
    int ret;
    tv.tv_sec = 120;                                                                //user timeout 120 seconds
    tv.tv_usec = 0;
    if(setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv))<0){
        puts("Socket time out not supported");
    }
    // ====== timeout set over ===================

    strcpy(msg, "Hello! Please log in or sign up for your account:\n");
    write(sock , msg , strlen(msg));                                                //link success

    while((read_size = recv(sock , buffer , 1024 , 0)) > 0 ){

        //printf("%s\n", buffer); //debug
        memset(msg, 0 ,1024);
        if(strncmp(buffer, "Exit", 4) == 0 ){
            pthread_mutex_lock(&mutex);                                             //atomic exit beacause of exit modifying global counter
            if(login == True){
                UserExit(acc);
                printf("    Client disconnected.\n");
                login = 0;
            }
            pthread_mutex_unlock(&mutex);
            strcpy (msg, "Bye\n");
            write(sock, msg, strlen(msg));
            memset(buffer, 0 ,1024);
            break;

        }
        else if(strncmp(buffer, "REGISTER#", 9) == 0 && strlen(strstr(buffer,"#"))>1){//case : register
            
            //========== beginning balance ===============
                char ack[] = {"ack"};
                write(sock, ack, strlen(ack));
                char mybal[10] = {};
                recv(sock , mybal , 10, 0);
                //printf("%s\n",mybal);
            //========== end of balance setting ===========
            
            if(Register(buffer, mybal) > 0){
                strcpy(msg,"100 OK\n");
            }
            else{
                strcpy(msg,"210 FAIL\n");
            }
            write(sock, msg, strlen(msg));
        }

        else if((strncmp(buffer, "List", 4) == 0) && login == True){           // case : list
            strcpy(msg, List(sock, acc));
            write(sock,msg, strlen(msg));
        }
        
        else if(login == False && strstr(buffer, "#")!=NULL){                  // case : log in
            //printf("%s",buffer);   //debug
            pthread_mutex_lock(&mutex);                                        // sychronize operation for modifying user array
            if(strlen(strstr(buffer,"#"))>1 && (id= Login(buffer, sock)) >= 0) // file open and see whether user exits
            {
                login = True;
                strncpy(acc, users[id], 10);
                strtok(acc,"#");
                strcpy(msg,List(sock ,acc));
                //printf("%s\n", acc); debug
            }
            else{
                strcpy(msg,"220 AUTH_FAIL\n");
            }
            pthread_mutex_unlock(&mutex);
            write(sock, msg, strlen(msg));
        }

        else{
            strcpy(msg,"Unknow instruction or did not login\n");
            write(sock, msg, strlen(msg));
        }

        memset(buffer, 0, 1024);
    }
    if (read_size < 0){
        if(errno == EWOULDBLOCK || errno== EAGAIN ) {                           //errorno.h
            pthread_mutex_lock(&mutex);                                         //atomic exit beacause of exit modifying global counter
            if(login == True){
                UserExit(acc);
                login = 0;
            }
            else{
                puts("Unknown user:");
            }
            pthread_mutex_unlock(&mutex);
            strcpy (msg, "[Error]Timeout: idling for more than 60 seconds.\n");
            write(sock, msg, strlen(msg));
            memset(buffer, 0 ,1024);
            printf("    Client timeout.\n");
        }
	    else
		    printf("Recv error:%d\n",read_size);
    }
    else if(login == True){
        pthread_mutex_lock(&mutex);                                             //atomic exit beacause of exit modifying global counter
        UserExit(acc);
        login = 0;
        puts("      Interrupted.");
        pthread_mutex_unlock(&mutex);
    }
    else if(id == -1)
        puts("Unknown user:\n       Interrupted or disconnected.");
    close(sock);
    return 0;
}
