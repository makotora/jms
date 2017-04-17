typedef struct message_node message_node;

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


struct message_node
{
	char* message;
	message_node * next;
};


struct queue
{
	message_node * first;
	message_node * last;
};

typedef struct queue queue;

queue* queue_create();
int queue_free(queue** queue_ptr_ptr);
int queue_push(queue* queue_ptr,char* message);
char* queue_pop(queue* queue_ptr);
int queue_is_empty(queue* queue_ptr);