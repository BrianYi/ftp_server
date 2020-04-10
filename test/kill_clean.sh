#!/bin/sh
sh kill.sh

for dump in $(ls | grep "server-dump")
do
	#read -p "Delete $dump?(y/n)" ans
	[ "$ans" != "n" ] && rm -r $dump
done
for dir in $(ls | egrep "[[:digit:]]{10,}")
do
	# avoid removing accendently
	#read -p "Delete $dir?(y/n)" ans
	[ "$ans" != "n" ] && rm -r $dir
done

#read -p "Delete all other single dump file?(y/n)" ans
#if [ "$ans" != "n" ]; then
#	rm '../ftpServer/bin/x64/Debug/rtmp_server.dump'
#fi
