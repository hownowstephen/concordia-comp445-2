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
#include <time.h>

using namespace std;

#define ROUTER_PORT1 7000   // router port number 1 (server)
#define ROUTER_PORT2 7001   // router port number 2 (client)
#define PEER_PORT1  5000    // peer port number 1 (server)
#define PEER_PORT2  5001    // peer port number 2 (client)
#define BUFFER_SIZE 2048    // Size (in bytes) of the buffer
#define GET "get"           // Method name for GET requests
#define PUT "put"           // Method name for PUT requests
#define OK "OK"             // Expected response for successful transfers
#define RAND "RAND"         // Indicates a random number being sent
#define MISSING "NO"        // Expected response for failed transfers
#define SEND "Sender"       // Sender prefix
#define RECV "Receiver"     // Receiver prefix
#define HEADER "%s\t%s\t%s" // Format string for headers
#define TIMEOUT_USEC 300000 //time-out value
#define TRACE 1

FILE* LOGFILE;
char* TRACE_PREFIX;

void trace(char* message){
    if(TRACE){
        fprintf(LOGFILE,"%s: %s",TRACE_PREFIX,message);
        memset(message,0,sizeof(message));
    }
}


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

SOCKADDR_IN prepare_peer_connection(char* hostname, int port){
    SOCKADDR_IN sa;
    HOSTENT *hp;
    if((hp=gethostbyname(hostname)) == NULL) throw "Could not determine a host address from supplied name";

    cout << "Peer connection: " << hostname << ":" << port << endl;

    // Fill in port and address information
    memcpy(&sa.sin_addr,hp->h_addr,hp->h_length);
    sa.sin_family = hp->h_addrtype;   
    sa.sin_port = htons(port);
    return sa;
}

int recvbuf(SOCKET sock, SOCKADDR_IN sa, int* packet_num, char* buffer, int buffer_size=BUFFER_SIZE, bool allow_timeout=false){
    try{
        int ibytesrecv = 0;               // Number of bytes received
        int ibytessent = 0;               // Number of bytes sent
        int result;                       // Result of select call
        fd_set readfds;                   // Used by select to manage file descriptor multiplexing
        struct timeval *tp=new timeval;   // Timeout struct
        tp->tv_sec=0;                     // Set current time
        tp->tv_usec=TIMEOUT_USEC;         // Set timeout time
        char control_buffer[BUFFER_SIZE]; // Control flow buffer, used to store the ACK result
        int from = sizeof(sa);            // Size of the sockaddr
        bool mismatch = false;            // Checks if there is a packet mismatch
        char packetc;                     // Holds the packet number in character format
        int packeti;                      // Holds the packet number in int format
        char trace_buffer[BUFFER_SIZE];   // Holds trace information

        FD_ZERO(&readfds);
        FD_SET(sock,&readfds);

        if((result=select(1,&readfds,NULL,NULL,tp))==SOCKET_ERROR) throw "Timer error!";
        else if(result > 0){
            memset(buffer,0,buffer_size); // Clear the buffer to prepare to receive data
            if((ibytesrecv = recvfrom(sock,buffer,buffer_size,0,(SOCKADDR*)&sa, &from)) == SOCKET_ERROR){
                throw "Recv failed";
            }else{
                // Trace the packet if necessary
                sprintf(trace_buffer,"Received packet %d (%d bytes)",packet_num,ibytesrecv);
                trace(trace_buffer);

                memset(control_buffer,0,sizeof(control_buffer));
                packetc = buffer[BUFFER_SIZE-1];
                packeti = atoi(&packetc);

                if(packeti != *packet_num){
                    cout << "Packet mismatch, received packet " << packeti << ", discarding" << endl;
                    mismatch = true;
                }
                sprintf(control_buffer,"%d",packeti);
                cout << "Sending acknowledgment message " << control_buffer << endl;
                if ((ibytessent = sendto(sock,control_buffer,sizeof(control_buffer),0,(SOCKADDR*)&sa, from)) == SOCKET_ERROR){ 
                    throw "Send failed"; 
                }else{
                    if(!mismatch){
                        // Trace the packet if necessary
                        sprintf(trace_buffer,"Sent ack for packet %d",packet_num);
                        trace(trace_buffer);
                        if(*packet_num == 1)        *packet_num = 0;
                        else if(*packet_num == 2)   *packet_num = 3;    // Used in initial handshake
                        else if(*packet_num == 3)   *packet_num = 2;    // Used in initial handshake
                        else                        *packet_num = 1;
                        return ibytesrecv;  // Return the amount of data received
                    }else{
                        throw "Mismatch";
                    }
                }
            }
        }else{
            throw "Result not possible";
        }
    }catch(const char* str){
        if(allow_timeout){
            return -1;
        }
        cout << str << " attempting recvbuf again... ERROR:" << WSAGetLastError() << endl;
        return recvbuf(sock,sa,packet_num,buffer,buffer_size, allow_timeout);
    }
}

