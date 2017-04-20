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
	int pid, console_pid;
	char message[BUFSIZE];
	char reply[BUFSIZE];
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
	int reply_code;
	int finished_job_id, i, job_id;
	char* command;
	char* command_type;
	pool_list* p_list;
	pool_node* pool;
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
			read(p_info->receive_fd, message, BUFSIZE);
			fprintf(stderr, "Pool %d (%d) finished\n",p_info->id, p_info->pid);

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

					//Start making a new node in the list for this pool
					p_info = malloc(sizeof(pool_info));
					p_info->id = pool_counter;
					p_info->status = 0;
					p_info->active_count = 0;
					p_info->jobs = malloc(pool_max*sizeof(job_stats));

					//Open pipes (block until pool opens them as well)
					if ( (p_info->receive_fd = open(pool_pipe_in, O_RDONLY)) < 0)
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
				p_info->active_count++;

				//Forward the message
				fprintf(stderr, "Coord: Forwarding to pool (%d) : %s\n",p_info->pid, command);
				write_and_read(command, reply, p_info->receive_fd, p_info->send_fd);

				job_counter++;
				//Note info for this job
				i = job_counter % pool_max;
				(p_info->jobs)[i].id = job_counter;
				(p_info->jobs)[i].status = 0;//Note that this job is running 
				(p_info->jobs)[i].start_time = time(NULL);
			}
			else if ( !strcmp(command_type, "suspend"))
			{
				job_id = atoi(strtok(NULL, " \t\n"));

				if (job_id > 0 && job_id <= job_counter)
				{
					//get pool_info of the pool responsible for this job_id
					i = (job_id-1) / pool_max;
					p_info = pool_list_getby_id(p_list, i+1);//+1 because counting starts from 1 for ids

					i = (job_id-1) % pool_max;//index for this job in this pools job array

					//if job is still runnning
					if (p_info->jobs[i].status == 0)
					{
						//forward suspend message to that pool
						write_and_read(command, reply, p_info->receive_fd, p_info->send_fd);
						//pool will send back a reply for the command
						//0 means OK
						//-1 means FAIL.job is already done.
						/*
						To explain:I DONT request info from pools about which jobs are finished
						for commands suspend,resume shutdown.I just receive reply.
						Also there is a chance that a job finishes right before suspending it
						so I let the pool do the checking..
						*/
						reply_code = atoi(reply);

						if (reply_code == 0)//ok
						{
							p_info->jobs[i].status = -1;//not that this job is now suspended
							sprintf(reply, "Sent suspend signal to job %d!\n", job_id);
						}
						else//reply_code == -1 , meaning that job is finished but coord didnt know yet
						{
							p_info->jobs[i].status = 1;//not that this job is now finished
							sprintf(reply, "Cannot suspend.Job %d has finished!\n", job_id);
						}
					}
					else if (p_info->jobs[i].status == -1)//if its suspended
					{
						sprintf(reply, "Cannot suspend.Job %d is already suspended!\n", job_id);
					}
					else //status == 1 (finished)
					{
						sprintf(reply, "Cannot suspend.Job %d has finished!\n", job_id);	
					}
				}
				else
				{
					sprintf(reply, "Cannot suspend.No job with ID %d", job_id);
				}
			}
			else if ( !strcmp(command_type, "resume"))
			{
				job_id = atoi(strtok(NULL, " \t\n"));

				if (job_id > 0 && job_id <= job_counter)
				{
					//get pool_info of the pool responsible for this job_id
					i = (job_id-1) / pool_max;
					p_info = pool_list_getby_id(p_list, i+1);//+1 because counting starts from 1 for ids

					i = (job_id-1) % pool_max;//index for this job in this pools job array

					//Note that if a pool is finished <=> every job (of that pool) has finished
					if (p_info->jobs[i].status == -1)//if its suspended
					{
						//forward suspend message to that pool
						write_and_read(command, reply, p_info->receive_fd, p_info->send_fd);
						//pool will send back a reply for the command
						//0 means OK
						//-1 means Fail.job is already done.

						reply_code = atoi(reply);

						if (reply_code == 0)//ok
						{
							p_info->jobs[i].status = 0;//not that this job is now running
							sprintf(reply, "Sent resume signal to job %d!\n", job_id);
						}
						else//reply_code == -1 , meaning that job is finished but coord didnt know yet
						{
							p_info->jobs[i].status = 1;//not that this job is now finished
							sprintf(reply, "Cannot resume.Job %d has finished!\n", job_id);
						}
					}
					else if (p_info->jobs[i].status == 0)//if its running
					{
						sprintf(reply, "Cannot resume.Job %d is already running!\n", job_id);
					}
					else //status == 1 (finished)
					{
						sprintf(reply, "Cannot resume.Job %d has finished!\n", job_id);	
					}
				}
				else
				{
					sprintf(reply, "Cannot suspend.No job with ID %d", job_id);
				}
			}
			else if ( !strcmp(command_type, "status"))
			{
				job_id = atoi(strtok(NULL, " \t\n"));

				if (job_id > 0 && job_id <= job_counter)
				{
					//get pool_info of the pool responsible for this job_id
					i = (job_id-1) / pool_max;
					p_info = pool_list_getby_id(p_list, i+1);//+1 because counting starts from 1 for ids

					i = (job_id-1) % pool_max;//index for this job in this pools job array

					//if job is suspended
					if (p_info->jobs[i].status == -1)
					{
						sprintf(reply, "JobID %d Status: Suspended\n", job_id);
					}
					else //job is not suspended.So its either running or finished.We dont know yet
					{//Ask its pool for info
						if (p_info->status == 0) //If this pool isnt finished yet
						{
							strcpy(message, "info");//request info/update from that pool
							write(p_info->send_fd, message, BUFSIZE);
							
							read(p_info->receive_fd, reply, BUFSIZE);
							finished_job_id = atoi(reply);

							while (finished_job_id != -1)//until pool sends -1 (for end of communication)
							{
								fprintf(stderr, "Coord: Pool %d told me that job %d is done\n", p_info->id , finished_job_id);
								i = (finished_job_id - 1) % pool_max;
								((p_info->jobs)[i]).status = 1;//Note that this job (of this pull) is done
								read(p_info->receive_fd, reply, BUFSIZE);
								finished_job_id = atoi(reply);
							}
						}
						//NOW THAT WE GOT INFO/UPDATE FROM THIS POOL.CHECK THE JOB'S STATUS

						//if job is still runnning
						if (p_info->jobs[i].status == 0)
						{
							sprintf(reply, "JobID %d Status: Active (running for %ld sec)\n",
							 job_id, time(NULL) - p_info->jobs[i].start_time);
						}
						//if job is finished (its not suspended.we checked that above)
						else // status == 1
						{
							sprintf(reply, "JobID %d Status: Finished\n", job_id);	
						}

					}

				}
				else
				{
					sprintf(reply, "No job with ID %d", job_id);
				}
			}
			else if ( !strcmp(command_type, "shutdown") )
			{
				running = 0;
			}
			else
			{ 
			//If its not a submit,suspend,resume,status,shutdown command
			//It will be a command that needs info from all pools
			//first send a message to each pool to request info/update on the jobs status
				pool = p_list->first;

				while (pool != NULL)//until we reach the end of the pool list
				{
					if (pool->info->status == 0) //If this pool isnt finished yet
					{
						strcpy(message, "info");//request info
						write(pool->info->send_fd, message, BUFSIZE);
						
						read(pool->info->receive_fd, reply, BUFSIZE);
						finished_job_id = atoi(reply);

						while (finished_job_id != -1)//until pool sends -1 (for end of communication)
						{
							fprintf(stderr, "Coord: Pool %d told me that job %d is done\n", pool->info->id, finished_job_id);
							pool->info->active_count--;
							i = (finished_job_id - 1) % pool_max;
							((pool->info->jobs)[i]).status = 1;//Note that this job (of this pull) is done

							read(pool->info->receive_fd, reply, BUFSIZE);
							finished_job_id = atoi(reply);
						}
					}

					pool = pool->next;
				}
			}

			//SEND MESSAGE/REPLY TO CONSOLE
			write(send_fd, reply, BUFSIZE);
			free(command);
		}
	}

	pool_list_free(&p_list);

	close(receive_fd);
	close(send_fd);

	return 0;
}