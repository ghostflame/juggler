#!/bin/bash

PORT=12100
MESG="Test message"

function usage( )
{
    cat <<- EOF
Usage:  source.sh [-m <msg>] [-p <port>]

Format of output is:
"[%08d] %s\n", counter, <msg>

EOF
    exit 0
}

while getopts "Hm:p:" o; do
    case $o in
        H)  usage
            ;;
        m)  MESG="$OPTARG"
            ;;
        p)  PORT=$OPTARG
            ;;
    esac
done

# ensure we end in just one .
MESG=${MESG%%.}
MESG="$MESG."

( i=0
  while [ 1 ]; do
    printf "[%08d] %s\n" $i "$MESG"
    ((i++))
  done ) | nc -w 1 -u 127.0.0.1 $PORT

