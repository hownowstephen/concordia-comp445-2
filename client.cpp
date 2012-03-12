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

int main(void){

    int packet_num = 0; // Client negotiates with packet zero from the outset

    //socket data types
    SOCKET client_socket;   // Client socket
    SOCKADDR_IN sa_out;      // fill with server info, IP, port

    char szbuffer[BUFFER_SIZE]; // Buffer

    WSADATA wsadata;                                    // WSA connection
    char router[11];                                    // Host data
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

        client_socket = open_port(PEER_PORT2);

        prompt("Enter the router hostname: ",router);
        sa_out = prepare_peer_connection(router, ROUTER_PORT2);

        prompt("Enter a filename: ",filename);                  // Retrieve a filename from the client
        prompt("Direction of transfer [get|put]: ",direction);  // Retrieve a transfer direction

        // Make sure the direction is one of get or put
        if(!strcmp(direction,GET) || !strcmp(direction,PUT)){ 

            // Retrieve the local user name
            GetUserName(cusername,&dwusername);

            // Send client headers
            sprintf(szbuffer,HEADER, cusername, direction, filename); 
            sendbuf(client_socket,sa_out,&packet_num,szbuffer);

            packet_num = 0;

            // Perform a get request
            if(!strcmp(direction,GET)) get(client_socket, &sa_out, cusername, filename, packet_num);
            // Perform a put request
            else if(!strcmp(direction,PUT)) put(client_socket, &sa_out, cusername, filename, packet_num);

        }else{
            throw "this protocol only supports get or put";
        }

    } // try loop

    //Display any needed error response.
    catch (const char *str) { 
        cerr << str << WSAGetLastError() << endl;
    }

    //close the client socket and clean up
    closesocket(client_socket);
    WSACleanup();  
    return 0;
}





