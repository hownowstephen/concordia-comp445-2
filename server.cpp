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

FILE* tracefile = fopen("server.log","w");

int main(void){
    /* Main function, performs the listening loop for client connections */
    set_trace(tracefile,"Server");
    srand ( time(NULL) );

    SOCKET server_socket;       // Global listening socket
    SOCKADDR_IN sa_out;         // fill with router info
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
        while(1){

            char szbuffer[BUFFER_SIZE]; // buffer object
            int server_num = 0;         // client packet tracer
            int client_num = 0;         // server packet tracer

            int selected = rand() % 256;
            int received, verify;

            int progress = 0;

            while(1){
                client_num = 3;
                // Receive a random number from the client

                if(recvbuf(server_socket,sa_out,&client_num,szbuffer, BUFFER_SIZE, true) < 0){
                if(progress < 1) continue;
                }else progress = 1;
                cout << "Received " << szbuffer << endl;
                sscanf(szbuffer,"RAND %d",&received);

                server_num = 1;
                // Send acknowledgement to the client along with our random number
                sprintf(szbuffer,"RAND %d %d",received,selected);
                cout << "Sending " << szbuffer << endl;
                if(sendbuf(server_socket, sa_out, &server_num, szbuffer, BUFFER_SIZE, true) < 0){
                if(progress < 2) continue;
                }else    progress = 2;

                client_num = 2;
                // Finally wait for a response from the client with the number
                if(recvbuf(server_socket, sa_out, &client_num, szbuffer, BUFFER_SIZE, true) < 0){
                if(progress < 3) continue;
                }else    progress = 3;
                cout << "Received " << szbuffer << endl;
                sscanf(szbuffer,"RAND %d",&verify);

                if(progress == 3) break;
            }

            client_num = received & 0x1;
            server_num = selected & 0x1;

            cout << "Starting with server packet " << server_num << " and client packet " << client_num << endl;

            // Receive header data from the client
            recvbuf(server_socket,sa_out,&client_num,szbuffer);

            // Extract data from the headers
            char cusername[128], filename[128], direction[3];
            sscanf(szbuffer,HEADER,cusername,direction,filename);

            // Print out the information
            cout << "Client " << cusername << " requesting to " << direction << " file " << filename << endl;

            // Respond to the client request
            if(!strcmp(direction,GET)){
                set_trace(tracefile,SEND);
                put(server_socket, sa_out, PUT, filename, client_num);
            }else if(!strcmp(direction,PUT)){
                set_trace(tracefile,RECV);
                get(server_socket, sa_out, GET, filename, client_num);
            }else   throw "Requested protocol does not exist";
        }

    // Catch and print any errors
    } catch(const char * str){
        cerr << str << WSAGetLastError() << endl;
    }

    fclose(tracefile);
    //close server socket and clean up the winsock
    closesocket(server_socket);
    WSACleanup();
    return 0;
}




