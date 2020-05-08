#!/bin/bash
[ ! -e "gen" ] && (gcc -o gen gen.c)

if [ $# -ne 3 ]
then
    echo "usage: gen.sh FILE N_ITEMS VALUE"
    exit 1
fi

case "$3" in
    random) head -c $((4 * $2)) /dev/urandom > "$1";;
    *)      ./gen "$1" "$2" "$3";;
esac