int sendbuf(SOCKET sock, SOCKADDR_IN sa, int* packet_num, char* buffer,int buffer_size=BUFFER_SIZE, bool allow_timeout=false){
    try{
        int ibytesrecv = 0;               // Number of bytes received
        int ibytessent = 0;               // Number of bytes sent
        int result;                       // Result of select call
        fd_set readfds;                   // Used by select to manage file descriptor multiplexing
        struct timeval *tp=new timeval;   // Timeout struct
        tp->tv_sec=0;                     // Set current time
        tp->tv_usec=TIMEOUT_USEC;         // Set timeout time
        char control_buffer[BUFFER_SIZE]; // Control flow buffer, used to store the ACK result
        int from = sizeof(sa);            // Size of the sockaddr
        int verify;                       // Verify the received packet id
        char trace_buffer[BUFFER_SIZE];   // Trace info buffer

        if(*packet_num == 1)        buffer[BUFFER_SIZE-1] = '1';
        else if(*packet_num == 2)   buffer[BUFFER_SIZE-1] = '2';    // Used in initial handshake
        else if(*packet_num == 3)   buffer[BUFFER_SIZE-1] = '3';    // Used in initial handshake
        else                        buffer[BUFFER_SIZE-1] = '0';

        if ((ibytessent = sendto(sock,buffer,BUFFER_SIZE,0,(SOCKADDR*)&sa, from)) == SOCKET_ERROR) throw "Send failed";
        else{
            FD_ZERO(&readfds);
            FD_SET(sock,&readfds);
            
            if((result=select(1,&readfds,NULL,NULL,tp))==SOCKET_ERROR){
                throw "Timer error!";
            }else if(result > 0){

                // Trace the packet if necessary
                sprintf(trace_buffer,"Sent packet %d (%d bytes)",packet_num,ibytessent);
                trace(trace_buffer);

                memset(control_buffer,0,sizeof(control_buffer));
                if((ibytesrecv = recvfrom(sock,control_buffer,sizeof(control_buffer),0,(SOCKADDR*)&sa, &from)) == SOCKET_ERROR){
                    throw "Ack recv failed";
                }else{
                    sscanf(control_buffer,"%d",&verify);
                    if(*packet_num == verify){
                        // Trace the packet if necessary
                        sprintf(trace_buffer,"Received ack for packet %d",packet_num);
                        trace(trace_buffer);

                        if(*packet_num == 1)        *packet_num = 0;
                        else if(*packet_num == 2)   *packet_num = 3;    // Used in initial handshake
                        else if(*packet_num == 3)   *packet_num = 2;    // Used in initial handshake
                        else                        *packet_num = 1;
                        memset(buffer,0,buffer_size);
                        return ibytessent;
                    }else if(verify > 1 || verify < 0){
                        throw "Invalid verification data received";
                    }else{
                        cout << "Ignoring packet, got " << control_buffer << " parse to " << verify << " " << endl;
                    }
                }
            }else{
                // Otherwise re-initiate the process
                throw "No ack received";
            }
        }
    }catch(const char* str){
        if(allow_timeout){
            return -1;
        }
        cout << str << " attempting sendbuf again... ERROR:" << WSAGetLastError() << endl;
        return sendbuf(sock,sa,packet_num,buffer,buffer_size,allow_timeout);
    }
}

