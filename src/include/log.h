#ifndef __LOG_H__
#define __LOG_H__


#include <time.h>
#include <stdio.h>

#define LOGPATH     "../log/log.txt"

#define LOG(s) do{                  \
    time_t t;                       \
    time(&t);                       \
    FILE *fp;                       \
    fp = fopen(LOGPATH,"a+"); \
    fprintf(fp, "%s-%s [%s: %d] %s \n",__DATE__, __TIME__,__FILE__, __LINE__, s);\
    fclose(fp);                     \
}while(0)

#endif 