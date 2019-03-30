/*Creates a datagram server. POrt number is passed as an argument. This server runs forever 

To run:
gcc server.c -o server
./server 4547
 For ack timerfunctionality execution, close server terminal, and try to send 1 datagram
*/

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include <sys/uio.h>
#include <time.h>
 
int flag=0;
uint8_t segment=0;

void error(char *msg)
{
	perror(msg);
	exit(0);
}
        //Data packet format
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

//Display DATA datagram content
void data_packet_display(struct data_packet my_data_packet){
	printf("\n\t--------------------------------------------------------------------------------------------");
	printf("\n\tServer received following DATAGRAM from the client ->\n");
	printf("\n\tStart of Packet id : %x",my_data_packet.start_of_packet_id);
	printf("\n\tClient ID : %x", my_data_packet.client_id);
	printf("\n\tData : %x", my_data_packet.data);
	printf("\n\tSegment Number : %d", my_data_packet.segment_no);
	printf("\n\tLength : %d",my_data_packet.length);
	printf("\n\tPayload : %s", my_data_packet.payload);
	printf("\n\tEnd of Packet ID : %x",my_data_packet.end_of_packet_id);
	printf("\n");
}

//Display ACK datagram content
void ack_packet_display(struct ack_packet my_ack_packet){
	printf("\n\tServer is sending the following ACK to the client ->\n");
	printf("\n\tStart of Packet id : %x",my_ack_packet.start_of_packet_id);
	printf("\n\tClient ID : %x", my_ack_packet.client_id);
	printf("\n\tAck : %x", my_ack_packet.ack);
	printf("\n\tReceived Segment Number : %d", my_ack_packet.received_segment_no);
	printf("\n\tEnd of Packet ID : %x", my_ack_packet.end_of_packet_id);
	printf("\n");
	flag=1;
}
//Display REJECT datagram content
void reject_packet_display(struct reject_packet my_reject_packet){
	printf("\n\t____________________________________________________________________________________________\n");
	printf("\n\tRemember  ACK=0XFFF2  REJECT=0XFFF3  1. REJECT OUT OF SEQUENCE=0XFFF4");
	printf("\n\t2. REJECT LENGTH MISMATCH=0XFFF5  3. REJECT END OF PACKET MISSING=0XFFF6");
	printf("\n\t4. REJECT DUPLICATE PACKET=0XFFF7");
	printf("\n\t____________________________________________________________________________________________\n");
	printf("\n\tServer is sending the following REJECT to the client ->\n");
	printf("\n\tStart of Packet id : %x", my_reject_packet.start_of_packet_id);
	printf("\n\tClient ID : %x", my_reject_packet.client_id);
	printf("\n\tREJECT : %x", my_reject_packet.reject);
	printf("\n\tReject Sub Code : %x", my_reject_packet.reject_sub_code);
	printf("\n\tReceived Segment Number : %d", my_reject_packet.received_segment_no);
	printf("\n\tEnd of Packet ID : %x", my_reject_packet.end_of_packet_id);
	printf("\n");
	flag=1;
}

