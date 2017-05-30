/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2017 John Denholm                                             *
*                                                                         *
* thread.c - pthreads control, source loop function                       *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "juggler.h"

pthread_attr_t *tt_attr = NULL;

void thread_throw_init_attr( void )
{
    tt_attr = (pthread_attr_t *) calloc( 1, sizeof( pthread_attr_t ) );
    pthread_attr_init( tt_attr );

    if( pthread_attr_setdetachstate( tt_attr, PTHREAD_CREATE_DETACHED ) )
        fprintf( stderr, "Cannot set default attr state to detached -- %s", Err );
}

pthread_t thread_throw( void *(*fp) (void *), void *arg )
{
    THRD *t;

    if( !tt_attr )
        thread_throw_init_attr( );

    t = (THRD *) calloc( 1, sizeof( THRD ) );
    t->arg = arg;
    pthread_create( &(t->id), tt_attr, fp, t );

    return t->id;
}




void *thread_loop( void *arg )
{
    struct timeval tv;
    ssize_t bc, sc;
    //socklen_t slen;
    TRGT *t, *ot;
    THRD *ta;
    SRC *s;

    ta = (THRD *) arg;
    s  = (SRC *)  ta->arg;
    free( ta );

    s->buf     = (unsigned char *) calloc( 1, cfg->max );
    tv.tv_sec  = 3;
    tv.tv_usec = 0;

    // copy the targets, grabbing a socket on the way
    for( ot = cfg->targets; ot; ot = ot->next )
    {
        t = (TRGT *) calloc( 1, sizeof( TRGT ) );
        
        memcpy( t, ot, sizeof( TRGT ) );

        if( ( t->fd = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP ) ) < 0 )
        {
            fprintf( stderr, "Could not open target socket: %s\n", Err );
            exit( 1 );
        }

        t->next = s->targets;
        s->targets = t;
    }

    // find the end of the list
    for( t = s->targets; t->next; t = t->next );

    // and join up the list
    t->next = s->targets;

    if( ( s->fd = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP ) ) < 0 )
    {
        fprintf( stderr, "Could not open source socket: %s\n", Err );
        exit( 1 );
    }
    if( setsockopt( s->fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof( struct timeval ) ) )
    {
        fprintf( stderr, "Could not set source socket timeout: %s\n", Err );
        exit( 1 );
    }

    if( bind( s->fd, (struct sockaddr *) &(s->sa), sizeof( struct sockaddr_in ) ) )
    {
        fprintf( stderr, "Could not bind to source port %hu: %s\n", s->port, Err );
        exit( 1 );
    }
    printf( "Bound to port %hu.\n", ntohs( s->sa.sin_port ) );

    // start t at the front
    t = s->targets;

    while( cfg->run )
    {
        // slen = sizeof( struct sockaddr_in );
        // if( ( bc = recvfrom( s->fd, s->buf, cfg->max, 0, (struct sockaddr *) &(s->sa), &slen ) ) < 0 )
        if( ( bc = recv( s->fd, s->buf, cfg->max, 0 ) ) < 0 )
        {
            if( errno == EINTR || errno == EAGAIN )
                continue;

            fprintf( stderr, "Recv error on source port %hu: %s\n", s->port, Err );
            exit( 1 );
        }

        if( !bc )
            continue;

        // send that back out
        if( ( sc = sendto( t->fd, s->buf, bc, 0, (struct sockaddr *) &(t->sa),
                        sizeof( struct sockaddr_in ) ) ) < bc )
            printf( "Only sent %ld out of %ld bytes to %s\n",
                    sc, bc, t->host );

        // and move t on
        t = t->next;
    }

    return NULL;
}


