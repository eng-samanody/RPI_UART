#ifndef RELAY_ABSTRACTION_H_
#define RELAY_ABSTRACTION_H_

#define NUM_OF_RELAYS 5U
#define MAX_CMD_LENGTH 20U



extern void vidInitBoard(void);
extern void vidIncomingAckHandler(char * incMsg);

extern int getRelayState(int indx);
extern void REL_vidRelayOn(int relayNumber);
extern void REL_vidRelayOff(int relayNumber);


#endif