// FTP Client over Datagram (UDP) via router
// @author Stephen Young
// @email st_youn@encs.concordia.ca
// @student_id 9736247

#include <windows.h>
#include <winsock.h>
#include <string.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>

using namespace std;

#include "protocol.cpp"


int port=ROUTER_PORT2;

//buffer data types

int ibufferlen=0;
int ibytessent;
int ibytesrecv=0;

//host data types
HOSTENT *hp;
HOSTENT *rp;

int main(void){

    //socket data types
    SOCKET client_socket;   // Client socket
    SOCKADDR_IN sa_in;      // fill with server info, IP, port
    SOCKADDR_IN sa_out;      // fill with server info, IP, port

    char szbuffer[BUFFER_SIZE]; // Buffer

    WSADATA wsadata;                                    // WSA connection
    char localhost[11], remotehost[11];                 // Host data
    char cusername[128], filename[128], direction[3];   // Other header data
    DWORD dwusername = sizeof(cusername);               // Retains the size of the username

    try {

        if (WSAStartup(0x0202,&wsadata)!=0){  
            throw "Error in starting WSAStartup";
        } else {

            /* Display the wsadata structure */
            cout<< endl
                << "wsadata.wVersion "       << wsadata.wVersion       << endl
                << "wsadata.wHighVersion "   << wsadata.wHighVersion   << endl
                << "wsadata.szDescription "  << wsadata.szDescription  << endl
                << "wsadata.szSystemStatus " << wsadata.szSystemStatus << endl
                << "wsadata.iMaxSockets "    << wsadata.iMaxSockets    << endl
                << "wsadata.iMaxUdpDg "      << wsadata.iMaxUdpDg      << endl;
        }  

        gethostname(localhost,10);
        cout << "Local host name is \"" << localhost << "\"" << endl;

        if((hp=gethostbyname(localhost)) == NULL)   throw "Localhost gethostbyname failed\n";

        //Ask for name of remote server
        prompt("please enter your remote server name: ",remotehost);

        if((rp=gethostbyname(remotehost)) == NULL)  throw "Remote gethostbyname failed\n";

        //Fill-in UDP Port and Address info.
        sa_in.sin_family = AF_INET;
        sa_in.sin_port = htons(PEER_PORT2);
        sa_in.sin_addr.s_addr = htonl(INADDR_ANY);

        // Create the socket
        if((client_socket = socket(AF_INET,SOCK_DGRAM,0))==INVALID_SOCKET)  throw "Socket failed\n";

        // Bind to the client port
        if (bind(client_socket,(LPSOCKADDR)&sa_in,sizeof(sa_in)) == SOCKET_ERROR)
            throw "can't bind the socket1";

        //Specify server address for client to connect to server.
        memset(&sa_out,0,sizeof(sa_out));
        memcpy(&sa_out.sin_addr,rp->h_addr,rp->h_length);
        sa_out.sin_family = rp->h_addrtype;   
        sa_out.sin_port = htons(ROUTER_PORT2);

        //Connect Client to the server
        if (connect(client_socket,(LPSOCKADDR)&sa_out,sizeof(sa_out)) == SOCKET_ERROR)    throw "connect failed\n";

        //Display the host machine internet address
        cout << "Connected to remote host: " << inet_ntoa(sa_out.sin_addr) << ":" << ROUTER_PORT2 << endl;

        prompt("Please enter a filename: ",filename);              // Retrieve a filename from the client
        prompt("Direction of transfer [get|put]: ",direction);     // Retrieve a transfer direction

        // Make sure the direction is one of get or put
        if(!strcmp(direction,GET) || !strcmp(direction,PUT)){ 

            // Retrieve the local user name
            GetUserName(cusername,&dwusername);

            // Send client headers
            sprintf(szbuffer,HEADER, cusername, direction, filename); 
            sendbuf(client_socket,sa_out,szbuffer);

            // Perform a get request
            if(!strcmp(direction,GET)) get(client_socket,sa_out,cusername,filename);
            // Perform a put request
            else if(!strcmp(direction,PUT)) put(client_socket,sa_out,cusername,filename);

        }else{
            throw "this protocol only supports get or put";
        }

    } // try loop

    //Display any needed error response.
    catch (const char *str) { 
        cerr << str << endl;
    }

    //close the client socket and clean up
    closesocket(client_socket);
    WSACleanup();  
    return 0;
}





