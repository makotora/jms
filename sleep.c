#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char* argv[])
{
	int duration = atoi(argv[1]);
	sleep(duration);
	fprintf(stdout, "STDOUT OUTPUT OF PROCESS %d\n", getpid());
	fprintf(stdout, "STDERR OUTPUT OF PROCESS %d\n", getpid());

	return 0;
}