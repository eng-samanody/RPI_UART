# RPI_UART
A raspberry UART library in C. This lib is a part of a layered SW architecture to serve as an interface layer between the SW and UART. This lib uses POSIX threads to continuosly read UART data in a non blocking manner. It also uses FIFO Buffer to accept transmit  requests and Incoming data. Separate threads are added to dispatch Tx commands from the FIFO and and Process Incoming data. 

Build Command:
gcc -pthread UART_MiddleWare.c REL_relayBoardAbstractionLayer.c logger.c main.c -o main.exe
