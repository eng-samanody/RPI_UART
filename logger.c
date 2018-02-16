#include "logger.h"
#include <stdarg.h>
#include <stdio.h>



extern void vidPrintLn(char * format, ...){
#if ( DEBUG_LVL >= DLVL_1)
    char debugMsg[DBG_BUF_MAX_SIZE];
    va_list ap;
    va_start(ap, format);
    vsnprintf(debugMsg, DBG_BUF_MAX_SIZE, format, ap);
    printf("%s\n",debugMsg);
    va_end(ap);
#endif
}