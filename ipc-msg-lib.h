#ifndef __IPC_MSG_LIB_H__
#define __IPC_MSG_LIB_H__
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>
#include <string.h>
#define TEST_ERROR    if (errno) {fprintf(stderr, \
					  "%s:%d: PID=%5d: Error %d (%s)\n", \
					  __FILE__,			\
					  __LINE__,			\
					  getpid(),			\
					  errno,			\
					  strerror(errno));}

#define MSG_LEN 120      // 128 - sizeof(long)

struct msgbuf {
	long mtype;             /* message type, must be > 0 */
	char mtext[MSG_LEN];    /* message data */
};


/*
 * Return the message queue identifier obtained from the key written
 * in the file named "filename"
 *
 * - if the file does not exist, it is created and written with such a
 *   key
 *
 * - if the file exists, it is opened and the first integer is
 *   interpreted as key to get the ID of the message queue
 *
 * If error, some helping message is printed and -1 is returned.
 */
int msgget_from_key_in_file(char * filename);

#endif   // __IPC_MSG_LIB_H__
