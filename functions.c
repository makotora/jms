#include "functions.h"

int write_and_read(char* message, int readfd, int writefd)
{
	//Write the message to the out pipe
	if (write(writefd, message, BUFSIZE) != BUFSIZE)
	{
		perror("send_message-write");
		return -1;
	}

	//Wait (by blocking) for receiver to (reply) write to the in pipe (his out pipe)
	if (read(readfd, message, BUFSIZE) <= 0)
	{
		fprintf(stderr, "Pipe has no writers.Cannot read!\n");
		return -3;
	}
	//Print reply
	// fprintf(stderr, "%s\n", message);

	return 0;
}


int intToString(int integer,char **string_ptr)
{
	int number_of_digits = 1;
	if (integer < 0) 
		integer*= -1;
	
	int tempint = integer;

	while (tempint > 9)
	{
		tempint /= 10;
		number_of_digits++;
	}
	
	*string_ptr = malloc( (number_of_digits+1)*sizeof(char));
	if (*string_ptr == NULL) return 10;

	sprintf(*string_ptr,"%d",integer);

	return 0;
}


void get_date_time_str(char* datestr, char* timestr)
{
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);
	char day[3];
	char month[3];
	char hour[3];
	char minute[3];
	char second[3];

	if (tm.tm_mon + 1 > 9)
		sprintf(month, "%d",tm.tm_mon + 1);
	else
		sprintf(month, "0%d",tm.tm_mon + 1);

	if (tm.tm_mday > 9)
		sprintf(day, "%d",tm.tm_mday);
	else
		sprintf(day, "0%d",tm.tm_mday);

	if (tm.tm_hour > 9)
		sprintf(hour, "%d",tm.tm_hour);
	else
		sprintf(hour, "0%d",tm.tm_hour);

	if (tm.tm_min > 9)
		sprintf(minute, "%d",tm.tm_min);
	else
		sprintf(minute, "0%d",tm.tm_min);

	if (tm.tm_sec > 9)
		sprintf(second, "%d",tm.tm_sec);
	else
		sprintf(second, "0%d",tm.tm_sec);

	sprintf(datestr, "%d%s%s", tm.tm_year + 1900, month, day);
	sprintf(timestr, "%s%s%s", hour, minute, second);
}