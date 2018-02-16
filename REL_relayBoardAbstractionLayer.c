#include <stdio.h>
#include <string.h>

#include "logger.h"
#include "UART_MiddleWare.h"
#include "REL_relayBoardAbstractionLayer.h"

#include <pthread.h>




#if (MAX_CMD_LENGTH > MAX_MSG_LNGTH)
#error "MAX_CMD_LENGTH cannt be greater than MAX_MSG_LNGTH"
#endif

/*MUST BE KEPT IN ORDER*/
 #define R0_ON  0U
#define R0_OFF 1U
 #define R1_ON  2U
#define R1_OFF 3U
 #define R2_ON  4U
#define R2_OFF 5U
 #define R3_ON  6U
#define R3_OFF 7U
 #define R4_ON  8U
#define R4_OFF 9U


pthread_mutex_t relay_state_mutex;


typedef enum{
	RELAY_OFF,
	RELAY_ON,
}enutRelayState;

typedef struct
{
	int relayIndx;
	int ralayState;
}strtRelay;

char * relaysCmds[]={ 
	[R0_ON ] = "#R01",
	[R0_OFF] = "#R00",
	[R1_ON ] = "#R11",
	[R1_OFF] = "#R10",
	[R2_ON ] = "#R21",
	[R2_OFF] = "#R20",
	[R3_ON ] = "#R31",
	[R3_OFF] = "#R30",
	[R4_ON ] = "#R41",
	[R4_OFF] = "#R40",
};

strtRelay boardRelays[NUM_OF_RELAYS];

extern void vidInitBoard(void){
	int i;

	for(i=0; i<NUM_OF_RELAYS; i++){
		boardRelays[i].relayIndx  = i;
		boardRelays[i].ralayState = RELAY_OFF;
	}
}

extern int getRelayState(int indx){

	int state;

	pthread_mutex_lock(&relay_state_mutex);
    state = boardRelays[indx].ralayState;
	pthread_mutex_unlock(&relay_state_mutex);

	return state;
}



extern void vidIncomingAckHandler(char * incMsg){
	char msg[MAX_CMD_LENGTH]={0};
	int i;
	strcpy(msg, incMsg);
	if(strstr(msg, "#R" )){
		for(i=0; i<NUM_OF_RELAYS*2; i++){
			if(strstr(relaysCmds[i], msg)){
				if (i%2==0){
					pthread_mutex_lock(&relay_state_mutex);
					boardRelays[i/2].ralayState = RELAY_ON;
					pthread_mutex_unlock(&relay_state_mutex);
				} else {
					pthread_mutex_lock(&relay_state_mutex);
					boardRelays[i].ralayState = RELAY_OFF;
					pthread_mutex_unlock(&relay_state_mutex);
				}
				break;
			}
		}
	} else {
		INFO("UNKNOWN CMD!!");
	}
}

extern void REL_vidRelayOn(int relayNumber){
	int rekayIndx = relayNumber*2;
	char cmdFormatted[MAX_CMD_LENGTH]={0};
	if (relayNumber>0 && relayNumber<=NUM_OF_RELAYS){
		sprintf(cmdFormatted, "%s\r", relaysCmds[rekayIndx]);
		send_command(cmdFormatted);

	} else {
		INFO("INVALID RELAY!!");
	}
}

extern void REL_vidRelayOff(int relayNumber){
	int rekayIndx = (relayNumber*2)+1;
	char cmdFormatted[MAX_CMD_LENGTH]={0};
	if (relayNumber>0 && relayNumber<=NUM_OF_RELAYS){
		sprintf(cmdFormatted, "%s\r", relaysCmds[rekayIndx]);
		send_command(relaysCmds[rekayIndx]);
	} else {
		INFO("INVALID RELAY!!");
	}
}



