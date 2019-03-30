/* To run:
gcc secondclient.c -o secondclient
./secondclient localhost 4547

For ack timerfunctionality execution, close server terminal, and try to send 1 datagram

PS: Let the verification database file stay in the same folder as this file
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
#include <stdint.h>

int segment=1;
char option;

        //Access Request datagram format
	struct request_packet{
		unsigned int start_of_packet_id;		//2 bytes
		uint8_t client_id;				//1 byte
		unsigned int acc_per;				//2 bytes
		uint8_t segment_no;				//1 byte
		uint8_t length;					//1 byte
		uint8_t technology;				//1 byte
		unsigned long source_subscriber_no;		//4 byte
		unsigned int end_of_packet_id;			//2 bytes
	};

        //Access Response datagram format
	struct response_packet{
		unsigned int start_of_packet_id;		//2 bytes
		uint8_t client_id;				//1 byte
		unsigned int response_type;			//2 bytes		1. ACK=0XFFFB.... 2. NOT PAID=0XFFF9.... 3. NOT EXIST=0XFFFA
		uint8_t segment_no;				//1 byte
		uint8_t length;					//1 byte
		uint8_t technology;				//1 byte
		unsigned long source_subscriber_no;		//4 byte
		unsigned int end_of_packet_id;			//2 bytes	
		};


void error(char *msg)
{	
	perror(msg);
	exit(0);
}

//Display access permission request
	void request_packet_display(struct request_packet my_request_packet){
	printf("\n\t--------------------------------------------------------------------------------------------");
	printf("\n\tCLient is sending following access request DATAGRAM to the server ->\n");
	printf("\n\tStart of Packet id : %x",my_request_packet.start_of_packet_id);
	printf("\n\tClient ID : %x", my_request_packet.client_id);
	printf("\n\tAccess Permsission : %x", my_request_packet.acc_per);
	printf("\n\tSegment Number : %d", my_request_packet.segment_no);
	printf("\n\tLength : %x", my_request_packet.length);
	printf("\n\tTechnology : %d", my_request_packet.technology);
	printf("\n\tSource Subscriber Number: %lu", my_request_packet.source_subscriber_no);
	printf("\n\tEnd of Packet ID : %x", my_request_packet.end_of_packet_id);
	printf("\n");
}

//Display access response
void response_packet_display(struct response_packet my_response_packet){
	printf("\n\t____________________________________________________________________________________________\n");
	printf("\n\tRemember  1. Access Permitted Code=0XFFFB  2. NOT PAID CODE=0XFFF9  3. NOT EXIST CODE=0XFFFA");
	printf("\n\t____________________________________________________________________________________________\n");
	printf("\n\tCLient received the following response datagram from the server ->\n");
	printf("\n\tStart of Packet id : %x",my_response_packet.start_of_packet_id);
	printf("\n\tClient ID : %x", my_response_packet.client_id);
	printf("\n\tResponse Type : %x", my_response_packet.response_type);
	printf("\n\tSegment Number : %d", my_response_packet.segment_no);
	printf("\n\tLength : %x", my_response_packet.length);
	printf("\n\tTechnology : %d", my_response_packet.technology);
	printf("\n\tSource Subscriber Number: %lu", my_response_packet.source_subscriber_no);
	printf("\n\tEnd of Packet ID : %x", my_response_packet.end_of_packet_id);
	printf("\n");
}

int main(int argc, char *argv[])
{

	int sock,length,n=0;
	struct sockaddr_in server,from;	
	struct hostent *hp;
	//declare
	struct request_packet my_request_packet;
	struct response_packet  my_response_packet;

	// try to obtain response from server 3 times
	int retry_counter=0;

	//data_packet_value_initialization
	my_request_packet.start_of_packet_id=0XFFFF;
	my_request_packet.client_id=0XFF;
	my_request_packet.acc_per=0XFFF8;
	my_request_packet.segment_no=segment;
	my_request_packet.length=0xFF;
	my_request_packet.source_subscriber_no=4294967295;
	my_request_packet.end_of_packet_id=0XFFFF;

	if(argc !=3)
	{
		error("\n\tError: Too little arguments to connect to the server");
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
		error("\n\t Error : Unknown host");
	}
	//copying the values of the server address
	bcopy((char *)hp->h_addr,(char *)&server.sin_addr,hp->h_length);
	//hton converts converts port number entered by user to network byte order(i.e. network type)
	server.sin_port = htons(atoi(argv[2]));
	length=sizeof(struct sockaddr_in);
	//For timer
	struct timeval tv;
	tv.tv_sec = 3;
	tv.tv_usec = 0;

	/*setsockoptn() allows setting options for socket. If no error occurs, setsockoptn returns 0.... SO_RCVTIMEO sets timeout in milliseconds for blocking the receiving call; SOL_SOCKET is a socket layer itself. Used for options that are protocol independent; (const char*)&tv is a pointer to the buffer in which the value for the requested option is specified */
	setsockopt(sock,SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(struct timeval) );
	int tech;
	do
	{
	n=0;
	retry_counter=0;
	unsigned long input_subs_no;
	char input[10];

	my_request_packet.start_of_packet_id=0XFFFF;
	my_request_packet.client_id=0XFF;
	my_request_packet.acc_per=0XFFF8;
	my_request_packet.segment_no=segment;
	my_request_packet.length= 0XFF;
	my_request_packet.end_of_packet_id=0XFFFF;

	//ASk user whether to quit or continue
	printf("\n\t--------------------------------------------------------------------------------------------\n");
	printf("\n\tPress any key to continue or q to EXIT : ");
	scanf("%s", &option);
	if(option=='q')
	{
		exit(0);
	}
	//Ask user for subscriber number
	printf("\n\t1. Enter subscriber number (e.g. 4086146234): ");
	scanf("%s",input);
	input_subs_no = atol(input);		//atol converts string to long integer
	while(input_subs_no<1000000000 || input_subs_no>4294967295)
	{
	printf("\n\t!!! Re-enter a valid subscriber number (between 1000000000 and 4294967295): ");
	scanf("%s",input);
	input_subs_no = atol(input);		//atol converts string to long integer
	}
	my_request_packet.source_subscriber_no=input_subs_no;

	//ASk user for technology
	printf("\n\t2. Enter technology (e.g. 2,3,4 only) : ");
	scanf("%d",&tech);
	while(tech<2 || tech>4)
	{
	printf("\n\t!!! Re-enter a valid technology (e.g. 2,3,4 only) : ");
	scanf("%d",&tech);	
	}
	my_request_packet.technology=tech; 

		// Display the access request packet
		request_packet_display(my_request_packet);

		//checks if acccess permission is recieved or not after sending; if not, the request datagram is resent thrice
		while((n<=0) && (retry_counter<3))
		{
		sendto(sock,&my_request_packet,sizeof(my_request_packet),0,(struct sockaddr *) &server,length);	
		if(n<0)  //If no response was recieved from server
		{
			printf("\n\tNo response from server for three seconds. Resending the datagram.");
			retry_counter++;
		}	
		n=recvfrom(sock,&my_response_packet,sizeof(my_response_packet),0,NULL,NULL);
		//If retry counter is 3, send a message and terminate the program
		}
		if(retry_counter==3)
		{
			printf("\n\tServer failed to respond after 3 attempts.\n");
			exit(0);
		}
		my_response_packet.segment_no = my_request_packet.segment_no;
		//Display response from server
		response_packet_display(my_response_packet);
		//Increment segment and assign for next user request
		segment=segment+1;
		my_request_packet.segment_no = segment;
	}while(option!='q');
	close(sock);
}
