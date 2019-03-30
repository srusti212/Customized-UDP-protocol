/*Creates a datagram server. POrt number is passed as an argument. This server runs forever 

To run:
gcc secondserver.c -o secondserver
./secondserver 4547
 For ack timerfunctionality execution, close server terminal, and try to send request datagram

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
#include <inttypes.h>
 
int k=0,t=0;

void error(char *msg)
{
	perror(msg);
	exit(0);
}

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
		unsigned int response_type;			//2 bytes		//1. ACK=0XFFFB.... 2. NOT PAID=0XFFF9.... 3. NOT EXIST=0XFFFA
		uint8_t segment_no;				//1 byte
		uint8_t length;					//1 byte
		uint8_t technology;				//1 byte
		unsigned long source_subscriber_no;		//4 byte
		unsigned int end_of_packet_id;			//2 bytes	
		};

	//Verification Database
	struct verif_database{
		unsigned long subscriber_no;
		uint8_t technology;
		uint8_t paid;
	};

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

int read_database(struct verif_database vd[]){
	//To read the line
	char l[40];
	FILE *f;
	//try to open file
	f=fopen("verificationdatabase","r");
	//if file is unable to open
	if(f==NULL)
	{
		error("\n\tError : Failed to open database file!");
	}
	else
	{
		//leave out the 1st line
		fgets(l,sizeof(l),f);
		//printout first line	
		printf("\n\t%s",l);
		while((fgets(l,sizeof(l),f))!=NULL)
		{
		//printout each line from 2nd line to last line
		printf("\n\t%s",l);
		char *lineholder;
		lineholder = strtok(l," ");
	
		vd[k].subscriber_no = (unsigned long)atol(lineholder);
		//tells the function to resume from the last spot in the string
		lineholder=strtok(NULL," ");
	
		vd[k].technology = (uint8_t)atoi(lineholder);
		//tells the function to resume from the last spot in the string
		lineholder=strtok(NULL," ");
	
		vd[k].paid = (uint8_t)atoi(lineholder);
		//tells the function to resume from the last spot in the string
		lineholder=strtok(NULL," ");
		//printf("\n\t%lu\t%d\t%d\n",vd[k].subscriber_no,vd[k].technology,vd[k].paid);
		k=k+1;
		}
	fclose(f);
	}
	return k;
}

int main(int argc,char *argv[])
{     

	int sock,length,fromlen,n;
	struct sockaddr_in server;
	struct sockaddr_in from;
	struct request_packet my_request_packet;
	struct response_packet  my_response_packet;

	//initialize my_response_packet values
	my_response_packet.start_of_packet_id=0XFFFF;
	my_response_packet.client_id=0XFF;
	my_response_packet.response_type=0XFFFA;			//1. ACK=0XFFFB.... 2. NOT PAID=0XFFF9.... 3. NOT EXIST=0XFFFA
	my_response_packet.segment_no=my_request_packet.segment_no;
	my_response_packet.length=0XFF;
	my_response_packet.technology=1;			// by default setting technology to 1 by default
	my_response_packet.source_subscriber_no=4294967295;	// by default setting max no. as subscriber no.
	my_response_packet.end_of_packet_id=0XFFFF;	
	//create array of struct to hold 3 records
	struct verif_database vd[100];
	//Print Contents of Verification Database
	printf("\n\t------------Contents of Verification Database------------\n\n");
	t=read_database(vd);
	char buf[1024];

	if(argc<2)
	{
	fprintf(stderr,"ERROR, no port provided\n");
	exit(0);
	}
	//AF_NET refers to IP addresses(addresses in internet);SOCK_DGRAM refers to datagrams on connection-less; 0 because using UDP; sock(domain,type,protocol);socket() creates a socket in the particular domain, of type and uses protocol
	sock=socket(AF_INET,SOCK_DGRAM,0);
	//If socket building is not successfull
	if(sock<0)
	{
		error("Opening socket failed");
	}
	length = sizeof(server);
	bzero(&server,length);
	server.sin_family=AF_INET;
	server.sin_addr.s_addr=INADDR_ANY;
	server.sin_port=htons(atoi(argv[1]));
	//bind() call allows a process to specify the local address of the socket
	if(bind(sock,(struct sockaddr *)&server,length)<0)
	{
		error("building");
	}
	fromlen = sizeof(struct sockaddr_in);
	//while server is alive
	while(1)
	{
				
		//To receive message from socket
		n=recvfrom(sock,&my_request_packet,sizeof(my_request_packet),0,(struct sockaddr *)&from,&fromlen);
		//if unable to receive message from socket
		if(n<0)
		{
			error("Failed to receive any request");
		}
		// Show the received data datagram
		request_packet_display(my_request_packet);
		// To recalculate for new user
		int user_exists=0;
		int user_paid=0;
		int user_technology=0;
		int allokay=0;

		my_response_packet.start_of_packet_id=0XFFFF;
		my_response_packet.client_id=0XFF;
		my_response_packet.segment_no=my_request_packet.segment_no;
		my_response_packet.length=0XFF;
		my_response_packet.technology=my_request_packet.technology;			// by default setting technology to 1 by default
		my_response_packet.source_subscriber_no=my_request_packet.source_subscriber_no;	// by default setting max no. as subscriber no.
		my_response_packet.end_of_packet_id=0XFFFF;
		
		//check user exists or not
		for(int i=0;i<t;i++)
		{
			if((vd[i].subscriber_no==my_request_packet.source_subscriber_no) && (vd[i].technology==my_request_packet.technology))
				{
				user_exists=1;
				user_technology=1;
				user_paid = vd[i].paid;
				break;
				}
		}
		if(user_paid==1)							//user exists, has paid for correct technology
			{
				allokay=1;
			}
	
		//Case 1. Check whether user exists, has paid for the correct tecnology--------------------
		if(user_exists==1 && user_technology==1 && user_paid==1 && allokay==1)
		{
			my_response_packet.response_type=0XFFFB;					//1. ACK=0XFFFB.... 2. NOT PAID=0XFFF9.... 3. NOT EXIST=0XFFFA	

		}

		//Case 2. Check whether existing user has paid or not ----------------------		
		else if(user_exists==1 && user_technology==1 && user_paid!=1)
		{
			my_response_packet.response_type=0XFFF9	;					//1. ACK=0XFFFB.... 2. NOT PAID=0XFFF9.... 3. NOT EXIST=0XFFFA	

		}

		//Case 3. DEfault: User does not exist----------------------
		 else
		{
			my_response_packet.response_type=0XFFFA;					//1. ACK=0XFFFB.... 2. NOT PAID=0XFFF9.... 3. NOT EXIST=0XFFFA	

		}
			//Send response datagram to client
			n=sendto(sock,&my_response_packet,sizeof(my_response_packet),0,(struct sockaddr *)&from,fromlen);
			if(n<0)
			{
			error("Failed to send a response");
			}
			//Show sent response datagram
			response_packet_display(my_response_packet);
			//Print Contents of Verification Database
			printf("\n\t------------Contents of Verification Database------------\n\n");
			t=read_database(vd);

	}
	//close the socket
	close(sock);
}
