#include "queue.h"
#include <pthread.h>
#include <stdlib.h>

static request_t request_queue[QUEUE_SIZE];
static int start;
static int end;
static int size;

pthread_mutex_t queue_mutex;
pthread_cond_t queue_not_empty;
pthread_cond_t queue_not_full;

int queue_is_empty() {
    return size == 0;
}

int queue_is_full() {
    return size == QUEUE_SIZE;
}

void init_queue() {
    pthread_mutex_init(&queue_mutex, NULL);
    pthread_cond_init(&queue_not_empty, NULL);
    pthread_cond_init(&queue_not_full, NULL);
    start = 0;
    end = 0;
    size = 0;
}

void enqueue_request(int connfd, struct timeval arrival) {
    pthread_mutex_lock(&queue_mutex);
    while (queue_is_full()) {
        pthread_cond_wait(&queue_not_full, &queue_mutex); // thread goes to sleep if queue is full
    }

    request_queue[end].connfd = connfd;
    request_queue[end].arrival = arrival;
    end = (end + 1) % QUEUE_SIZE; // "selects" the new tail
    size++;

    pthread_cond_signal(&queue_not_empty); // signal that the queue is not empty anymore
    pthread_mutex_unlock(&queue_mutex);
}

request_t dequeue_request() {
    pthread_mutex_lock(&queue_mutex);
    while (queue_is_empty()) {
        pthread_cond_wait(&queue_not_empty, &queue_mutex); // thread goes to sleep if queue is empty
    }

    request_t req = request_queue[start];
    start = (start + 1) % QUEUE_SIZE; // "selects" the new head
    size--;

    pthread_cond_signal(&queue_not_full); // signal that the queue is not full anymore
    pthread_mutex_unlock(&queue_mutex);
    return req;
}




