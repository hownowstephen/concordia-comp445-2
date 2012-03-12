// File Transfer Library, encapsulates get and put requests
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

#define ROUTER_PORT1 7000   // router port number 1 (server)
#define ROUTER_PORT2 7001   // router port number 2 (client)
#define PEER_PORT1  5000    // peer port number 1 (server)
#define PEER_PORT2  5001    // peer port number 2 (client)
#define BUFFER_SIZE 2048    // Size (in bytes) of the buffer
#define GET "get"           // Method name for GET requests
#define PUT "put"           // Method name for PUT requests
#define OK "OK"             // Expected response for successful transfers
#define MISSING "NO"        // Expected response for failed transfers
#define HEADER "%s\t%s\t%s" // Format string for headers
#define TIMEOUT_USEC 300000 //time-out value


SOCKET open_port(int port){
    SOCKET sock;      // Define the socket to return
    SOCKADDR_IN sa;     // Define the socket address information
    HOSTENT *hp;        // Host entity details
    char hostname[11]; // Store the value of localhost

    // Retrieve the local hostname
    gethostname(hostname,11);

    if((hp=gethostbyname(hostname)) == NULL)   throw "Could not determine a host address from supplied name";
    //Fill-in UDP Port and Address info.
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    sa.sin_addr.s_addr = htonl(INADDR_ANY);
     // Create the socket
    if((sock = socket(AF_INET,SOCK_DGRAM,0))==INVALID_SOCKET) throw "Generating a new local socket failed";
    // Bind to the client port
    if (bind(sock,(LPSOCKADDR)&sa,sizeof(sa)) == SOCKET_ERROR) throw "Could not bind socket to supplied port";

    return sock;
}

SOCKADDR_IN* prepare_peer_connection(char* hostname, int port){
    SOCKADDR_IN* sa;
    HOSTENT *hp;
    if((hp=gethostbyname(hostname)) == NULL) throw "Could not determine a host address from supplied name";

    // Fill in port and address information
    memcpy(&sa.sin_addr,hp->h_addr,hp->h_length);
    sa.sin_family = hp->h_addrtype;   
    sa.sin_port = htons(port);
    return &sa;
}

int recvbuf(SOCKET sock, SOCKADDR_IN* sa, int* packet_num, char* buffer, int buffer_size=BUFFER_SIZE){
    try{
        int ibytesrecv = 0;               // Number of bytes received
        int ibytessent = 0;               // Number of bytes sent
        int result;                       // Result of select call
        fd_set readfds;                   // Used by select to manage file descriptor multiplexing
        struct timeval *tp=new timeval;   // Timeout struct
        tp->tv_sec=0;                     // Set current time
        tp->tv_usec=TIMEOUT_USEC;         // Set timeout time
        char control_buffer[BUFFER_SIZE]; // Control flow buffer, used to store the ACK result
        int from = sizeof(*sa);            // Size of the sockaddr
        bool mismatch = false;            // Checks if there is a packet mismatch
        char packetc;                     // Holds the packet number in character format
        int packeti;                      // Holds the packet number in int format

        FD_ZERO(&readfds);
        FD_SET(sock,&readfds);

        if((result=select(1,&readfds,NULL,NULL,tp))==SOCKET_ERROR){
            throw "Timer error!";
        }else if(result > 0){
            cout << "Receiving packet " << *packet_num << endl;
            memset(buffer,0,buffer_size); // Clear the buffer to prepare to receive data
            if((ibytesrecv = recvfrom(sock,buffer,buffer_size,0,(SOCKADDR*)sa, &from)) == SOCKET_ERROR){
                throw "Recv failed";
            }else{
                memset(control_buffer,0,sizeof(control_buffer));

                packetc = buffer[BUFFER_SIZE-1];
                packeti = atoi(&packetc);

                if(packeti == *packet_num){
                    sprintf(control_buffer,"%d %s",packeti,OK);
                }else{
                    cout << "Packet mismatch, received packet " << packeti << ", discarding" << endl;
                    sprintf(control_buffer,"%d %s",(int)!*packet_num,OK);
                    mismatch = true;
                }
                cout << "Sending acknowledgment message " << control_buffer << endl;
                if ((ibytessent = sendto(sock,control_buffer,sizeof(control_buffer),0,(SOCKADDR*)sa, sizeof(sa))) == SOCKET_ERROR){ 
                    throw "Send failed"; 
                }else{
                    cout << "Sent ack successfully" << endl;
                    if(!mismatch){
                        if(*packet_num)  *packet_num = 0;
                        else             *packet_num = 1;
                        return ibytesrecv;  // Return the amount of data received
                    }else{
                        return recvbuf(sock,sa,packet_num,buffer,buffer_size);
                    }
                }
            }
        }else{
            return recvbuf(sock, sa, packet_num, buffer, buffer_size);
        }
    }catch(const char* str){
        cout << str << " attempting recvbuf again... (" << WSAGetLastError() << ")" << endl;
        return recvbuf(sock,sa,packet_num,buffer,buffer_size);
    }
}

