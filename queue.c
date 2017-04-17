#include "queue.h"

queue* queue_create()
{
	queue * new_queue;
	new_queue = malloc(sizeof(queue));
	if ( new_queue == NULL ) 
		return NULL;

	new_queue->first = NULL;
	new_queue->last = NULL;

	return new_queue;
}


int queue_free(queue ** queue_ptr_ptr)
{
	queue * queue_ptr = *queue_ptr_ptr;
	message_node * current = queue_ptr->first;

	while (current != NULL)
	{
		message_node * next;
		next = current->next;

		free(current->message);
		free(current);
		current = next;
	}

	free(*queue_ptr_ptr);

	return 0;
}


int queue_push(queue * queue_ptr,char* message)
{
	message_node * new_message_node = malloc(sizeof(message_node));
	new_message_node->message = malloc( (strlen(message) + 1)*sizeof(char) );

	if ( new_message_node == NULL || new_message_node->message == NULL)
		return 10;

	strcpy(new_message_node->message, message);
	new_message_node->next = NULL;

	if ( queue_ptr->first == NULL )
	{
		queue_ptr->first = new_message_node;
		queue_ptr->last = new_message_node;
	}
	else
	{
		queue_ptr->last->next = new_message_node;
		queue_ptr->last = new_message_node;
	}

	return 0;
}


char* queue_pop(queue * queue_ptr)
{
	if ( queue_ptr->first == NULL)
		return NULL;
	else
	{
		message_node * popped_message_node = queue_ptr->first;
		char* message = popped_message_node->message;

		queue_ptr->first = queue_ptr->first->next;
		free(popped_message_node);

		return message;
	}
}


int queue_is_empty(queue* queue_ptr)
{ return queue_ptr->first == NULL; }