// FTP Client over Datagram (UDP) via router
// @author Stephen Young
// @email st_youn@encs.concordia.ca
// @student_id 9736247

#include <windows.h>
#include <winsock.h>
#include <string.h>
#include <fstream>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>

using namespace std;

#include "protocol.cpp"


int port=REQUEST_PORT;

//buffer data types

int ibufferlen=0;
int ibytessent;
int ibytesrecv=0;

//host data types
HOSTENT *hp;
HOSTENT *rp;

// Filename to be transferred

void prompt(char* message, char*buffer){
    cout << message << flush ;  // Print the message
    cin >> buffer;              // Record the input into the buffer
}

int main(void){

    //socket data types
    SOCKET client_socket;   // Client socket
    SOCKADDR_IN sa;         // filled by bind
    SOCKADDR_IN sa_in;      // fill with server info, IP, port

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

        if((hp=gethostbyname(localhost)) == NULL)   throw "Localhoost gethostbyname failed\n";

        //Ask for name of remote server
        prompt("please enter your remote server name: ",remotehost);

        if((rp=gethostbyname(remotehost)) == NULL)  throw "Remote gethostbyname failed\n";

        //Create the socket
        if((client_socket = socket(AF_INET,SOCK_STREAM,0))==INVALID_SOCKET)  throw "Socket failed\n";

        //Specify server address for client to connect to server.
        memset(&sa_in,0,sizeof(sa_in));
        memcpy(&sa_in.sin_addr,rp->h_addr,rp->h_length);
        sa_in.sin_family = rp->h_addrtype;   
        sa_in.sin_port = htons(port);

        //Display the host machine internet address
        cout << "Connecting to remote host:" << inet_ntoa(sa_in.sin_addr) << endl;

        //Connect Client to the server
        if (connect(s,(LPSOCKADDR)&sa_in,sizeof(sa_in)) == SOCKET_ERROR)    throw "connect failed\n";

        prompt("Please enter a filename: ",&filename);              // Retrieve a filename from the client
        prompt("Direction of transfer [get|put]: ",&direction);     // Retrieve a transfer direction

        // Make sure the direction is one of get or put
        if(!strcmp(direction,GET) || !strcmp(direction,PUT)){ 

            // Retrieve the local user name
            GetUserName(cusername,&dwusername);

            // Send client headers
            sprintf(szbuffer,"%s || %s || %s", cusername, direction, filename); 
            sendbuf(client_socket,szbuffer)

            // Perform a get request
            if(!strcmp(direction,GET)) get(client_socket,cusername,filename);
            // Perform a put request
            else if(!strcmp(direction,PUT)) put(client_socket,cusername,filename);

        }else{
            throw "this protocol only supports get or put";
        }

    } // try loop

    //Display any needed error response.
    catch (const char *str) { 
        cerr<<str<<endl;
    }

    //close the client socket
    closesocket(s);

    /* When done uninstall winsock.dll (WSACleanup()) and exit */ 
    WSACleanup();  
    return 0;
}