int sendbuf(SOCKET sock, SOCKADDR_IN* sa, int* packet_num, char* buffer,int buffer_size=BUFFER_SIZE){
    try{
        int ibytesrecv = 0;               // Number of bytes received
        int ibytessent = 0;               // Number of bytes sent
        int result;                       // Result of select call
        fd_set readfds;                   // Used by select to manage file descriptor multiplexing
        struct timeval *tp=new timeval;   // Timeout struct
        tp->tv_sec=0;                     // Set current time
        tp->tv_usec=TIMEOUT_USEC;         // Set timeout time
        char control_buffer[BUFFER_SIZE]; // Control flow buffer, used to store the ACK result
        int from = sizeof(*sa);            // Size of the sockaddr

        cout << "Sending packet " << *packet_num << endl;

        if ((ibytessent = sendto(sock,buffer,BUFFER_SIZE,0,(SOCKADDR*)sa, sizeof(sa))) == SOCKET_ERROR){ 
            throw "Send failed"; 
        }else{

            FD_ZERO(&readfds);
            FD_SET(sock,&readfds);
            cout << "Waiting on ack from peer for packet " << *packet_num << endl;
            // TODO: if anything fails, re-run sendbuf
            if((result=select(1,&readfds,NULL,NULL,tp))==SOCKET_ERROR){
                throw "Timer error!";
            }else if(result > 0){
                memset(control_buffer,0,sizeof(control_buffer));
                if((ibytesrecv = recvfrom(sock,control_buffer,sizeof(control_buffer),0,(SOCKADDR*)sa, &from)) == SOCKET_ERROR){
                    throw "Ack recv failed";
                }else{
                    // TODO: Verify the sequence number of this request
                    cout << "Finished negotiating a packet, acknowledgment " << control_buffer << " received" << endl;
                    memset(buffer,0,buffer_size);
                    if(*packet_num)  *packet_num = 0;
                    else             *packet_num = 1;
                    return ibytessent; 
                }
            }else{
                // Otherwise re-initiate the process
                return sendbuf(sock, sa, packet_num, buffer, buffer_size);
            }
        }
    }catch(const char* str){
        cout << str << " attempting sendbuf again... (" << WSAGetLastError() << ")" << endl;
        return sendbuf(sock,sa,packet_num,buffer,buffer_size);
    }
}

void prompt(const char* message, char*buffer){
    cout << message << flush ;  // Print the message
    cin >> buffer;              // Record the input into the buffer
}

