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

#define REQUEST_PORT 0x7070 // Port to listen on
#define BUFFER_SIZE 2048    // Size (in bytes) of the buffer
#define GET "get"           // Method name for GET requests
#define PUT "put"           // Method name for PUT requests
#define OK "OK"             // Expected response for successful transfers
#define MISSING "NO"        // Expected response for failed transfers
#define STKSIZE  16536      // Size of available stack to provide to threads
#define HEADER "%s\t%s\t%s" // Format string for headers

int sendbuf(SOCKET sock, char* buffer,int buffer_size=BUFFER_SIZE){
    int ibytessent = 0;
    if ((ibytessent = send(sock,buffer,strlen(buffer),0)) == SOCKET_ERROR){ 
        throw "Send failed"; 
    }else{
        memset(buffer,0,buffer_size);
        return ibytessent; 
    }   
}

int recvbuf(SOCKET sock, char* buffer, int buffer_size=BUFFER_SIZE){
    int ibytesrecv = 0;
    memset(buffer,0,buffer_size); // Clear the buffer to prepare to receive data
    if((ibytesrecv = recv(sock,buffer,buffer_size,0)) == SOCKET_ERROR){
        throw "Recv failed";
    }else{
        return ibytesrecv;  // Return the amount of data received
    }
}

void get(SOCKET s, char * username, char * filename){

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
        //wait for reception of server response.
        recvbuf(s,szbuffer); // Get the response from the server
        sscanf(szbuffer,"%s %d",response,&filesize);    // Extract file data

        cout << "Response " << response << " filesize " << filesize << endl;

        // Ensure the response from the socket is OK
        if(!strcmp(response,OK)){

            // Open our local file for writing
            recv_file = fopen(filename,"w");

            // Send ack to start data transfer
            memset(szbuffer,0,BUFFER_SIZE);
            sprintf(szbuffer,"SEND");
            sendbuf(s,szbuffer); // Send an ACK

            int size = 0, count = 0;
            // Read data from the server until we have received the file
            while(count < filesize){
                recvbuf(s,szbuffer);
                if(filesize - count >= BUFFER_SIZE) size = sizeof(szbuffer); // Read a full buffer
                else                                size = filesize - count;  // Read a shorter buffer
                fwrite(szbuffer,sizeof(char),size,recv_file);
                count += sizeof(szbuffer);
                cout << "Received " << count << " of " << filesize << " bytes" << endl;
            }

            // Close our output file
            fclose(recv_file);

            // Clear the buffer and send an ack to the server to confirm receipt
            memset(szbuffer,0,BUFFER_SIZE);
            sprintf(szbuffer,"OK");
            sendbuf(s,szbuffer);    // Send confirmation of receipt
        }else{
            // Make a note that the file does not exist
            cout << "Requested file does not exist" << endl;
        }
    } catch(const char* str){
        cerr<<str<<WSAGetLastError()<<endl;
    }
 }


void put(SOCKET s, char * username, const char* filename){

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

        if((send_file = fopen(filename,"r")) != NULL){
            // Determine the size of the file
            fseek(send_file, 0L, SEEK_END);
            filesize = ftell(send_file);
            fseek(send_file, 0L, SEEK_SET);

            cout << "File size: " << filesize << endl;

            // Filesize headers
            sprintf(szbuffer,"%s %d",OK,filesize);

            sendbuf(s,szbuffer);    // Send the filesize
            recvbuf(s,szbuffer);    // Wait for ack from client

            int size = 0, sent = 0;
            // Loop through the file and stream in chunks based on the buffer size
            while( sent < filesize){
                if(filesize - sent > BUFFER_SIZE)   size = BUFFER_SIZE;
                else                                size = filesize - sent;
                sent += fread( szbuffer, sizeof(char), size, send_file );
                sendbuf(s,szbuffer);
                cout << "Sent " << sent << " bytes" << endl;
            }

            cout << "Finished sending file" << endl;

            fclose(send_file);

            if((ibytesrecv = recv(s,szbuffer,BUFFER_SIZE,0)) == SOCKET_ERROR)
                throw "Receive error in server program\n";

            if(!strcmp(szbuffer,OK)){
                cout << "File transfer completed" << endl;
            }

        }else{

            cout << "File does not exist, sending decline" << endl;
            // Send back a NO to the client to indicate that the file does not exist
            sprintf(szbuffer,"NO -1");
            sendbuf(s,szbuffer);
        }
    // Print out any errors
    } catch(const char* str){
        cerr<<str<<WSAGetLastError()<<endl;
    }
 }