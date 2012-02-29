// FTP Server over Datagram (UDP) via router
// @author Stephen Young
// @email st_youn@encs.concordia.ca
// @student_id 9736247

#include <winsock.h>
#include <process.h>
#include <iostream>
#include <fstream>
#include <time.h>
#include <windows.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

using namespace std;

#include "protocol.cpp"

int port = REQUEST_PORT; // Listening port
SOCKET s;               // Global listening socket
fd_set readfds;         // Socket multiplex
int infds=1, outfds=0;

void handle_client(){
    /* Client handler function, spawned by the main loop in response to a client connection */

    SOCKET client_socket;       // Client socket this function is bound to
    char szbuffer[BUFFER_SIZE]; // Buffer for client/server communication
    int ibytesrecv;             // Retains a count of received bytes from the client

    union { struct sockaddr generic; struct sockaddr_in ca_in; } ca; // Client socket addressing
    int calen=sizeof(ca);                                            // Length of client socket addressing

    try {

        //Found a connection request, try to accept. 
        if((client_socket=accept(s,&ca.generic,&calen))==INVALID_SOCKET)   throw "Couldn't accept connection\n";

        //Connection request accepted.
        cout<<"accepted connection from "<<inet_ntoa(ca.ca_in.sin_addr)<<":"<<hex<<htons(ca.ca_in.sin_port)<<endl;

        memset(szbuffer,0,BUFFER_SIZE); // zero the buffer

        //Fill in szbuffer from accepted request.
        if((ibytesrecv = recv(client_socket,szbuffer,BUFFER_SIZE,0)) == SOCKET_ERROR)
            throw "Error in client headers";

        cout << "Received headers from client" << szbuffer << endl;

        // Retrieve data about the user
        char client_name[11];
        char direction[3];
        char filename[100];
        sscanf(szbuffer,"%s %s %s",client_name,direction,filename);

        // Print out the information
        cout << "Client " << client_name << " requesting to " << direction << " file " << filename << endl;

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
        if((s = socket(AF_INET,SOCK_STREAM,0))==INVALID_SOCKET)  throw "can't initialize socket";

        //Fill-in Server Port and Address info.
        sa.sin_family = AF_INET;
        sa.sin_port = htons(port);
        sa.sin_addr.s_addr = htonl(INADDR_ANY);

        //Bind the server port
        if (bind(s,(LPSOCKADDR)&sa,sizeof(sa)) == SOCKET_ERROR) throw "can't bind the socket";

        //Successfull bind, now listen for client requests.
        if(listen(s,10) == SOCKET_ERROR)    throw "couldn't  set up listen on socket";

        FD_ZERO(&readfds);

        while(1){   
            FD_SET(s,&readfds);  //always check the listener
            if((outfds=select(infds,&readfds,NULL,NULL,tp)) == SOCKET_ERROR) throw "failure in Select";
            else if (FD_ISSET(s,&readfds)){
                // Received a new connection request, spawn a subthread with handle_client to respond
                int args = 0;
                if(( result = _beginthread((void (*)(void *))handle_client, STKSIZE, (void *) args))>=0)
                    cout << "Preparing to establish a new connection" << endl; 
            }
        }
    // Catch and print any errors
    } catch(const char * str){
        cerr << str << endl;
    }

    //close server socket and clean up the winsock
    closesocket(s);
    WSACleanup();
    return 0;
}




