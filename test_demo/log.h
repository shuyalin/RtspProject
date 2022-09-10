#ifndef _LOG_H
#define _LOG_H
#include "stdio.h"
#define lev_error 0
#define lev_warning 3
#define loglev 2

#define NONE                      "\033[m"
#define RED                         "\033[0;32;31m"
#define LIGHT_RED           "\033[1;31m"
#define GREEN                   "\033[0;32;32m"
#define LIGHT_GREEN     "\033[1;32m"
#define BLUE                       "\033[0;32;34m"
#define LIGHT_BLUE         "\033[1;34m"
#define DARY_GRAY         "\033[1;30m"
#define CYAN                       "\033[0;36m"
#define LIGHT_CYAN         "\033[1;36m"
#define PURPLE                 "\033[0;35m"
#define LIGHT_PURPLE   "\033[1;35m"
#define BROWN                  "\033[0;33m"
#define YELLOW                 "\033[1;33m"
#define LIGHT_GRAY        "\033[0;37m"
#define WHITE                    "\033[1;37m"

#define logs(format,...) do{if(loglev >1)printf(""__FILE__">>LINE: %d:"LIGHT_GREEN format"\n"NONE, __LINE__,##__VA_ARGS__);}while(0)
#define log1(format,...) do{if(loglev >2)printf(""__FILE__">>LINE: %d:"format"\n", __LINE__,##__VA_ARGS__);}while(0)
#define log2(format,...) do{if(loglev >3)printf(""__FILE__">>LINE: %d:"format"\n", __LINE__,##__VA_ARGS__);}while(0)
#define log3(format,...) do{if(loglev >4)printf(""__FILE__">>LINE: %d:"format"\n", __LINE__,##__VA_ARGS__);}while(0)
#define log4(format,...) do{if(loglev >5)printf(""__FILE__">>LINE: %d:"format"\n", __LINE__,##__VA_ARGS__);}while(0)
#define logerror(format,...) do{if(loglev >lev_error)printf(""__FILE__">>%s LINE: %d:"#format"\n"NONE, RED"myerror", __LINE__,##__VA_ARGS__);}while(0)

#endif

