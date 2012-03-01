// FTP Server over Datagram (UDP) via router
// @author Stephen Young
// @email st_youn@encs.concordia.ca
// @student_id 9736247

#include <winsock.h>
#include <process.h>
#include <iostream>
#include <time.h>
#include <windows.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

using namespace std;

#include "protocol.cpp"

int port = REQUEST_PORT; // Listening port
SOCKET server_socket;    // Global listening socket
fd_set readfds;          // Socket multiplex
int infds=1, outfds=0;

struct timeval timeout;             // Socket timeout struct
const struct timeval *tp=&timeout;

void handle_client(){
    /* Client handler function, spawned by the main loop in response to a client connection */

    SOCKET client_socket;       // Client socket this function is bound to
    char szbuffer[BUFFER_SIZE]; // Buffer for client/server communication
    int ibytesrecv;             // Retains a count of received bytes from the client

    union { struct sockaddr generic; struct sockaddr_in ca_in; } ca; // Client socket addressing
    int calen=sizeof(ca);                                            // Length of client socket addressing

    try {

        //Found a connection request, try to accept. 
        if((client_socket=accept(server_socket,&ca.generic,&calen))==INVALID_SOCKET)   throw "Couldn't accept connection\n";

        //Connection request accepted.
        cout<<"accepted connection from "<<inet_ntoa(ca.ca_in.sin_addr)<<":"<<htons(ca.ca_in.sin_port)<<endl;

        // Receive header data from the client
        recvbuf(client_socket,szbuffer);

        // Extract data from the headers
        char cusername[128], filename[128], direction[3];
        sscanf(szbuffer,HEADER,cusername,direction,filename);

        // Print out the information
        cout << "Client " << cusername << " requesting to " << direction << " file " << filename << endl;

        // Respond to the client request
        if(!strcmp(direction,GET))      put(client_socket,PUT,filename);
        else if(!strcmp(direction,PUT)) get(client_socket,GET,filename);
        else                            throw "Requested protocol does not exist";
    
    // Catch any errors
    } catch(const char* str){
        cerr << str << endl;
    }

    //close Client socket
    closesocket(client_socket);    
}

int main(void){
    /* Main function, performs the listening loop for client connections */

    WSADATA wsadata;            // Winsock connection object
    int result;                 // Retains result of spawning a thread
    char localhost[11];         // Store the value of localhost
    HOSTENT *hp;                // Host entity
    SOCKADDR_IN sa;             // filled by bind
    SOCKADDR_IN sa1;            // fill with server info, IP, port
     
    try {
        if (WSAStartup(0x0202,&wsadata)!=0){  
            throw "Error in starting WSAStartup";
        }else{
            /* display the wsadata structure */
            cout<< endl
                << "wsadata.wVersion "       << wsadata.wVersion       << endl
                << "wsadata.wHighVersion "   << wsadata.wHighVersion   << endl
                << "wsadata.szDescription "  << wsadata.szDescription  << endl
                << "wsadata.szSystemStatus " << wsadata.szSystemStatus << endl
                << "wsadata.iMaxSockets "    << wsadata.iMaxSockets    << endl
                << "wsadata.iMaxUdpDg "      << wsadata.iMaxUdpDg      << endl;
        }  

        //Display info of local host
        gethostname(localhost,10);
        cout<<"hostname: "<<localhost<< endl;

        // Ensure the local machine has an addressable name
        if((hp=gethostbyname(localhost)) == NULL)   throw "gethostbyname() cannot get local host info"; 

        //Create the UDP server socket
        if((server_socket = socket(AF_INET,SOCK_STREAM,0))==INVALID_SOCKET)  throw "can't initialize socket";

        //Fill-in Server Port and Address info.
        sa.sin_family = AF_INET;
        sa.sin_port = htons(port);
        sa.sin_addr.s_addr = htonl(INADDR_ANY);

        //Bind the server port
        if (bind(server_socket,(LPSOCKADDR)&sa,sizeof(sa)) == SOCKET_ERROR) throw "can't bind the socket";

        //Successfull bind, now listen for client requests.
        if(listen(server_socket,10) == SOCKET_ERROR)    throw "couldn't  set up listen on socket";

        FD_ZERO(&readfds);

        while(1){   
            Sleep(1);   // Sleep to allow for interrupts
            FD_SET(server_socket,&readfds);  //always check the listener
            if((outfds=select(infds,&readfds,NULL,NULL,tp)) == SOCKET_ERROR) throw "failure in Select";
            else if (FD_ISSET(server_socket,&readfds)){
                // Received a new connection request, spawn a subthread with handle_client to respond
                int args = 0;6
                result = _beginthread((void (*)(void *))handle_client, STKSIZE, (void *) args);
            }
        }
    // Catch and print any errors
    } catch(const char * str){
        cerr << str << endl;
    }

    //close server socket and clean up the winsock
    closesocket(server_socket);
    WSACleanup();
    return 0;
}




