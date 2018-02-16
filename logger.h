#ifndef LOGGER_H_
#define LOGGER_H_

#define DLVL_0 0U
#define DLVL_1 1U
#define DLVL_2 2U
#define DEBUG_LVL DLVL_1

#define DBG_BUF_MAX_SIZE 200U

#define INFO(...) vidPrintLn((char*)__VA_ARGS__);

extern void vidPrintLn(char * format, ...);



#endif