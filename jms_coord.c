#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <fcntl.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#define BUFSIZE 1024
#define PERMS 0666

#include "functions.h"
#include "lists.h"


int main(int argc, char* argv[])
{
	char * path = NULL;
	char* max_str;
	int pool_max = -1;
	char * jms_in = NULL;
	char * jms_out = NULL;
	int receive_fd, send_fd;
	int status;
	int pid, console_pid;
	char message[1024];
	char* job_count_str;
	char pool_pipe_in[100]; 
	char pool_pipe_out[100]; 

	//Read Arguments
	//..

	path = "temp";
	max_str = "3";
	jms_in = "jmsin";
	jms_out = "jmsout";

	pool_max = atoi(max_str);
	mkdir(path, 0777);

	//Create pipes for communication with console
	if ( (mkfifo(jms_in, PERMS) < 0) && (errno != EEXIST) )
	{
		perror("Cannot create fifo");
	}

	if ( (mkfifo(jms_out, PERMS) < 0) && (errno != EEXIST) )
	{
		perror("Cannot create fifo");
	}

	fprintf(stderr, "Coord: Created pipes.About to try opening them\n");
	
	//Open pipes
	if ( (receive_fd = open(jms_in, O_RDONLY | O_NONBLOCK)) < 0)
	{
		perror("Cannot open jms_in pipe");
	}

	if ( (send_fd = open(jms_out, O_WRONLY)) < 0)
	{
		perror("Cannot open jms_out pipe");
	}

	fprintf(stderr, "Coord: Opened pipes!\n");

	pid = getpid();
	
	//"Handshake" with console
	read(receive_fd, message, BUFSIZE);//This wont fail because console first writes to jms_in and 
	//after that opens jms_out for reading (unblocking coord)
	console_pid = atoi(message);
	fprintf(stderr, "Coord: Received console's pid (%d)\n", console_pid);

	fprintf(stderr, "Coord: My pid is %d.Sending it to console\n", pid);
	sprintf(message, "%d", pid);
	write(send_fd, message, BUFSIZE);//Send my pid to console so he can signal me for messages

	

	//HANDLE INCOMING MESSAGES
	int pool_counter = 0;
	int job_counter = 0;
	int pool_pid, pool_status;
	char* command;
	char* command_type;
	pool_list* p_list;
	pool_info* p_info;
	p_list = pool_list_create();

	int running = 1;

	while (running)
	{
		pool_pid = waitpid(-1, &pool_status, WNOHANG);/*Check if any pool has finished*/

		if (pool_pid > 0)
		{
			p_info = pool_list_getby_pid(p_list, pool_pid);//get the finished pipe's info
			if (p_info == NULL)
			{
				fprintf(stderr, "Fatal error.Cannot find pool node for pool: %d\n", pool_pid);
				exit(-2);
			}

			p_info->status = 1;//note that it finished
			read(p_info->receive_fd, message, BUFSIZE);//Read his lasts words
			fprintf(stderr, "Pool %d (%d) finished: %s\n",p_info->id, p_info->pid, message);
//TO DO:LAST WORDS MUST CONTAIN EVERY INFO WE NEED FOR THAT POOLS JOBS
//PUT THAT IN AN ARRAY

			close(p_info->receive_fd);
			close(p_info->send_fd);
		}
		
		if (read(receive_fd, message, BUFSIZE) > -1)//If we have something to read from console
		{
			fprintf(stderr, "Coord: Console sent me: %s\n", message);
			command = malloc((strlen(message) + 1)*sizeof(char));
			strcpy(command, message);
			command_type = strtok(message, " \t\n");//get first word of message to see the type

			/*If its a submit command check if we need to create a new pool*/
			if (!strcmp(command_type, "submit"))
			{
				if (job_counter % pool_max == 0)
				{
					pool_counter++;

					sprintf(pool_pipe_in, "pipe%d_in", pool_counter);
					sprintf(pool_pipe_out, "pipe%d_out", pool_counter);

					//Start making a new node in the list for this pool
					p_info = malloc(sizeof(pool_info));
					p_info->id = pool_counter;
					p_info->status = 0;
					p_info->jobs = NULL;

					//Make pipes for communication with pool
					if ( (mkfifo(pool_pipe_in, PERMS) < 0) && (errno != EEXIST) )
					{
						perror("Cannot create fifo");
					}

					if ( (mkfifo(pool_pipe_out, PERMS) < 0) && (errno != EEXIST) )
					{
						perror("Cannot create fifo");
					}

					//START POOL
					if ( intToString(job_counter,&job_count_str) != 0)
						exit(-2); 

					if ( (pool_pid = fork()) == 0)
					{
						//close pipes for comm with console (pool doesnt need that)
						close(receive_fd);
						close(send_fd);
						
						execl("./pool","./pool", path, pool_pipe_in, pool_pipe_out, job_count_str, max_str, NULL);
					}

					free(job_count_str);

					//Open pipes (block until pool opens them as well)
					if ( (p_info->receive_fd = open(pool_pipe_in, O_RDONLY | O_NONBLOCK)) < 0)
					{
						perror("Cannot open jms_in pipe");
					}

					if ( (p_info->send_fd = open(pool_pipe_out, O_WRONLY)) < 0)
					{
						perror("Cannot open jms_out pipe");
					}
					
					//"Handshake" with pool
					read(p_info->receive_fd, message, BUFSIZE);//again like console handshake.this wont fail
					p_info->pid = atoi(message);
					fprintf(stderr, "Coord: Received pool %d pid (%d)\n",pool_counter, p_info->pid);


					fprintf(stderr, "Coord: My pid is %d.Sending it to pool %d\n", pid, pool_counter);
					sprintf(message, "%d", pid);
					write(p_info->send_fd, message, BUFSIZE);

					//Pool creation over.Add pool_info to pool_list
					pool_list_add(p_list, p_info);
				}

				//The pool to handle this submit is there (maybe just created).Forward the submit command
				p_info = pool_list_get_last(p_list);//get info for the correct pool (the last there is)

				if (p_info->status == 0)//If it is still running
				{//Forward the message
					fprintf(stderr, "Coord: Forwarding to pool (%d) : %s\n",p_info->pid, command);
					write(p_info->send_fd, command, BUFSIZE);
				}
				else
				{
					fprintf(stderr, "Coord: This pool has finished!Need to access its last words array\n");
				}
				
				job_counter++;
			}
			else if ( !strcmp(command_type, "shutdown") )
			{
				running = 0;
			}

			//SEND MESSAGE TO CORRECT POOL AND WAIT FOR REPLY
			strcpy(message, "OK\n");
			write(send_fd, message, BUFSIZE);
			free(command);
		}
	}

	pool_list_free(&p_list);

	close(receive_fd);
	close(send_fd);

	return 0;
}