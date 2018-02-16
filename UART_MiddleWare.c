/*
 *
 * This is pseudo code but outlines the functionality i envision for
 * the UART driver i would like to have done.
 *
 * The way i envision it uses interrupts to retrieve a new char from 
 * the UART. Once a complete line is received it will add it to a list
 * (queue, linked list, ?) for later processing by the main loop.
 *
 * I don't actually mind whether to use interrupts, or a non-blocking
 * select call, important is that reading from the UART does not block 
 * the complete loop or collides with other system activities
 *
 * For sending stuff to the uart device id like to have the same. Firstly, 
 * strings\n to send will get added to a list. Then at some point in the main
 * loop and item from this list will get picked and sent. 
 *
 * You will have to add more libraries, functions, defines, variables most likely. 
 * That is fine. 
 * But the function names that i already wrote i would like to see implemented.
 *
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>

#include "logger.h"
#include "REL_relayBoardAbstractionLayer.h"
#include "UART_MiddleWare.h"

#include <pthread.h>


#define E_NOK 0U
#define E_OK  1U

typedef enum{
	UART_UNINITIALIZED,
	//UART_INITIALIZED,
	UART_CONNECTED,
	//UART_DISCONNECTED,
}enutUartState;

typedef enum{
	CHANNEL_BUSY,
	CHANNEL_FREE,
}enutChannelState;



int sfd=-1;
unsigned char u8UartCurrentState = UART_UNINITIALIZED;
unsigned char u8ChannelCurrentState = CHANNEL_BUSY;
pthread_mutex_t states_mutex;

unsigned char buffer[MAX_MSG_LNGTH]={0};

// Represents a queue/list for strings received
// by the UART
unsigned char incoming_list[MAX_NO_OF_MSGS][MAX_MSG_LNGTH];
unsigned int  incoming_msgs_count = 0;
pthread_mutex_t incoming_msgs_mutex;


// Represents a queue/list for strings to be
// send to the UART
unsigned char outgoing_list[MAX_NO_OF_MSGS][MAX_MSG_LNGTH];
unsigned int  outgoing_msgs_count = 0;
pthread_mutex_t outgoing_msgs_mutex;

static int is_device_ready(void);

extern unsigned char getDeviceState(void){
	unsigned char state;
	pthread_mutex_lock(&states_mutex);
	state = u8UartCurrentState;
	pthread_mutex_unlock(&states_mutex);
	return state;
}


// Returns how many items are still
// in this list
int count_incoming_list(){
	int inc_msgs_count;
	pthread_mutex_lock(&incoming_msgs_mutex);
    inc_msgs_count = incoming_msgs_count;
	pthread_mutex_unlock(&incoming_msgs_mutex);
	return inc_msgs_count;
}

// Returns how many items are still
// in this list
int count_outgoing_list(){
	int out_msgs_count;
	pthread_mutex_lock(&outgoing_msgs_mutex);
    out_msgs_count = outgoing_msgs_count;
	pthread_mutex_unlock(&outgoing_msgs_mutex);
	return out_msgs_count;
}

// adds new item to incoming list
int add_to_incoming(char cmd[]){
	pthread_mutex_lock(&incoming_msgs_mutex);
    strcpy(incoming_list[incoming_msgs_count], cmd);
    incoming_msgs_count = (incoming_msgs_count+1) % MAX_NO_OF_MSGS;
    pthread_mutex_unlock(&incoming_msgs_mutex);
	return E_OK;
}

// adds new item to outgoing list
int add_to_outgoing(char cmd[]){
	pthread_mutex_lock(&outgoing_msgs_mutex);
	strcpy(outgoing_list[outgoing_msgs_count], cmd);
    outgoing_msgs_count = (outgoing_msgs_count+1) % MAX_NO_OF_MSGS;
    pthread_mutex_unlock(&outgoing_msgs_mutex);
	return E_OK;
}

int get_next_incoming(char * cmd){
	// 1.) get next item from list (first come first serve)
	// 2.) delete item from list
	// 3.) return item as string
	int i;
	pthread_mutex_lock(&incoming_msgs_mutex);
	if(strlen(incoming_list[0])>0 && incoming_msgs_count>0){
		strcpy(cmd, incoming_list[0]);
		memset(incoming_list[0], '\0', MAX_MSG_LNGTH);
		for(i = 0; i<=incoming_msgs_count; i++){
			strcpy(incoming_list[i], incoming_list[i+1]);
		}
		incoming_msgs_count--;
	} else {
		pthread_mutex_unlock(&incoming_msgs_mutex);
		return E_NOK;
	}
	pthread_mutex_unlock(&incoming_msgs_mutex);
	
	return E_OK;
}

int get_next_outgoing(char * cmd){
	// 1.) get next item from list
	// 2.) delete item from list
	// 3.) return item as string
	int i;
	pthread_mutex_lock(&outgoing_msgs_mutex);

	if(strlen(outgoing_list[0])>0 && outgoing_msgs_count>0 ){
		strcpy(cmd, outgoing_list[0]);
		memset(outgoing_list[0], '\0', MAX_MSG_LNGTH);
		for(i = 0; i<=outgoing_msgs_count; i++){
			strcpy(outgoing_list[i], outgoing_list[i+1]);
		}
		if(outgoing_msgs_count>0){
			outgoing_msgs_count--;
		}
	} else {
		pthread_mutex_unlock(&outgoing_msgs_mutex);
		return E_NOK;
	}
	pthread_mutex_unlock(&outgoing_msgs_mutex);

	return E_OK;
}


// open connection to uart 
extern int uart_open(void){ 
	INFO("UART starting....");
	int device_ready = is_device_ready();
	pthread_mutex_lock(&states_mutex);
	if( device_ready && u8UartCurrentState == UART_UNINITIALIZED){
		sfd = open(UART_DEVICE, O_RDWR | O_NOCTTY);
		if (sfd == -1) {
			pthread_mutex_unlock(&states_mutex);
			INFO("Error no is : %d\n", errno);
			INFO("Error description is : %s\n", strerror(errno));
			return (E_NOK);
		}
		struct termios options;
		tcgetattr(sfd, &options);
		cfsetspeed(&options, B9600);
		cfmakeraw(&options);
		options.c_cflag &= ~CSTOPB;
		options.c_cflag |= CLOCAL;
		options.c_cflag |= CREAD;
		options.c_cc[VTIME]=0;
		options.c_cc[VMIN]=0;
		tcsetattr(sfd, TCSANOW, &options);

		u8UartCurrentState = UART_CONNECTED;
		u8ChannelCurrentState = CHANNEL_FREE;
		
		pthread_mutex_unlock(&states_mutex);
		INFO("UART Started ...");
	} else {
		pthread_mutex_unlock(&states_mutex);
		return (E_NOK);
	}
	return E_OK;
}

// close the connection
extern int uart_close(void){
	INFO("Terminating UART Connection...");
	pthread_mutex_lock(&states_mutex);
	if(u8UartCurrentState == UART_CONNECTED){
		close(sfd);
		sfd=-1;
		u8UartCurrentState = UART_UNINITIALIZED;
		pthread_mutex_unlock(&states_mutex);	
	} else {
		pthread_mutex_unlock(&states_mutex);
		INFO("No connection opened!!");
		return E_NOK;
	}
	
	return E_OK;
}

// check if the UART_DEVICE exists, meaning is plugged in
// maybe some other fancy methods to check?
static int is_device_ready(void){

    if(!access(UART_DEVICE, F_OK )){
        INFO("The Device %s\t was Found",UART_DEVICE);
    }else{
    	pthread_mutex_lock(&states_mutex);
    	u8UartCurrentState = UART_UNINITIALIZED;
    	sfd=-1;
    	pthread_mutex_unlock(&states_mutex);
        INFO("The Device %s\t not Found",UART_DEVICE);
		return E_NOK;
    }

    return E_OK;
}

extern int send_command(char * cmd){
	int count = 0;
	pthread_mutex_lock(&states_mutex);
	if (u8UartCurrentState == UART_CONNECTED && u8ChannelCurrentState == CHANNEL_FREE){
		u8ChannelCurrentState = CHANNEL_BUSY;
		count = write(sfd, cmd, strlen(cmd));
		u8ChannelCurrentState = CHANNEL_FREE;
		pthread_mutex_unlock(&states_mutex);
		if (count == 0){
            add_to_outgoing(cmd);
			return E_NOK;
		} 
		INFO("CMD %s sent \t\t [OK]\n", cmd);
	} else {
		pthread_mutex_unlock(&states_mutex);
		add_to_outgoing(cmd);
		INFO("Sorry couldn't send. Maybe device have issue or busy");
		return E_NOK;	
	}
    return E_OK;
}

void* outgoing_command_dispatcher(void *arg){
	char cmd[MAX_MSG_LNGTH]={0};
	while (1){
		if(get_next_outgoing(cmd)){
			send_command(cmd);
		} else {
			INFO("No further outgoing messages");
		}
		sleep(1);
	}

	return NULL;
}

void* incoming_command_handler(void *arg){
	char cmd[MAX_MSG_LNGTH]={0};
	while (1){
		if(get_next_incoming(cmd)){
			INFO(">> handling : %s cmd", cmd);
			vidIncomingAckHandler(cmd);
			memset(cmd,'\0',MAX_MSG_LNGTH);
		} else {
			INFO("No further incoming messages");
		}
		sleep(1);
	}
}

// This will attempt a non blocking reading for byte if available on the UART_DEVICE in a non blocking manner
void *  new_data (void *arg){
	char incomingChar;
	static int indx=0;
	int count=0;
	while(1){
		count = read(sfd, &incomingChar, 1);
		if(count!=0){
			if(incomingChar != MSG_DELIMITER){
				buffer[indx]=incomingChar;
				//printf("%c" , incomingChar);
				indx = (indx+1) % MAX_MSG_LNGTH;
			} else {
				//copy buffer
				add_to_incoming(buffer);
				indx=0;
				INFO(">> %s", buffer);
				memset(buffer,'\0',MAX_MSG_LNGTH);
			}
		}
		//usleep(1000);
	}
}

void* deviceCheck(void *arg){
	while(1){
		is_device_ready();
		sleep(2);
	}
}



pthread_t receivingThread;
pthread_t cmdHandlerThread;
pthread_t cmdDispatcherThread;
pthread_t deviceCheckThread;

extern void vidInitUartMiddleWare(void){

	int  iret;
	
	vidInitBoard();
	//getDeviceState() != UART_CONNECTED
	while(is_device_ready()!=E_OK){
		INFO("Waiting for device %s to be plugged in\n", UART_DEVICE);
		sleep(1);
	}

	uart_open();

	iret = pthread_create( &receivingThread, NULL, new_data, NULL);
	if(iret){
		INFO("Error - pthread_create(Reading Thread) ");
		exit(EXIT_FAILURE);
	}

	iret = pthread_create( &cmdHandlerThread, NULL, incoming_command_handler, NULL);
	if(iret){
		INFO("Error - pthread_create(Incoming Command Handler)");
		exit(EXIT_FAILURE);
	}

	iret = pthread_create( &cmdDispatcherThread, NULL, outgoing_command_dispatcher, NULL);
	if(iret){
		INFO("Error - pthread_create(Outgoing Command Dispatcher)");
		exit(EXIT_FAILURE);
	}

	iret = pthread_create( &deviceCheckThread, NULL, deviceCheck, NULL);
	if(iret){
		INFO("Error - pthread_create(Outgoing Command Dispatcher)");
		exit(EXIT_FAILURE);
	}

}



