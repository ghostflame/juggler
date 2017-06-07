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
 -l <level>              Specify a log level (err, warn, notice, info, debug)\n\
 -d <sec>                Specify a reconnect delay in seconds\n\n\
Juggler will listen on all specified UDP ports and load-balance (round robin\n\
for now) packets across the specified targets.\n\n";

    printf( "%s", help_str );
    exit( exval );
}



size_t logger( int level, int id, char *fmt, ... )
{
    struct timeval tv;
    char buf[4096];
    struct tm tm;
    va_list args;
    size_t s = 0;

    if( level > cfg->level )
        return 0;

    // get the time
    gettimeofday( &tv, NULL );
    localtime_r( &(tv.tv_sec), &tm );

    // write a timestamp
    s += strftime( buf + s, 4095 - s, "[%F %T", &tm );
    s += snprintf( buf + s, 4095 - s, ".%06ld ", tv.tv_usec );
    s += strftime( buf + s, 4095 - s, "%Z] ", &tm );

    // add the log line
    va_start( args, fmt );
    s += vsnprintf( buf + s, 4095 - s, fmt, args );
    va_end( args );

    // and a newline
    buf[s++] = '\n';

    // and write
    return fwrite( buf, s, 1, cfg->logto );
}


void set_log_level( char *level )
{
    if( !level )
    {
        cfg->level = LOG_NOTICE;
        return;
    }

    if( !strcasecmp( level, "debug" ) )
        cfg->level = LOG_DEBUG;
    else if( !strcasecmp( level, "info" ) )
        cfg->level = LOG_INFO;
    else if( !strcasecmp( level, "notice" ) )
        cfg->level = LOG_NOTICE;
    else if( !strcasecmp( level, "warn" ) )
        cfg->level = LOG_WARN;
    else if( !strcasecmp( level, "error" ) )
        cfg->level = LOG_ERROR;
    else
    {
        err( "Unrecognised log level: %s", level );
        exit( 1 );
    }
}



void finish_loop( int sig )
{
    info( "Caught stop signal." );
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
        err( "Could not set exit signal handlers -- %s", Err );
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
        err( "Invalid target port specification '%s'", target );
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

    return 0;
}


// TODO - add bind interface
int add_source( char *port )
{
    uint16_t p;
    SRC *s;

    if( !( p = (uint16_t) strtoul( port, NULL, 10 ) ) )
    {
        err( "Invalid source port specification '%s'", port );
        return -1;
    }

    s = (SRC *) calloc( 1, sizeof( SRC ) );
    s->port = p;

    // can't set the buffer yet, we might not have read a max argument

    s->sa.sin_family        = AF_INET;
    s->sa.sin_port          = htons( s->port );
    s->sa.sin_addr.s_addr   = INADDR_ANY;

    s->next = cfg->sources;
    cfg->sources = s;
    cfg->scount++;

    return 0;
}


void reverse_lists( void )
{
    TRGT *t, *tlist;
    SRC *s, *slist;

    tlist = cfg->targets;
    cfg->targets = NULL;

    while( tlist )
    {
        t = tlist;
        tlist = t->next;

        t->next = cfg->targets;
        cfg->targets = t;

        info( "Added target %s:%hu.", t->host, t->port );
    }


    slist = cfg->sources;
    cfg->sources = NULL;

    while( slist )
    {
        s = slist;
        slist = s->next;

        s->next = cfg->sources;
        cfg->sources = s;
    }
}




int main( int ac, char **av )
{
    int i, o;
    SRC *s;

    cfg = (CONF *) calloc( 1, sizeof( CONF ) );
    cfg->max = MAX_PACKET;
    cfg->level = LOG_NOTICE;
    cfg->delay = RECONN_DELAY;
    cfg->logto = stdout;

    while( ( o = getopt( ac, av, "hHm:t:s:l:d:" ) ) != -1 )
        switch( o )
        {
            case 'H':
            case 'h':
                usage( 0 );
                break;
            case 'l':
                set_log_level( optarg );
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
            case 'd':
                cfg->delay = atoi( optarg );
                break;
            default:
                err( "Unrecognised option: %c\n", (char) o );
                usage( 1 );
                break;
        }

    // log to stderr if we are writing to stdout
    if( !cfg->targets )
        cfg->logto = stderr;

    if( !cfg->scount )
    {
        err( "No sources defined.\n" );
        usage( 1 );
    }

    set_signals( );
    reverse_lists( );

    cfg->run = 1;

    for( i = 0, s = cfg->sources; s; s = s->next, i++ )
        thread_throw( thread_loop, s, i );

    while( cfg->run )
    {
        cfg->ts = time( NULL );
        usleep( 25000 );
    }

    return 0;
}

