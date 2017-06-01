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
        err( "Cannot set default attr state to detached -- %s", Err );
}

pthread_t thread_throw( void *(*fp) (void *), void *arg, int ctr )
{
    THRD *t;

    if( !tt_attr )
        thread_throw_init_attr( );

    t = (THRD *) calloc( 1, sizeof( THRD ) );
    t->arg = arg;
    t->ctr = ctr;
    pthread_create( &(t->id), tt_attr, fp, t );

    return t->id;
}


int thread_target_socket( TRGT *t, int id )
{
    int sk;

    if( ( sk = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP ) ) < 0 )
    {
        terr( "Could not open target socket: %s", Err );
        return -1;
    }

    if( connect( sk, (struct sockaddr *) &(t->sa), sizeof( struct sockaddr_in ) ) < 0 )
    {
        terr( "Could not set peer address to %s:%hu: %s", t->host, t->port, Err );
        close( sk );
        return -1;
    }

    tinfo( "Connected to %s:%hu", t->host, t->port );

    return sk;
}


TRGT *thread_targets( int id )
{
    TRGT *t, *ct, *targets;
    int i;

    targets = (TRGT *) calloc( cfg->tcount, sizeof( TRGT ) );

    // copy the targets, grabbing a socket on the way
    for( i = 0, ct = cfg->targets; ct; ct = ct->next, i++ )
    {
        t = targets + i;
        memcpy( t, ct, sizeof( TRGT ) );

        if( ( t->fd = thread_target_socket( t, id ) ) < 0 )
        {
            terr( "Cannot get socket for target %s:%hu.", t->host, t->port );
            free( targets );
            return NULL;
        }
    }

    // and link them in a circle
    for( i = 0; i < cfg->tcount; i++ )
        targets[i].next = &(targets[(i + 1) % cfg->tcount]);

    return targets;
}



int thread_socket( SRC *s, int id )
{
    struct timeval tv;
    int sk;

    if( ( sk = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP ) ) < 0 )
    {
        terr( "Could not open source socket: %s", Err );
        return -1;
    }

    tv.tv_sec  = 3;
    tv.tv_usec = 0;

    if( setsockopt( sk, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof( struct timeval ) ) )
    {
        terr( "Could not set source socket timeout: %s", Err );
        close( sk );
        return -1;
    }

    // bind our socket
    if( bind( sk, (struct sockaddr *) &(s->sa), sizeof( struct sockaddr_in ) ) )
    {
        terr( "Could not bind to source port %hu: %s", s->port, Err );
        close( sk );
        return -1;
    }

    tnotice( "Bound to port %hu.", ntohs( s->sa.sin_port ) );

    return sk;
}



void *thread_loop( void *arg )
{
    unsigned char *buf;
    ssize_t bc, sc;
    int sk, id;
    size_t max;
    THRD *ta;
    TRGT *t;
    SRC *s;

    ta  = (THRD *) arg;
    s   = (SRC *)  ta->arg;
    id  = ta->ctr;
    buf = (unsigned char *) calloc( 1, cfg->max );
    sk  = -1;

    // are we talking to remote targets
    if( cfg->tcount )
        // start t at the front
        t = thread_targets( id );
    else
        t = NULL;


    // leave room for a newline and a null
    max = cfg->max - 2;


    // now recv and push
    while( cfg->run )
    {
        // do we need a new socket?
        if( sk < 0 )
        {
            // are we recreating the socket yet?
            if( s->until > cfg->ts )
            {
                usleep( 25000 );
                continue;
            }

            if( ( sk = thread_socket( s, id ) ) < 0 )
            {
                terr( "Cannot get socket for port %hu.", s->port );
                s->until = cfg->ts + cfg->delay;
                continue;
            }
        }

        if( ( bc = recv( sk, buf, max, 0 ) ) < 0 )
        {
            if( errno == EINTR || errno == EAGAIN )
                continue;

            // eeep, close down the socket
            terr( "Recv error on source port %hu: %s", s->port, Err );
            close( sk );
            sk = -1;

            s->until = cfg->ts + cfg->delay;
            continue;
        }

        if( !bc )
            continue;

        // cap it
        buf[bc] = '\0';

        // target or stdout?
        if( t )
        {
            // move on one
            t = t->next;

            if( t->fd < 0 )
            {
                if( t->until > cfg->ts )
                    continue;

                if( ( t->fd = thread_target_socket( t, id ) ) < 0 )
                {
                    terr( "Unable to get socket for %s:%hu.", t->host, t->port );
                    t->until = cfg->ts + cfg->delay;
                    continue;
                }
            }

            if( ( sc = send( t->fd, buf, bc, 0 ) ) < 0 )
            {
                terr( "Send error to %s:%hu: %s", t->host, t->port, Err );
                close( t->fd );
                t->fd = -1;

                t->until = cfg->ts + cfg->delay;
                continue;
            }
            else if( sc < bc )
                twarn( "Only sent %ld out of %ld bytes to %s:%hu",
                        sc, bc, t->host, t->port );
        }
        else
        {
            if( buf[bc - 1] != '\n' )
            {
                buf[bc++] = '\n';
                buf[bc]   = '\0';
            }
            fwrite( buf, bc, 1, stdout );
        }
    }

    free( ta );
    return NULL;
}



