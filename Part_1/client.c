/* To run:

gcc client.c -o client
./client localhost 4547

*/
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include <sys/uio.h>
#include <time.h>

uint8_t segment=1;
int option=1, random_number;

//Format of data packet
	struct data_packet{
		unsigned int start_of_packet_id;
		uint8_t client_id;
		unsigned int data;
		uint8_t segment_no;
		uint8_t length;
		char payload[255];
		unsigned int end_of_packet_id;
	};
      //Ack packet format
	struct ack_packet{
		unsigned int start_of_packet_id;
		uint8_t client_id;
		unsigned int ack;
		uint8_t received_segment_no;
		unsigned int end_of_packet_id;	
		};

	//Reject packet format
	struct reject_packet{
		unsigned int start_of_packet_id;
		uint8_t client_id;
		unsigned int reject;
		unsigned int reject_sub_code;
		uint8_t received_segment_no;
		unsigned int end_of_packet_id;	
		};

void error(char *msg);
//Display data datagram content
void data_packet_display(struct data_packet my_data_packet){
	printf("\n\tCLient is sending following DATAGRAM to the server ->\n");
	printf("\n\tStart of Packet id : %x",my_data_packet.start_of_packet_id);
	printf("\n\tClient ID : %x", my_data_packet.client_id);
	printf("\n\tData : %x", my_data_packet.data);
	printf("\n\tSegment Number : %d", my_data_packet.segment_no);
	printf("\n\tLength : %d", my_data_packet.length);
	printf("\n\tPayload : %s", my_data_packet.payload);
	printf("\n\tEnd of Packet ID : %x", my_data_packet.end_of_packet_id);
	printf("\n");
}
//Display ack datagram content
void ack_packet_display(struct ack_packet my_ack_packet){
	printf("\n\t____________________________________________________________________________________________\n");
	printf("\n\tRemember  ACK=0XFFF2  REJECT=0XFFF3  1. REJECT OUT OF SEQUENCE=0XFFF4");
	printf("\n\t2. REJECT LENGTH MISMATCH=0XFFF5  3. REJECT END OF PACKET MISSING=0XFFF6");
	printf("\n\t4. REJECT DUPLICATE PACKET=0XFFF7");
	printf("\n\t____________________________________________________________________________________________\n");
	printf("\n\tCLient received an ACK from server ->\n");
	printf("\n\tStart of Packet id : %x", my_ack_packet.start_of_packet_id);
	printf("\n\tClient ID : %x",my_ack_packet.client_id);
	printf("\n\tAck : %x", my_ack_packet.ack);
	printf("\n\tReceived Segment Number : %d", my_ack_packet.received_segment_no);
	printf("\n\tEnd of Packet ID : %x", my_ack_packet.end_of_packet_id);
	printf("\n");
}
//Display reject datagram content
void reject_packet_display(struct reject_packet my_reject_packet){
	printf("\n\t____________________________________________________________________________________________\n");
	printf("\n\tRemember  ACK=0XFFF2  REJECT=0XFFF3  1. REJECT OUT OF SEQUENCE=0XFFF4");
	printf("\n\t2. REJECT LENGTH MISMATCH=0XFFF5  3. REJECT END OF PACKET MISSING=0XFFF6");
	printf("\n\t4. REJECT DUPLICATE PACKET=0XFFF7");
	printf("\n\t____________________________________________________________________________________________\n");
	printf("\n\tCLient received a REJECT from server ->\n");
	printf("\n\tStart of Packet id : %x", my_reject_packet.start_of_packet_id);
	printf("\n\tClient ID : %x", my_reject_packet.client_id);
	printf("\n\tREJECT : %x", my_reject_packet.reject);
	printf("\n\tReject Sub Code : %x",my_reject_packet.reject_sub_code);
	printf("\n\tReceived Segment Number : %d", my_reject_packet.received_segment_no);
	printf("\n\tEnd of Packet ID : %x", my_reject_packet.end_of_packet_id);
	printf("\n");
}
int main(int argc, char *argv[])
{

	int sock,length,n=0;
	struct sockaddr_in server,from;	
	struct hostent *hp;
	struct data_packet my_data_packet;
	struct ack_packet  my_ack_packet;
	struct reject_packet my_reject_packet;
	char buffer[256];
	int retry_counter=0;
	//data_packet_value_initialization
	my_data_packet.start_of_packet_id=0XFFFF;
	my_data_packet.client_id=0XFF;
	my_data_packet.data=0XFFF1;
	my_data_packet.segment_no=segment;
	strcpy(my_data_packet.payload,"this is my payload!");
	int l;
	l=strlen(my_data_packet.payload);
	my_data_packet.length= l;
	my_data_packet.end_of_packet_id=0XFFFF;

	if(argc !=3)
	{
		printf("Usage: server port\n");
		exit(1);
	}
	//AF_NET refers to IP addresses(addresses in internet);SOCK_DGRAM refers to datagrams on connection-less; 0 because using UDP; sock(domain,type,protocol);socket() creates a socket in the particular domain, of type and uses protocol
	sock=socket(AF_INET,SOCK_DGRAM,0);
	//If socket building is not successfull
	if(sock<0)
	{
		error("socket");	
	}
	server.sin_family = AF_INET;
	hp = gethostbyname(argv[1]);

	if(hp==0)
	{
		error("Unknown host");
	}
	//copying the values of the server address
	bcopy((char *)hp->h_addr,(char *)&server.sin_addr,hp->h_length);
	//hton converts converts port number entered by user to network byte order(i.e. network type)
	server.sin_port = htons(atoi(argv[2]));
	length=sizeof(struct sockaddr_in);
	struct timeval tv;
	tv.tv_sec = 3;
	tv.tv_usec = 0;
	setsockopt(sock,SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(struct timeval) );
	do
	{
	n=0;
	retry_counter=0;
	//bzero(buffer,256);
	printf("\n\t--------------------------------------------------------------------------------------------");
	printf("\n\tChoose an option");
	printf("\n\t1. Send a datagram with no errors");
	printf("\n\t2. Send a datagram with out-of-sequence error");
	printf("\n\t3. Send a datagram with length mismatch error");
	printf("\n\t4. Send a datagram with missing end-of-packet-identifier error");
	printf("\n\t5. Send a datagram with duplicate-datagram error");
	printf("\n\t6. EXIT\n\t-----> ");
	scanf("%d", &option);
	switch(option)
	{
	case(1):
		//1. Send a datagram with no errors
		//data_packet_value_initialization
		my_data_packet.segment_no=segment;
		strcpy(my_data_packet.payload,"this is my payload!");
		l=strlen(my_data_packet.payload);
		my_data_packet.length= l;
		my_data_packet.end_of_packet_id=0XFFFF;
		data_packet_display(my_data_packet);
		//checks if the ack is recieved or not after sending; if not, the datagram is resent thrice
		while((n<=0) && (retry_counter<3))
		{
		sendto(sock,&my_data_packet,sizeof(my_data_packet),0,(struct sockaddr *) &server,length);	
		if(n<0) 
		{
			printf("\n\tNo response from server for three seconds. Resending the datagram.");
			retry_counter++;
		}	
		n=recvfrom(sock,&my_ack_packet,sizeof(my_ack_packet),0,NULL,NULL);
		}
		if(retry_counter==3)
		{
			printf("\n\tServer failed to respond after 3 attempts.");
			exit(0);
		}
		my_ack_packet.received_segment_no = my_data_packet.segment_no;
		//display acked datagram
		ack_packet_display(my_ack_packet);
		//only increment segment for correct datagram
		segment=segment+1;
		//reinitialize for next datagram in advance
		my_data_packet.segment_no = segment;
		break;
	case(2):
		//2. Send an out-of-sequence datagram 
		random_number= rand() % 100+1; //rand(1,100)
		my_data_packet.segment_no=segment+random_number;
		strcpy(my_data_packet.payload,"this is my payload!");
		l=strlen(my_data_packet.payload);
		my_data_packet.length= l;
		my_data_packet.end_of_packet_id=0XFFFF;
		data_packet_display(my_data_packet);
		while((n<=0) && (retry_counter<3))
		{
		sendto(sock,&my_data_packet,sizeof(my_data_packet),0,(struct sockaddr *) &server,length);		
		if(n<0)
		{
			printf("\n\tNo response from server for three seconds. Resending the datagram ");
			retry_counter++;
		}
		n=recvfrom(sock,&my_reject_packet,sizeof(my_reject_packet),0,NULL,NULL);
		}
		if(retry_counter>=3)
		{
			printf("\n\tServer failed to respond after 3 attempts.");
			exit(0);
		}
		my_reject_packet.received_segment_no = my_data_packet.segment_no;
		reject_packet_display(my_reject_packet);
		break;
	case(3):
		//3. Send a datagram with incorrect-length error
		my_data_packet.segment_no=segment;
		strcpy(my_data_packet.payload,"this is my payload!");
		l=strlen(my_data_packet.payload);
		random_number= rand() % l+1;    //rand(1,l)
		my_data_packet.length= random_number;
		my_data_packet.end_of_packet_id=0XFFFF;
		data_packet_display(my_data_packet);
		while((n<=0) && (retry_counter<3))
		{
		sendto(sock,&my_data_packet,sizeof(my_data_packet),0,(struct sockaddr *) &server,length);		
		if(n<0)
		{
			printf("\n\tNo response from server for three seconds. Resending the datagram ");
			retry_counter++;
		}
		n=recvfrom(sock,&my_reject_packet,sizeof(my_reject_packet),0,NULL,NULL);
		}
		if(retry_counter>=3)
		{
			printf("\n\tServer failed to respond after 3 attempts.");
			exit(0);
		}
		my_reject_packet.received_segment_no = my_data_packet.segment_no;
		reject_packet_display(my_reject_packet);
		my_data_packet.segment_no = segment;
		break;
	case(4):
		//4. Send a datagram with missing end-of-packet identifier
		my_data_packet.segment_no=segment;
		strcpy(my_data_packet.payload,"this is my payload!");
		l=strlen(my_data_packet.payload);
		my_data_packet.length= l;
		my_data_packet.end_of_packet_id=0X0000;
		data_packet_display(my_data_packet);
		while((n<=0) && (retry_counter<3))
		{
		sendto(sock,&my_data_packet,sizeof(my_data_packet),0,(struct sockaddr *) &server,length);		
		if(n<0)
		{
			printf("\n\tNo response from server for three seconds. Resending the datagram ");
			retry_counter++;
		}
		n=recvfrom(sock,&my_reject_packet,sizeof(my_reject_packet),0,NULL,NULL);
		}
		if(retry_counter>=3)
		{
			printf("\n\tServer failed to respond after 3 attempts.");
			exit(0);
		}
		my_reject_packet.received_segment_no = my_data_packet.segment_no;
		reject_packet_display(my_reject_packet);
		my_data_packet.segment_no = segment;
		break;
	case(5):
		//5. Send a duplicate datagram 
		my_data_packet.segment_no=segment-1;
		strcpy(my_data_packet.payload,"this is my payload!");
		l=strlen(my_data_packet.payload);
		my_data_packet.length=l;
		my_data_packet.end_of_packet_id=0XFFFF;
		data_packet_display(my_data_packet);
		while((n<=0) && (retry_counter<3))
		{
		sendto(sock,&my_data_packet,sizeof(my_data_packet),0,(struct sockaddr *) &server,length);		
		if(n<0)
		{
			printf("\n\tNo response from server for three seconds. Resending the datagram.");
			retry_counter++;
		}
		n=recvfrom(sock,&my_reject_packet,sizeof(my_reject_packet),0,NULL,NULL);
		}
		if(retry_counter>=3)
		{
			printf("\n\tServer does not respond\n");
			exit(0);
		}
		my_reject_packet.received_segment_no = my_data_packet.segment_no;
		reject_packet_display(my_reject_packet);
		break;
	case(6):
		exit(0);
	default:
		break;
	}
	}while(option<=5);
	close(sock);
}
