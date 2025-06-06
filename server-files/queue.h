#ifndef QUEUE_H
#define QUEUE_H

#include <sys/time.h>

//#define QUEUE_SIZE 10

typedef struct {
    int connfd; //the assigned socket of the client
    struct timeval arrival; //the time of arrival
    struct timeval dispatch; //the time of dispatch
} request_t;

void init_queue();
void enqueue_request(int connfd, struct timeval arrival);
request_t dequeue_request();
int queue_is_empty();

#endif // QUEUE_H
