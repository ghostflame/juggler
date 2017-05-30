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

        if( ( t->fd = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP ) ) < 0 )
        {
            terr( "Could not open target socket: %s", Err );
            free( targets );
            return NULL;
        }
        tinfo( "Opened socket %d to %s:%hu\n", t->fd, t->host, t->port );

        if( connect( t->fd, (struct sockaddr *) &(t->sa), sizeof( struct sockaddr_in ) ) < 0 )
        {
            terr( "Could not set peer address to %s:%hu: %s", t->host, t->port, Err );
            free( targets );
            return NULL;
        }
        tinfo( "Connected (ish) to %s:%hu", t->host, t->port );
    }

    // and link them in a circle
    for( i = 0; i < cfg->tcount; i++ )
        targets[i].next = &(targets[(i + 1) % cfg->tcount]);

    return targets;
}




void *thread_loop( void *arg )
{
    unsigned char *buf;
    struct timeval tv;
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

    if( ( sk = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP ) ) < 0 )
    {
        terr( "Could not open source socket: %s", Err );
        free( ta );
        return NULL;
    }

    tv.tv_sec  = 3;
    tv.tv_usec = 0;

    if( setsockopt( sk, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof( struct timeval ) ) )
    {
        terr( "Could not set source socket timeout: %s", Err );
        free( ta );
        return NULL;
    }

    // bind our socket
    if( bind( sk, (struct sockaddr *) &(s->sa), sizeof( struct sockaddr_in ) ) )
    {
        terr( "Could not bind to source port %hu: %s", s->port, Err );
        exit( 1 );
    }
    tnotice( "Bound to port %hu.", ntohs( s->sa.sin_port ) );

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
        if( ( bc = recv( sk, buf, max, 0 ) ) < 0 )
        {
            if( errno == EINTR || errno == EAGAIN )
                continue;

            terr( "Recv error on source port %hu: %s", s->port, Err );
            break;
        }

        if( !bc )
            continue;

        // cap it
        buf[bc] = '\0';

        if( t )
        {
            if( ( sc = send( t->fd, buf, bc, 0 ) ) < 0 )
            {
                terr( "Send error to %s:%hu: %s", t->host, t->port, Err );
                break;
            }
            else if( sc < bc )
                twarn( "Only sent %ld out of %ld bytes to %s:%hu",
                        sc, bc, t->host, t->port );

            // and move t on
            t = t->next;
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



