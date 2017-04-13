#ifndef POOL_LIST_H
#define POOL_LIST_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>


typedef struct job_info job_info;
typedef struct job_node job_node;
typedef struct job_list job_list;

typedef struct pool_info pool_info;
typedef struct pool_node pool_node;
typedef struct pool_list pool_list;

struct job_info
{
	int id;
	int pid;
	int status;/*-1: suspended, 0 running, 1 finished*/
	int start_time;
};

struct job_node
{
	job_info* info;
	job_node* next;
};

struct job_list
{
	job_node* first;
	job_node* last;
};

job_list* job_list_create();
void job_list_free(job_list** list_ptr_ptr);
void job_list_add(job_list* list_ptr, job_info* info);
job_info* job_list_getby_id(job_list* list_ptr, int id);
job_info* job_list_getby_pid(job_list* list_ptr, int pid);
void job_info_print(job_info info);
void job_list_print(job_list* list_ptr);

struct pool_info
{
	int id;
	int pid;
	int status;/*0 running,1 finished*/
	int send_fd;
	int receive_fd;
	job_info* jobs;
};

struct pool_node
{
	pool_info* info;
	pool_node* next;
};

struct pool_list
{
	pool_node* first;
	pool_node* last;
};

pool_list* pool_list_create();
void pool_list_free(pool_list** list_ptr_ptr);
void pool_list_add(pool_list* list_ptr, pool_info* info);
pool_info* pool_list_getby_id(pool_list* list_ptr, int id);
pool_info* pool_list_getby_pid(pool_list* list_ptr, int pid);
void pool_info_print(pool_info info);
void pool_list_print(pool_list* list_ptr);

#endif