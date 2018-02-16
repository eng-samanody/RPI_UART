#include <unistd.h>


#include "logger.h"
#include "REL_relayBoardAbstractionLayer.h"
#include "UART_MiddleWare.h"


void main (void){


	int i = 0;


	vidInitBoard();
	vidInitUartMiddleWare();

	while (1){
		INFO("main loop : %d ...",i++);
		
		sleep(1);
	}

}