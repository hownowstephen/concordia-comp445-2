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

int port = ROUTER_PORT1; // Listening port
SOCKET server_socket;    // Global listening socket
fd_set readfds;          // Socket multiplex
int infds=1, outfds=0;

struct timeval timeout;             // Socket timeout struct
const struct timeval *tp=&timeout;

int main(void){
    /* Main function, performs the listening loop for client connections */

    WSADATA wsadata;            // Winsock connection object
    int result;                 // Retains result of spawning a thread
    char localhost[11];         // Store the value of localhost
    char router[11];            // Store the name of the router
    HOSTENT *hp;                // Host entity
    HOSTENT *rp;                // Router entity
    SOCKADDR_IN sa_in;          // fill with server info, IP, port
    SOCKADDR_IN sa_out;         // fill with router info
    char szbuffer[BUFFER_SIZE]; // buffer object
     
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
        if((server_socket = socket(AF_INET,SOCK_DGRAM,0))==INVALID_SOCKET)  throw "can't initialize socket";

        //Fill-in Server Port and Address info.
        sa_in.sin_family = AF_INET;
        sa_in.sin_port = htons(PEER_PORT1);
        sa_in.sin_addr.s_addr = htonl(INADDR_ANY);

        // Bind to the server port
        if (bind(server_socket,(LPSOCKADDR)&sa_in,sizeof(sa_in)) == SOCKET_ERROR) throw "can't bind local host socket";

        // Prompt for router connection
        prompt("Enter the router hostname: ", router);

        // Load remote host details
        if((rp=gethostbyname(router)) == NULL) throw "supplied router name could not be found on the network";

        //Fill-in Server Port and Address info.
        memset(&sa_out,0,sizeof(sa_out));
        memcpy(&sa_out.sin_addr,rp->h_addr,rp->h_length);
        sa_out.sin_family = rp->h_addrtype;
        sa_out.sin_port = htons(ROUTER_PORT1);

        // Connect to the router (or exit if it is not online)
        if (connect(server_socket,(LPSOCKADDR)&sa_out,sizeof(sa_out)) == SOCKET_ERROR) throw "connecting to the router failed";

        //Display the host machine internet address
        cout << "Connected to remote host: " << inet_ntoa(sa_out.sin_addr) << ":" << ROUTER_PORT1 << endl;

        // Server will block waiting for new client requests indefinitely
        while(1) {

            // Receive header data from the client
            recvbuf(server_socket,sa_out,szbuffer);

            // Extract data from the headers
            char cusername[128], filename[128], direction[3];
            sscanf(szbuffer,HEADER,cusername,direction,filename);

            // Print out the information
            cout << "Client " << cusername << " requesting to " << direction << " file " << filename << endl;

            // Respond to the client request
            if(!strcmp(direction,GET))      put(server_socket,sa_out,PUT,filename);
            else if(!strcmp(direction,PUT)) get(server_socket,sa_out,GET,filename);
            else                            throw "Requested protocol does not exist";

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




