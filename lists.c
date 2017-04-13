#include "lists.h"


//JOB LIST
job_list* job_list_create()
{
	job_list* new_list = malloc(sizeof(job_list));

	if (new_list == NULL)
	{
		fprintf(stderr,"Malloc error in list_create!");
		exit(-1);
	}

	new_list->first = NULL;
	new_list->last = NULL;

	return new_list;
}

void job_list_free(job_list** list_ptr_ptr)
{
	job_node* current = (*list_ptr_ptr)->first;
	job_node* target;

	while (current != NULL)
	{
		target = current;
		current = current->next;
		free(target->info);
		free(target);
	}

	free(*list_ptr_ptr);
}


void job_list_add(job_list* list_ptr, job_info* info)
{
	job_node* new_node = malloc(sizeof(job_node));

	if (new_node == NULL)
	{
		fprintf(stderr, "Malloc error in list_add!");
		exit(-1);
	}

	new_node->info = info;
	new_node->next = NULL;

	if (list_ptr->first == NULL)
	{
		list_ptr->first = new_node;
		list_ptr->last = new_node;
	}
	else
	{
		list_ptr->last->next = new_node;
		list_ptr->last = new_node;
	}
}


job_info* job_list_getid(job_list* list_ptr, int id)
{
	job_node* current = list_ptr->first;

	int i;

	for (i=0;i<id;i++)
	{
		current = current->next;
	}

	return current->info;
}

job_info* job_list_getpid(job_list* list_ptr, int pid)
{
	job_node* current = list_ptr->first;

	while (current != NULL)
	{
		if (current->info->pid == pid)
			return current->info;

		current = current->next;
	}
	

	return NULL;//no job with this pid
}

void job_info_print(job_info info)
{
	fprintf(stderr, "> JOB %d <\n",info.id);
	fprintf(stderr, "PID: %d\n",info.pid);
	fprintf(stderr, "STATUS: %d\n",info.status);
	fprintf(stderr, "Start time: %d\n",info.start_time);
}


void job_list_print(job_list* list_ptr)
{
	job_node* current = list_ptr->first;

	while (current != NULL)
	{
		job_info_print(*(current->info));
		current = current->next;
	}
}

//POOL LIST
pool_list* pool_list_create()
{
	pool_list* new_list = malloc(sizeof(pool_list));

	if (new_list == NULL)
	{
		fprintf(stderr, "Malloc error in list_create!");
		exit(-1);
	}

	new_list->first = NULL;
	new_list->last = NULL;

	return new_list;
}

void pool_list_free(pool_list** list_ptr_ptr)
{
	pool_node* current = (*list_ptr_ptr)->first;
	pool_node* target;

	while (current != NULL)
	{
		target = current;
		current = current->next;
		free(target->info);
		free(target);
	}

	free(*list_ptr_ptr);
}


void pool_list_add(pool_list* list_ptr, pool_info* info)
{
	pool_node* new_node = malloc(sizeof(pool_node));

	if (new_node == NULL)
	{
		fprintf(stderr, "Malloc error in list_add!");
		exit(-1);
	}

	new_node->info = info;
	new_node->next = NULL;

	if (list_ptr->first == NULL)
	{
		list_ptr->first = new_node;
		list_ptr->last = new_node;
	}
	else
	{
		list_ptr->last->next = new_node;
		list_ptr->last = new_node;
	}
}


pool_info* pool_list_getid(pool_list* list_ptr, int id)
{
	pool_node* current = list_ptr->first;

	int i;

	for (i=0;i<id;i++)
	{
		current = current->next;
	}

	return current->info;
}

pool_info* pool_list_getpid(pool_list* list_ptr, int pid)
{
	pool_node* current = list_ptr->first;

	while (current != NULL)
	{
		if (current->info->pid == pid)
			return current->info;

		current = current->next;
	}
	

	return NULL;//no job with this pid
}

void pool_info_print(pool_info info)
{
	fprintf(stderr, "> POOL %d <\n",info.id);
	fprintf(stderr, "PID: %d\n",info.pid);
	fprintf(stderr, "STATUS: %d\n",info.status);

}


void pool_list_print(pool_list* list_ptr)
{
	pool_node* current = list_ptr->first;

	while (current != NULL)
	{
		pool_info_print(*(current->info));
		current = current->next;
	}
}