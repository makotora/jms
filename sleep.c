#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

int main(int argc, char* argv[])
{
	int duration = 5;
	int write_size;
	char* output;

	if (argc > 2)
	{
		duration = atoi(argv[1]);
		write_size = atoi(argv[2]) + 1;
	}
	else
	{
		srand(time(NULL));
		write_size = rand() % 100;
		if (argc == 2)
			duration = atoi(argv[1]);
	}

	output = malloc(write_size*sizeof(char));

	int i;
	for (i=0;i<write_size-1;i++)
		output[i] = 'a';

	output[write_size-1] = '\0';

	fprintf(stdout, "STDOUT OUTPUT OF PROCESS %d\n%s\n", getpid(), output);
	fprintf(stdout, "STDERR OUTPUT OF PROCESS %d\n", getpid());

	free(output);

	sleep(duration);

	return 0;
}