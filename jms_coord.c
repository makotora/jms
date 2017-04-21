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
#include "mystring.h"


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
	int finished_pools = 0;
	int job_counter = 0;
	int interrupted_jobs;
	int pool_pid, pool_status;
	int reply_code;
	int i, j, job_id, time_duration, q_time, counter, job_limit;
	char* command;
	char* token;
	pool_list* p_list;
	pool_node* pool;
	pool_info* p_info;
	mystring* mstring;

	p_list = pool_list_create();
	mstring = mystring_create(BUFSIZE);

	int running = 1;

	while (running)
	{
		//(1) CHECK IF ANY POOL HAS FINISHED
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
			finished_pools++;
			read(p_info->receive_fd, message, BUFSIZE);
			fprintf(stderr, "Coord: Pool %d (pid %d) has finished\n",p_info->id, p_info->pid);

			close(p_info->receive_fd);
			close(p_info->send_fd);
		}

		//(2) CHECK IF ANY OF THE POOLS JOBS HAS FINISHED (IF THEY SEND A "JOBDONE" message)
		pool = p_list->first;

		while (pool != NULL)//until we reach the end of the pool list
		{
			if (pool->info->status == 0) //If this pool isnt finished yet
			{
				//read all messages this pool has sent (if there are any)
				while (read(pool->info->receive_fd, message, BUFSIZE) > 0)
				{
					job_finished(pool->info, pool_max, message);//note that this job is finished
				}

				//If all of this pool's children are done.Note that this pool has nothing more to say
				if (pool->info->finished_count == pool_max)
				{
					pool->info->status = 1;
					strcpy(message, "bye");//send message to pool so it can finish
					write(pool->info->send_fd, message, BUFSIZE);
				}
			}

			pool = pool->next;
		}
		
		if (read(receive_fd, message, BUFSIZE) > 0)//If we have something to read from console
		{
			fprintf(stderr, "Coord: Console sent me: %s\n", message);
			command = malloc((strlen(message) + 1)*sizeof(char));
			strcpy(command, message);
			token = strtok(message, " \t\n");//get first word of message to see the type

			/*If its a submit command check if we need to create a new pool*/
			if (!strcmp(token, "submit"))
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
					p_info->job_count = 0;
					p_info->finished_count = 0;
					p_info->jobs = malloc(pool_max*sizeof(job_stats));

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
				p_info->job_count++;

				//Forward the message
				fprintf(stderr, "Coord: Forwarding to pool (%d) : %s\n",p_info->pid, command);
				write_and_read(command, reply, p_info->receive_fd, p_info->send_fd);

				//handle possible messages for job finishing from that specific pool (they start with !)
				while (  reply[0] == '!' )
				{
					job_finished(p_info, pool_max, reply);//note that this job is finished

					while( read(p_info->receive_fd, reply, BUFSIZE) == -1);//receive next message
				}

				//If all of this pool's jobs are done.Note that this pool has nothing more to say
				if (p_info->finished_count == pool_max)
				{
					p_info->status = 1;
					strcpy(message, "bye");//send message to pool so it can finish
					write(p_info->send_fd, message, BUFSIZE);
				}
				//reply now contains the actual reply to the command

				job_counter++;
				//Note info for this job
				i = (job_counter - 1) % pool_max;
				(p_info->jobs)[i].id = job_counter;
				(p_info->jobs)[i].status = 0;//Note that this job is running 
				(p_info->jobs)[i].start_time = time(NULL);
			}
			else if ( !strcmp(token, "status"))
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
					//if job is still runnning
					else if (p_info->jobs[i].status == 0)
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
				else
				{
					sprintf(reply, "No job with ID %d", job_id);
				}
			}
			else if ( !strcmp(token, "status-all"))
			{
				token = strtok(NULL, " \t\n");
				counter = 0;
				q_time = time(NULL);//note question time

				time_duration = -1;
				if (token != NULL)//if command includes time-duration
				{
					time_duration = atoi(token);
				}

				pool = p_list->first;

				//for all pools besides the last
				job_limit = pool_max;
				for (i=0;i<pool_counter-1;i++)
				{
					//for all jobs in this pool
					for (j=0;j<job_limit;j++)
					{
						//if time_duration doesnt matter (wasnt asked)
						//or it was submited at most 'time_duration' seconds ago
						if (time_duration == -1 ||
							q_time - pool->info->jobs[j].start_time <= time_duration)
						{
							counter++;
							job_id = pool->info->jobs[j].id;

							if (pool->info->jobs[j].status == -1)
							{
								sprintf(reply, "%d. JobID %d Status: Suspended\n", counter, job_id);
							}
							//if job is still runnning
							else if (pool->info->jobs[j].status == 0)
							{
								sprintf(reply, "%d. JobID %d Status: Active (running for %ld sec)\n",
								 counter, job_id, q_time - pool->info->jobs[j].start_time);
							}
							//if job is finished (its not suspended.we checked that above)
							else // status == 1
							{
								sprintf(reply, "%d. JobID %d Status: Finished\n", counter, job_id);	
							}

							mystring_add(mstring, reply);
						}
					}

					pool = pool->next;
				}
				//for the last pool (that may not be "full")
				if (job_counter % pool_max != 0)
				{
					job_limit = job_counter % pool_max;
				}

				for (j=0;j<job_limit;j++)
				{
					//if time_duration doesnt matter (wasnt asked)
					//or it was submited at most 'time_duration' seconds ago
					if (time_duration == -1 ||
							q_time - pool->info->jobs[j].start_time <= time_duration)
					{
						counter++;
						job_id = pool->info->jobs[j].id;

						if (pool->info->jobs[j].status == -1)
						{
							sprintf(reply, "%d. JobID %d Status: Suspended\n", counter, job_id);
						}
						//if job is still runnning
						else if (pool->info->jobs[j].status == 0)
						{
							sprintf(reply, "%d. JobID %d Status: Active (running for %ld sec)\n",
							 counter, job_id, q_time - pool->info->jobs[j].start_time);
						}
						//if job is finished (its not suspended.we checked that above)
						else // status == 1
						{
							sprintf(reply, "%d. JobID %d Status: Finished\n", counter, job_id);	
						}

						mystring_add(mstring, reply);
					}
				}
				if (mstring->size != 0)
				{
					strcpy(reply, mstring->string);//Copy the string we build to the final reply string
					mystring_clear(mstring);//clear the string that was built
				}
				else
				{//no results
					strcpy(reply, "Nothing to show.\n");
				}	
			}
			else if ( !strcmp(token, "show-active"))
			{
				mystring_add(mstring, "Active jobs:\n");
				counter = 0;
				pool = p_list->first;

				//for all pools besides the last
				job_limit = pool_max;
				for (i=0;i<pool_counter-1;i++)
				{
					//for all jobs in this pool
					for (j=0;j<job_limit;j++)
					{
						if (pool->info->jobs[j].status != 1)//if job is not finished.it is active
						{
							counter++;
							job_id = pool->info->jobs[j].id;
							sprintf(reply, "\t%d. JobID %d\n", counter, job_id);
							mystring_add(mstring, reply);
						}
					}

					pool = pool->next;
				}
				//for the last pool (that may not be "full")
				if (job_counter % pool_max != 0)
				{
					job_limit = job_counter % pool_max;
				}
				//(else) job_limit = pool_max (like before)

				for (j=0;j<job_limit;j++)
				{
					if (pool->info->jobs[j].status != 1)//if job is not finished.it is active
					{
						counter++;
						job_id = pool->info->jobs[j].id;
						sprintf(reply, "\t%d. JobID %d\n", counter, job_id);
						mystring_add(mstring, reply);
					}
				}

				if (counter == 0)
				{
					mystring_add(mstring, "\tNothing to show.\n");	
				}

				strcpy(reply, mstring->string);//Copy the string we build to the final reply string
				mystring_clear(mstring);//clear the string that was built
			}
			else if ( !strcmp(token, "show-pools"))
			{
				mystring_add(mstring, "PoolPid & NumberOfJobs:\n");
				counter = 0;
				pool = p_list->first;

				while (pool != NULL)
				{
					if (pool->info->status == 0)//if this pool is still running
					{
						counter++;
						sprintf(reply, "\t%d. %d %d\n", 
							counter, pool->info->pid, pool->info->job_count - pool->info->finished_count);
						mystring_add(mstring, reply);
					}

					pool = pool->next;
				}

				if (counter == 0)
				{
					mystring_add(mstring, "\tNothing to show.\n");	
				}

				strcpy(reply, mstring->string);//Copy the string we build to the final reply string
				mystring_clear(mstring);//clear the string that was built
			}
			else if ( !strcmp(token, "show-finished"))
			{
				mystring_add(mstring, "Finished jobs:\n");
				counter = 0;
				pool = p_list->first;

				//for all pools besides the last
				job_limit = pool_max;
				for (i=0;i<pool_counter-1;i++)
				{
					//for all jobs in this pool
					for (j=0;j<job_limit;j++)
					{
						if (pool->info->jobs[j].status == 1)//if job is finished
						{
							counter++;
							job_id = pool->info->jobs[j].id;
							sprintf(reply, "\t%d. JobID %d\n", counter, job_id);
							mystring_add(mstring, reply);
						}
					}

					pool = pool->next;
				}
				//for the last pool (that may not be "full")
				if (job_counter % pool_max != 0)
				{
					job_limit = job_counter % pool_max;
				}

				for (j=0;j<job_limit;j++)
				{
					if (pool->info->jobs[j].status == 1)//if job is finished
					{
						counter++;
						job_id = pool->info->jobs[j].id;
						sprintf(reply, "\t%d. JobID %d\n", counter, job_id);
						mystring_add(mstring, reply);
					}
				}

				if (counter == 0)
				{
					mystring_add(mstring, "\tNothing to show.\n");	
				}

				strcpy(reply, mstring->string);//Copy the string we build to the final reply string
				mystring_clear(mstring);//clear the string that was built
			}
			else if ( !strcmp(token, "suspend"))
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
						fprintf(stderr, "Coord: Forwarding to pool (%d) : %s\n",p_info->pid, command);
						write_and_read(command, reply, p_info->receive_fd, p_info->send_fd);

						//handle possible messages for job finishing from that specific pool (they start with !)
						while (  reply[0] == '!' )
						{
							job_finished(p_info, pool_max, reply);//note that this job is finished

							while( read(p_info->receive_fd, reply, BUFSIZE) == -1);//receive next message
						}

						//If all of this pool's jobs are done.Note that this pool has nothing more to say
						if (p_info->finished_count == pool_max)
						{
							p_info->status = 1;
							strcpy(message, "bye");//send message to pool so it can finish
							write(p_info->send_fd, message, BUFSIZE);
						}
						//reply now contains the actual reply to the command
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
			else if ( !strcmp(token, "resume"))
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
						//Forward the message
						fprintf(stderr, "Coord: Forwarding to pool (%d) : %s\n",p_info->pid, command);
						write_and_read(command, reply, p_info->receive_fd, p_info->send_fd);

						//handle possible messages for job finishing from that specific pool (they start with !)
						while (  reply[0] == '!' )
						{
							job_finished(p_info, pool_max, reply);//note that this job is finished

							while( read(p_info->receive_fd, reply, BUFSIZE) == -1);//receive next message
						}

						//If all of this pool's jobs are done.Note that this pool has nothing more to say
						if (p_info->finished_count == pool_max)
						{
							p_info->status = 1;
							strcpy(message, "bye");//send message to pool so it can finish
							write(p_info->send_fd, message, BUFSIZE);
						}
						//reply now contains the actual reply to the command
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
							sprintf(reply, "Cannot resume.Job %d didnt receive the signal!\n", job_id);
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
			else if ( !strcmp(token, "shutdown") )
			{
				running = 0;
				pool = p_list->first;

				//for all pools that are still active
				while (pool != NULL)
				{
					if (pool->info->status == 0)
					{
						fprintf(stderr, "Coord: Sending SIGTERM signal to pool %d (pid %d)\n",pool->info->id, pool->info->pid);
						kill(pool->info->pid, 15);//send a SIGTERM Signal to that pool
					}

					pool = pool->next;
				}

				//Now wait for the pools who we havent been noted as finished (didnt catch SIGCHLD)
				interrupted_jobs = 0;//receive from pools the number of jobs interrupted
				for (i = 0; i < (pool_counter - finished_pools); i++)
				{
					pool_pid = waitpid(-1, &pool_status, 0);//wait for any pool to finish
					p_info = pool_list_getby_pid(p_list, pool_pid);
					fprintf(stderr, "Coord: Pool %d (pid %d) has finished\n",p_info->id, p_info->pid);

					//receive this pool's messages we haven't read yet
					//it will return 0 when the pool closes the pipe
					while ( read(p_info->receive_fd, message, BUFSIZE) > 0)
					{
						//if its a message for a finished job
						if (message[0] == '!')
						{
							job_finished(p_info, pool_max, message);//note that this job is finished
						}
						else //its the pool's last message saying how many jobs were interrupted
						{//due to the shutdown
							counter = atoi(message);
							fprintf(stderr, "Coord: Pool %d told me that %d jobs were interrupted\n",p_info->id, counter);
							interrupted_jobs += counter;
						}
					}

					close(p_info->receive_fd);
					close(p_info->send_fd);
				}
				sprintf(reply, "Served %d jobs, %d were still in progress\n", job_counter, interrupted_jobs);

			}
			else
			{ 
				fprintf(stderr, "Coord: Console sent me something strange: %s\n", command);
				sprintf(reply, "Unknown/Unexpected command : %s\n", command);
			}

			//SEND MESSAGE/REPLY TO CONSOLE
			write(send_fd, reply, BUFSIZE);
			free(command);
		}
	}

	mystring_free(&mstring);
	pool_list_free(&p_list);

	close(receive_fd);
	close(send_fd);

	return 0;
}