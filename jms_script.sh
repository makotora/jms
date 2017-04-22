#!/bin/bash

path=""
cmnd=""
n=-1

#read arguments
while [ $# -gt 0 ]; do
	case $1 in
		-l)
			shift #$1 is now the arg after -l (<path>)
			if [ $# -gt 0 ]; then
				path=$1
			else
				echo "No path specified after -l"
				echo "Usage: ./jms_script.sh -l <path> -c <command>"
			fi
			shift #$1 is not the arg after <path>
			;;
		-c)
			shift #$1 is now the arg after -l (<path>)
			if [ $# -gt 0 ]; then
				cmnd=$1
				if [ "$cmnd" == size ]; then
					if [ $# -gt 1 ]; then
						if [ "$2" != "-l" ]; then
							shift
							n=$1
						fi
					fi
				fi
			else
				echo "No command specified after -c"
				echo "Usage: ./jms_script.sh -l <path> -c <command>"
			fi
			shift #$1 is not the arg after <path>
			;;
		*)
			echo "Invalid argument $1!"
			exit -1
			;;
	esac
done

#check if arguments were actually given
if [ -z $path ]; then
	echo "No path given"
	echo "Usage: ./jms_script.sh -l <path> -c <command>"
	exit -2	
fi

if [ -z $cmnd ]; then
	echo "No command given"
	echo "Usage: ./jms_script.sh -l <path> -c <command>"
	exit -2	
fi

#do what command wants
cd $path

if [ "$cmnd" == "list" ]; then
		ls -1
elif [ "$cmnd" == "size" ]; then
	if [ $n -eq -1 ]; then
		du -sb * | sort -h
	else
		du -sb * | sort -h | tail -f -n"$n"
	fi
elif [ "$cmnd" == "purge" ]; then
	rm -rf sdi1400201*
else
	echo "Unknown command '$cmnd'"
	exit -1
fi
