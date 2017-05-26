/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2017 John Denholm                                             *
*                                                                         *
* juggler.h - main includes and global config                             *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#define _GNU_SOURCE

#include <time.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/udp.h>


#ifndef Err
#define Err strerror( errno )
#endif

#define MAX_PACKET          16384

typedef struct target_port TRGT;
typedef struct source_port SRC;
typedef struct main_config CONF;
typedef struct thread_data THRD;

typedef void * throw_fn ( void * );

struct thread_data
{
    THRD                *   next;
    pthread_t               id;
    void                *   arg;
};



struct target_port
{
    TRGT                *   next;
    struct sockaddr_in      sa;
    int                     fd;
    char                *   host;
    uint16_t                port;
};


struct source_port
{
    SRC                 *   next;
    struct sockaddr_in      sa;
    //struct sockaddr_in      from;
    uint16_t                port;
    int                     fd;

    TRGT                *   targets;
    unsigned char       *   buf;
};


struct main_config
{
    TRGT                *   targets;
    SRC                 *   sources;

    int                     tcount;
    int                     scount;
    int                     max;

    int                     run;
};

CONF *cfg;



// thread.c
pthread_t thread_throw( void *(*fp) (void *), void *arg );
void *thread_loop( void *arg );