int main(int argc,char *argv[])
{     

	int sock,length,fromlen,n;
	struct sockaddr_in server;
	struct sockaddr_in from;
	struct data_packet my_data_packet;
	struct ack_packet  my_ack_packet;
	struct reject_packet my_reject_packet;

	//initialize my_ack_packet values
	my_ack_packet.start_of_packet_id=0XFFFF;
	my_ack_packet.client_id=0XFF;
	my_ack_packet.ack=0XFFF2;
	my_ack_packet.received_segment_no=my_data_packet.segment_no;
	my_ack_packet.end_of_packet_id=0XFFFF;	

	//Initializing my_reject_packet values
	my_reject_packet.start_of_packet_id=0XFFFF;
	my_reject_packet.client_id=0XFF;
	my_reject_packet.reject=0XFFF3;
	my_reject_packet.reject_sub_code=0X00;
	my_reject_packet.received_segment_no=my_ack_packet.received_segment_no;
	my_reject_packet.end_of_packet_id=0XFFFF;

	char buf[1024];

	if(argc<2)
	{
	fprintf(stderr,"ERROR, no port provided\n");
	exit(0);
	}

	sock=socket(AF_INET,SOCK_DGRAM,0);
	if(sock<0)
	{
		error("Opening socket failed");
	}
	length = sizeof(server);
	bzero(&server,length);
	server.sin_family=AF_INET;
	server.sin_addr.s_addr=INADDR_ANY;
	server.sin_port=htons(atoi(argv[1]));
	if(bind(sock,(struct sockaddr *)&server,length)<0)
	{
		error("building");
	}
	fromlen = sizeof(struct sockaddr_in);
	//while server is alive
	while(1)
	{
		unsigned int expected_end_of_packet_id=0XFFFF;
		int m,n,k;
		//To receive message from socket
		n=recvfrom(sock,&my_data_packet,sizeof(my_data_packet),0,(struct sockaddr *)&from,&fromlen);
		if(n<0)
		{
			error("recvfrom");
		}
		k=my_data_packet.segment_no;

		m = strlen(my_data_packet.payload);
		n=my_data_packet.segment_no;
		// Show the received data datagram
		data_packet_display(my_data_packet);
		
		//Initializing common my_reject_packet values
		my_reject_packet.start_of_packet_id=0XFFFF;
		my_reject_packet.client_id=0XFF;
		my_reject_packet.reject=0XFFF3;

		//Case 2. Check Out-Of-Sequence datagram----------------------
		if(k!=segment+1)
		{
			//Initializing uncommon my_reject_packet values			
			my_reject_packet.reject_sub_code=0XFFF4;
			my_reject_packet.received_segment_no=my_ack_packet.received_segment_no;
			my_reject_packet.end_of_packet_id=0XFFFF;
			//Send REJECT datagram to client
			n=sendto(sock,&my_reject_packet,sizeof(my_reject_packet),0,(struct sockaddr *)&from,fromlen);
			if(n<0)
			{
			error("\n\t Error : Failed to send");
			}
			//Show sent reject datagram after updating value
			my_reject_packet.received_segment_no=my_data_packet.segment_no;
			reject_packet_display(my_reject_packet);
		}
		//Case 3. Check length of payload-----------------------		
		else if(m!=my_data_packet.length)
		{
			segment=segment+1;
			//Initializing uncommon my_reject_packet values
			my_reject_packet.reject_sub_code=0XFFF5;
			my_reject_packet.received_segment_no=my_ack_packet.received_segment_no;
			my_reject_packet.end_of_packet_id=0XFFFF;
			//Send REJECT datagram to client
			n=sendto(sock,&my_reject_packet,sizeof(my_reject_packet),0,(struct sockaddr *)&from,fromlen);
			if(n<0)
			{
			error("\n\t Error : Failed to send");
			}
			//Show sent reject datagram after updating value
			my_reject_packet.received_segment_no=my_data_packet.segment_no;
			reject_packet_display(my_reject_packet);
		}
		//Case 4. check end of packet identifier--------------------
		else if(expected_end_of_packet_id!=my_data_packet.end_of_packet_id)
		{
			segment=segment+1;
			//Initializing uncommon my_reject_packet values
			my_reject_packet.reject_sub_code=0XFFF6;
			my_reject_packet.received_segment_no=my_ack_packet.received_segment_no;
			my_reject_packet.end_of_packet_id=0X0000;
			//Send REJECT datagram to client
			n=sendto(sock,&my_reject_packet,sizeof(my_reject_packet),0,(struct sockaddr *)&from,fromlen);
			//Show sent reject datagram
			my_reject_packet.received_segment_no=my_data_packet.segment_no;
			reject_packet_display(my_reject_packet);
		}
		//Case 5. Check duplicate datagram----------------------
		else if((n>=1)&&(n<=segment))
		{
			//Initializing uncommon my_reject_packet values
			my_reject_packet.reject_sub_code=0XFFF7;
			my_reject_packet.received_segment_no=my_ack_packet.received_segment_no;
			my_reject_packet.end_of_packet_id=0XFFFF;
			//Send REJECT datagram to client
			n=sendto(sock,&my_reject_packet,sizeof(my_reject_packet),0,(struct sockaddr *)&from,fromlen);
			if(n<0)
			{
			error("sendto");
			}
			//Show sent reject datagram
			my_reject_packet.received_segment_no=my_data_packet.segment_no;
			reject_packet_display(my_reject_packet);
		}
		else
		{
			//If no error in the datagram
			segment=segment+1;
			//Send ack to client
			n=sendto(sock,&my_ack_packet,sizeof(my_ack_packet),0,(struct sockaddr *)&from,fromlen);
			if(n<0)
			{
				error("sendto");
			}
			//Show sent acked datagram
			my_ack_packet.received_segment_no=my_data_packet.segment_no;
			ack_packet_display(my_ack_packet);
		}
	}
	//Close the sock
	close(sock);
}
