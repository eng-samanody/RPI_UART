#ifndef UART_H_
#define UART_H_



#define UART_DEVICE "/dev/ttyAMA0"
#define BAUD_RATE 9600U

#define MAX_MSG_LNGTH  100U
#define MAX_NO_OF_MSGS 100U
#define MSG_DELIMITER '\n'

extern int uart_open(void);
extern int uart_close(void);
extern int send_command(char * cmd);
extern void vidInitUartMiddleWare(void);
extern unsigned char getBoardDeviceState(void);


#endif