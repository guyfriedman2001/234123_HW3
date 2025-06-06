#include "segel.h"
#include "request.h"
#include "log.h"
#include "pthread.h"
#include "queue.h"

#define THREAD_POOL_SIZE 4

typedef struct {
        int thread_id_in_array;
        server_log log; // Pointer to the server log
} thread_args;

struct Threads_stats threads_stats_array[THREAD_POOL_SIZE]; // Array to hold thread statistics for easier cleanup

//
// server.c: A very, very simple web server
//
// To run:
//  ./server <portnum (above 2000)>
//
// Repeatedly handles HTTP requests sent to this port number.
// Most of the work is done within routines written in request.c
//



// Parses command-line arguments
void getargs(int *port, int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(1);
    }
    *port = atoi(argv[1]);
}

/// Worker thread function that processes requests from the queue
void* worker_function(void* arg) {
    thread_args* args = (thread_args*)arg; 
    int id_in_array = args->thread_id_in_array;
    server_log serverLog = args->log; 
    threads_stats t = &threads_stats_array[id_in_array];

    while (1) {
        request_t req = dequeue_request(); 

        struct timeval dispatch;
        gettimeofday(&dispatch, NULL);
        requestHandle(req.connfd, req.arrival, dispatch, t, serverLog); // Handle the request
        Close(req.connfd); 
    }
    return NULL;
}

void init_threads_stats_array(){
    for (int i = 0; i < THREAD_POOL_SIZE; i++) {
        threads_stats_array[i].id = 0; 
        threads_stats_array[i].stat_req = 0;
        threads_stats_array[i].dynm_req = 0;
        threads_stats_array[i].post_req = 0;
        threads_stats_array[i].total_req = 0;
    }
}


// TODO: HW3 — Initialize thread pool and request queue
// This server currently handles all requests in the main thread.
// You must implement a thread pool (fixed number of worker threads)
// that process requests from a synchronized queue.

// === Master Thread Logic ===
int main(int argc, char *argv[])
{
    // Create the global server log
    server_log serverLog = create_log();

    int listenfd, connfd, port, clientlen;
    struct sockaddr_in clientaddr;

    getargs(&port, argc, argv);
    init_queue(); // Initialize the request queue
    init_threads_stats_array(); // Initialize the thread statistics array



    pthread_t* thread_pool = malloc(sizeof(pthread_t) * THREAD_POOL_SIZE);
    thread_args* thread_args_array[THREAD_POOL_SIZE];
    for (int i = 0; i < THREAD_POOL_SIZE; i++)
    {
        thread_args_array[i] = malloc(sizeof(thread_args));
        thread_args_array[i]->thread_id_in_array = i; // Assign thread ID in the array
        thread_args_array[i]->log = serverLog; // Assign the server log to the thread args

        pthread_create(&thread_pool[i], NULL, worker_function, thread_args_array[i]);
    }
    

    listenfd = Open_listenfd(port);
    while (1) {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);

        struct timeval arrival,dispatch;
        gettimeofday(&arrival, NULL); // Get the current time as the request arrival time
        enqueue_request(connfd, arrival); // Enqueue the request with its arrival time
        // TODO: HW3 — Record the request arrival time here

        // DEMO PURPOSE ONLY:
        // This is a dummy request handler that immediately processes
        // the request in the main thread without concurrency.
        // Replace this with logic to enqueue the connection and let
        // a worker thread process it from the queue.

        /*threads_stats t = malloc(sizeof(struct Threads_stats));
        t->id = 0;             // Thread ID (placeholder)
        t->stat_req = 0;       // Static request count
        t->dynm_req = 0;       // Dynamic request count
        t->total_req = 0;      // Total request count

        struct timeval arrival, dispatch;
        arrival.tv_sec = 0; arrival.tv_usec = 0;   // DEMO: dummy timestamps
        dispatch.tv_sec = 0; dispatch.tv_usec = 0; // DEMO: dummy timestamps

        // Call the request handler (immediate in main thread — DEMO ONLY)
        requestHandle(connfd, arrival, dispatch, t, log);

        free(t); // Cleanup
        Close(connfd); // Close the connection*/
    }
    for (int i = 0; i < THREAD_POOL_SIZE; i++)
    {
        pthread_join(thread_pool[i], NULL);
        free(thread_args_array[i]); 
    }
    
    // Clean up the server log before exiting
    destroy_log(serverLog);

    // TODO: HW3 — Add cleanup code for thread pool and queue
}

