/**************************************************************************
* This code is licensed under the Apache License 2.0.  See ../LICENSE     *
* Copyright 2017 John Denholm                                             *
*                                                                         *
* main.c - main entry point                                               *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#include "juggler.h"


void usage( int exval )
{
    char *help_str = "\
Usage:\tjuggler -h\n\
\tjuggler [OPTIONS] -t <target> -s <source>\n\n\
Options:\n\
 -h                      Print this help\n\
 -s <source>             Source port to listen on.  Option may repeat.\n\
 -t <target>             Target [<ip>:]<port> to send to.  Option may repeat.\n\n\
 -m <max size>           Maximum size packet (default: 16384)\n\
Juggler will listen on all specified UDP ports and load-balance (round robin\n\
for now) packets across the specified targets.\n\n";

    printf( "%s", help_str );
    exit( exval );
}





void finish_loop( int sig )
{
    printf( "\nCaught stop signal.\n" );
    cfg->run = 0;
}



int set_signals( void )
{
    struct sigaction sa;

    memset( &sa, 0, sizeof( struct sigaction ) );
    sa.sa_handler = finish_loop;
    sa.sa_flags   = SA_NOMASK;

    // finish signal
    if( sigaction( SIGTERM, &sa, NULL )
     || sigaction( SIGQUIT, &sa, NULL )
     || sigaction( SIGINT,  &sa, NULL ) )
    {
        fprintf( stderr, "Could not set exit signal handlers -- %s", Err );
        return -1;
    }

    return 0;
}





// TODO - host lookup
int add_target( char *target )
{
    TRGT tmp, *t;
    char *pt;
    int l;

    memset( &tmp, 0, sizeof( TRGT ) );
    l = 0;

    if( ( pt = strchr( target, ':' ) ) )
    {
        l = pt - target;
        while( *pt && !isdigit( *pt ) )
            pt++;
    }
    else
        pt = target;

    if( !( tmp.port = (uint16_t) strtoul( pt, NULL, 10 ) ) )
    {
        fprintf( stderr, "Invalid target port specification '%s'\n", target );
        return -1;
    }

    if( l )
    {
        tmp.host = (char *) calloc( 1, l + 1 );
        memcpy( tmp.host, target, l );
    }
    else
        tmp.host = strdup( "127.0.0.1" );

    tmp.sa.sin_family = AF_INET;
    tmp.sa.sin_port   = htons( tmp.port );

    inet_aton( tmp.host, &(tmp.sa.sin_addr) );

    t = (TRGT *) calloc( 1, sizeof( TRGT ) );
    memcpy( t, &tmp, sizeof( TRGT ) );

    t->next = cfg->targets;
    cfg->targets = t;
    cfg->tcount++;

    printf( "Added target %s:%hu\n", t->host, t->port );

    return 0;
}

// TODO - add bind interface
int add_source( char *port )
{
    uint16_t p;
    SRC *s;

    if( !( p = (uint16_t) strtoul( port, NULL, 10 ) ) )
    {
        fprintf( stderr, "Invalid source port specification '%s'\n", port );
        return -1;
    }

    s = (SRC *) calloc( 1, sizeof( SRC ) );
    s->port = p;

    s->sa.sin_family        = AF_INET;
    s->sa.sin_port          = htons( s->port );
    s->sa.sin_addr.s_addr   = INADDR_ANY;

    s->next = cfg->sources;
    cfg->sources = s;
    cfg->scount++;

    return 0;
}


int main( int ac, char **av )
{
    SRC *s;
    int o;

    cfg = (CONF *) calloc( 1, sizeof( CONF ) );
    cfg->max = MAX_PACKET;

    while( ( o = getopt( ac, av, "hHm:t:s:" ) ) != -1 )
        switch( o )
        {
            case 'H':
            case 'h':
                usage( 0 );
                break;
            case 'm':
                cfg->max = atoi( optarg );
                break;
            case 's':
                if( add_source( optarg ) )
                    usage( 1 );
                break;
            case 't':
                if( add_target( optarg ) )
                    usage( 1 );
                break;
            default:
                fprintf( stderr, "Unrecognised option: %c\n\n", (char) o );
                usage( 1 );
                break;
        }

    if( !cfg->scount || !cfg->tcount )
    {
        fprintf( stderr, "Both sources and targets are needed.\n\n" );
        usage( 1 );
    }


    set_signals( );

    cfg->run = 1;

    for( s = cfg->sources; s; s = s->next )
        thread_throw( thread_loop, s );

    while( cfg->run )
        sleep( 1 );

    return 0;
}

