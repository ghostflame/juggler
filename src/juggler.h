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
#include <stdarg.h>
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

enum log_levels
{
    LOG_ERROR = 0,
    LOG_WARN,
    LOG_NOTICE,
    LOG_INFO,
    LOG_DEBUG
};



typedef struct target_port TRGT;
typedef struct source_port SRC;
typedef struct main_config CONF;
typedef struct thread_data THRD;

typedef void * throw_fn ( void * );

struct thread_data
{
    THRD                *   next;
    pthread_t               id;
    int                     ctr;
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
    uint16_t                port;
};


struct main_config
{
    TRGT                *   targets;
    SRC                 *   sources;

    int                     tcount;
    int                     scount;
    int                     max;
    int                     level;

    int                     run;
};

CONF *cfg;


// main.c
int logger( int level, int id, char *fmt, ... );

// thread.c
pthread_t thread_throw( void *(*fp) (void *), void *arg, int ctr );
void *thread_loop( void *arg );


#define err( ... )          logger( LOG_ERROR,  -1, ## __VA_ARGS__ )
#define warn( ... )         logger( LOG_WARN,   -1, ## __VA_ARGS__ )
#define notice( ... )       logger( LOG_NOTICE, -1, ## __VA_ARGS__ )
#define info( ... )         logger( LOG_INFO,   -1, ## __VA_ARGS__ )
#define debug( ... )        logger( LOG_DEBUG,  -1, ## __VA_ARGS__ )


#define terr( ... )         logger( LOG_ERROR,  id, ## __VA_ARGS__ )
#define twarn( ... )        logger( LOG_WARN,   id, ## __VA_ARGS__ )
#define tnotice( ... )      logger( LOG_NOTICE, id, ## __VA_ARGS__ )
#define tinfo( ... )        logger( LOG_INFO,   id, ## __VA_ARGS__ )
#define tdebug( ... )       logger( LOG_DEBUG,  id, ## __VA_ARGS__ )