void prompt(const char* message, char*buffer){
    cout << message << flush ;  // Print the message
    cin >> buffer;              // Record the input into the buffer
}

void get(SOCKET s, SOCKADDR_IN sa, char * username, char * filename, int packet_num){

    char szbuffer[BUFFER_SIZE];
    char tracebuf[BUFFER_SIZE]; // for managing trace

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
        recvbuf(s,sa,&packet_num,szbuffer); // Get the response from the server
        sscanf(szbuffer,"%s %d",response,&filesize);    // Extract file data

        cout << "Response " << response << " filesize " << filesize << endl;

        // Ensure the response from the socket is OK
        if(!strcmp(response,OK)){

            // Open our local file for writing
            recv_file = fopen(filename,"wb");

            int size = 0, count = 0;
            // Read data from the server until we have received the file
            while(count < filesize){
                if(filesize - count >= BUFFER_SIZE) size = (sizeof(szbuffer) / sizeof(char)) - sizeof(char); // Read a full buffer
                else                                size = ((filesize - count) / sizeof(char)) - sizeof(char);  // Read a shorter buffer
                recvbuf(s,sa,&packet_num,szbuffer);
                fwrite(szbuffer,sizeof(char),size,recv_file);
                count += sizeof(szbuffer);
                cout << "Received " << count << " of " << filesize << " bytes" << endl;
            }

            cout << "Finished receiving data" << endl;

            // Do cleanup

            while(true){
                if(recvbuf(s,sa,&packet_num,szbuffer,BUFFER_SIZE,true) < 0){
                    memset(szbuffer,0,sizeof(szbuffer));
                    sprintf(szbuffer,"%s",OK);
                    sendbuf(s,sa,&packet_num,szbuffer,BUFFER_SIZE,true);
                }else{
                    break;
                }

            }

            // Close our output file
            fclose(recv_file);

            cout << "Completed transfer of " << filename << endl;
        }else{
            // Make a note that the file does not exist
            cout << "Requested file does not exist" << endl;
        }
    } catch(const char* str){
        cerr<<str<<WSAGetLastError()<<endl;
    }
 }


void put(SOCKET s, SOCKADDR_IN sa, char * username, char* filename, int packet_num){

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
            sendbuf(s,sa,&packet_num,szbuffer);    // Send the filesize

            int size = 0, sent = 0;
            // Loop through the file and stream in chunks based on the buffer size
            while ( !feof(send_file) ){
                fread(szbuffer,1,BUFFER_SIZE-1,send_file);
                cout << "Sending " << sizeof(szbuffer) << " bytes (packet " << packet_num << ")" << endl;
                sendbuf(s,sa,&packet_num,szbuffer);
            }

            fclose(send_file);

            // Do cleanup

            while(true){
                memset(szbuffer,0,sizeof(szbuffer));
                sprintf(szbuffer,"%s",OK);
                if(sendbuf(s,sa,&packet_num,szbuffer,BUFFER_SIZE,true) < 0){
                    recvbuf(s,sa,&packet_num,szbuffer,BUFFER_SIZE,true);
                }else{
                    break;
                }
            }

            if(!strcmp(szbuffer,OK))    cout << "File transfer completed" << endl;

        }else{

            cout << "File does not exist, sending decline" << endl;
            // Send back a NO to the client to indicate that the file does not exist
            sprintf(szbuffer,"NO -1");
            sendbuf(s,sa,&packet_num,szbuffer);
        }
    // Print out any errors
    } catch(const char* str){
        cerr<<str<<WSAGetLastError()<<endl;
    }
 }