void get(SOCKET s, SOCKADDR_IN* sa, char * username, char * filename, int local, int peer){

    int* local_packet = &local;
    int* peer_packet = &peer;

    char szbuffer[BUFFER_SIZE];

    //host data types
    HOSTENT *hp;
    HOSTENT *rp;

    // Perform a get request
    // Parse response and filesize from server
    char response[2];
    int filesize;
    FILE *recv_file;

    try {
        cout << "Waiting for file headers" << endl;
        //wait for reception of server response.
        recvbuf(s,sa,peer_packet,szbuffer); // Get the response from the server
        sscanf(szbuffer,"%s %d",response,&filesize);    // Extract file data

        cout << "Response " << response << " filesize " << filesize << endl;

        // Ensure the response from the socket is OK
        if(!strcmp(response,OK)){

            // Open our local file for writing
            recv_file = fopen(filename,"wb");

            // Send ack to start data transfer
            memset(szbuffer,0,BUFFER_SIZE);
            sprintf(szbuffer,"SEND");
            memset(szbuffer+BUFFER_SIZE-1,*local_packet,1);    // Append the packet identifier
            sendbuf(s,sa,local_packet,szbuffer); // Send an ACK

            int size = 0, count = 0;
            // Read data from the server until we have received the file
            while(count < filesize){
                if(filesize - count >= BUFFER_SIZE) size = (sizeof(szbuffer) / sizeof(char)) - sizeof(char); // Read a full buffer
                else                                size = ((filesize - count) / sizeof(char)) - sizeof(char);  // Read a shorter buffer
                recvbuf(s,sa,peer_packet,szbuffer);
                fwrite(szbuffer,sizeof(char),size,recv_file);
                count += sizeof(szbuffer);
                cout << "Received " << count << " of " << filesize << " bytes" << endl;
            }

            cout << "Finished receiving data" << endl;

            // Close our output file
            fclose(recv_file);

            // Clear the buffer and send an ack to the server to confirm receipt
            memset(szbuffer,0,BUFFER_SIZE);
            sprintf(szbuffer,"OK");
            memset(szbuffer+BUFFER_SIZE-1,*local_packet,1);    // Append the packet identifier
            sendbuf(s,sa,local_packet,szbuffer);    // Send confirmation of receipt

            cout << "Completed transfer of " << filename << endl;
        }else{
            // Make a note that the file does not exist
            cout << "Requested file does not exist" << endl;
        }
    } catch(const char* str){
        cerr<<str<<WSAGetLastError()<<endl;
    }
 }


void put(SOCKET s, SOCKADDR_IN* sa, char * username, char* filename, int local, int peer){

    int* local_packet = &local;
    int* peer_packet = &peer;

    char szbuffer[BUFFER_SIZE];

    char *buffer;
    int ibufferlen=0;
    int ibytessent;
    int ibytesrecv=0;
    //host data types
    HOSTENT *hp;
    HOSTENT *rp;

    cout << "Sending file " << filename << endl;

    FILE *send_file;
    int filesize;

    try {

        if((send_file = fopen(filename,"rb")) != NULL){
            // Determine the size of the file
            fseek(send_file, 0L, SEEK_END);
            filesize = ftell(send_file);
            fseek(send_file, 0L, SEEK_SET);

            cout << "File size: " << filesize << endl;

            // Filesize headers
            sprintf(szbuffer,"%s %d",OK,filesize);
            memset(szbuffer+BUFFER_SIZE-1,*local_packet,1);    // Append the packet identifier
            sendbuf(s,sa,local_packet,szbuffer);    // Send the filesize
            recvbuf(s,sa,peer_packet,szbuffer);    // Wait for ack from client

            int size = 0, sent = 0;
            // Loop through the file and stream in chunks based on the buffer size
            while ( !feof(send_file) ){
                fread(szbuffer,1,BUFFER_SIZE-1,send_file);
                memset(szbuffer+BUFFER_SIZE-1,*local_packet,1);    // Append the packet identifier
                cout << "Sending " << sizeof(szbuffer) << " bytes" << endl;
                sendbuf(s,sa,local_packet,szbuffer);
            }

            fclose(send_file);
            recvbuf(s,sa,peer_packet,szbuffer); // Receive the ack from the client

            if(!strcmp(szbuffer,OK))    cout << "File transfer completed" << endl;

        }else{

            cout << "File does not exist, sending decline" << endl;
            // Send back a NO to the client to indicate that the file does not exist
            sprintf(szbuffer,"NO -1");
            memset(szbuffer+BUFFER_SIZE-1,*local_packet,1);    // Append the packet identifier
            sendbuf(s,sa,local_packet,szbuffer);
        }
    // Print out any errors
    } catch(const char* str){
        cerr<<str<<WSAGetLastError()<<endl;
    }
 }