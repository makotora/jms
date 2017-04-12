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

#include "protocol.h"
#include "lists.h"

int test_lists();

int main()
{
	test_lists();

	return 0;
}

int test_lists()
{
	job_info jinfo;
	job_list* jlist = job_list_create();
	jinfo.id = 1;
	jinfo.pid = 123;
	jinfo.status = 0;
	jinfo.start_time = time(NULL);
	job_list_add(jlist, jinfo);
	jinfo.id = 2;
	jinfo.pid = 1234;
	jinfo.status = 2;
	jinfo.start_time = time(NULL);
	job_list_add(jlist, jinfo);

	job_list_print(jlist);
	job_info_print(job_list_get(jlist,1));
	job_info_print(job_list_get(jlist,0));

	job_list_free(&jlist);

	pool_info pinfo;
	pool_list* plist = pool_list_create();
	pinfo.id = 1;
	pinfo.pid = 123;
	pinfo.status = 0;
	pool_list_add(plist, pinfo);
	pinfo.id = 2;
	pinfo.pid = 1234;
	pinfo.status = 2;
	pool_list_add(plist, pinfo);

	pool_list_print(plist);
	pool_info_print(pool_list_get(plist,1));
	pool_info_print(pool_list_get(plist,0));

	pool_list_free(&plist);

	return 0;
}