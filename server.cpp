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

int main(void){
    /* Main function, performs the listening loop for client connections */

    int server_num = 0;         // Server packet identifier
    int client_num = 0;         // Client packet identifier
    SOCKET server_socket;       // Global listening socket
    SOCKADDR_IN sa_out;         // fill with router info
    char szbuffer[BUFFER_SIZE]; // buffer object
    WSADATA wsadata;            // Winsock connection object
    char router[11];            // Store the name of the router
     
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
        
        server_socket = open_port(PEER_PORT1);
        // Prompt for router connection
        prompt("Enter the router hostname: ", router);
        sa_out = prepare_peer_connection(router, ROUTER_PORT1);

        // Server will block waiting for new client requests indefinitely
        while(1) {

            int selected = rand() % 256;
            int received, received_verify;

            // Receive a random number from the client
            recvbuf(server_socket,sa_out,&client_num,szbuffer);
            sscanf(szbuffer,"RAND %d",received);

            // Send acknowledgement to the client along with our random number
            sprintf(szbuffer,"RAND %d %d",received,selected);
            sendbuf(server_socket, sa_out, &client_num, szbuffer);

            // Finally wait for a response from the client with the number
            recvbuf(server_socket,sa_out,&client_num,szbuffer);
            sscanf(szbuffer,"RAND %d",received_verify);

            if(selected != recieved_verify){
                cout << "Something went wrong in the initial handshake..." << endl;
                continue;
            }

            client_num = recieved & 0x1;
            server_num = selected % 0x1;

            // Receive header data from the client
            recvbuf(server_socket,sa_out,&client_num,szbuffer);

            client_num = received & 0x1;
            server_num = selected & 0x1;

            // Extract data from the headers
            char cusername[128], filename[128], direction[3];
            sscanf(szbuffer,HEADER,cusername,direction,filename);

            // Print out the information
            cout << "Client " << cusername << " requesting to " << direction << " file " << filename << endl;

            // Respond to the client request
            if(!strcmp(direction,GET))      put(server_socket, sa_out, PUT, filename, server_num, client_num);
            else if(!strcmp(direction,PUT)) get(server_socket, sa_out, GET, filename, client_num, server_num);
            else                            throw "Requested protocol does not exist";

        }

    // Catch and print any errors
    } catch(const char * str){
        cerr << str << WSAGetLastError() << endl;
    }

    //close server socket and clean up the winsock
    closesocket(server_socket);
    WSACleanup();
    return 0;
}




