#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "log.h"

struct LogNode {
    char* data;               
    int data_length;             
    struct LogNode* next;     
};



// Opaque struct definition
struct Server_Log {
    struct LogNode* head;
    struct LogNode* tail;

    int readers_inside;
    int writers_inside;
    int writers_waiting;

    pthread_cond_t read_allowed;
    pthread_cond_t write_allowed;
    pthread_mutex_t global_lock;
};

void init_lock(server_log log) {
    log->head = NULL;
    log->tail = log->head;
    log->readers_inside = 0;
    log->writers_inside = 0;
    log->writers_waiting = 0;

    pthread_cond_init(&log->read_allowed, NULL);
    pthread_cond_init(&log->write_allowed, NULL);
    pthread_mutex_init(&log->global_lock, NULL);
}

void reader_lock(server_log log) {
    pthread_mutex_lock(&log->global_lock);
    while (log->writers_inside > 0 || log->writers_waiting > 0)
        pthread_cond_wait(&log->read_allowed, &log->global_lock);
    log->readers_inside++;
    pthread_mutex_unlock(&log->global_lock);
}

void reader_unlock(server_log log) {
    pthread_mutex_lock(&log->global_lock);
    log->readers_inside--;
    if (log->readers_inside == 0)
        pthread_cond_signal(&log->write_allowed);
    pthread_mutex_unlock(&log->global_lock);
}

void writer_lock(server_log log) {
    pthread_mutex_lock(&log->global_lock);
    log->writers_waiting++;
    while (log->writers_inside + log->readers_inside > 0)
        pthread_cond_wait(&log->write_allowed, &log->global_lock);
    log->writers_waiting--;
    log->writers_inside++;
    pthread_mutex_unlock(&log->global_lock);
}

void writer_unlock(server_log log) {
    pthread_mutex_lock(&log->global_lock);
    log->writers_inside--;
    if (log->writers_inside == 0) {
        pthread_cond_broadcast(&log->read_allowed);
        pthread_cond_signal(&log->write_allowed);
    }
    pthread_mutex_unlock(&log->global_lock);
}

void destroy_log_list(server_log log){
    struct LogNode* current = log->head;
    while (current != NULL) {
        struct LogNode* next = current->next;
        free(current->data);
        free(current);
        current = next;
    }
}

// Creates a new server log instance (stub)
server_log create_log() {
    server_log log = malloc(sizeof(struct Server_Log));
    if (log == NULL) {
        return NULL; //TODO: maybe change the behavior
    }
    init_lock(log);
    return log;
}

// Destroys and frees the log (stub)
void destroy_log(server_log log) {
    if (log == NULL) {
        return; // TODO: maybe change the behavior
    }
    destroy_log_list(log);
    pthread_mutex_destroy(&log->global_lock);
    pthread_cond_destroy(&log->read_allowed);
    pthread_cond_destroy(&log->write_allowed);
    free(log);
}


// Returns dummy log content as string (stub)
int get_log(server_log log, char** dst) {
    // TODO: Return the full contents of the log as a dynamically allocated string
    // This function should handle concurrent access

    /*const char* dummy = "Log is not implemented.\n";
    int len = strlen(dummy);
    *dst = (char*)malloc(len + 1); // Allocate for caller
    if (*dst != NULL) {
        strcpy(*dst, dummy);
    }
    return len;*/

    if (log == NULL || dst == NULL) {
        return 0; // TODO: maybe change the behavior
    }

    reader_lock(log);
    int total_length = 0;

    struct LogNode* current = log->head;
    while (current != NULL) {
        total_length += current->data_length;
        current = current->next;
    }

    *dst = malloc(total_length + 1);
    if (!*dst) { // Memory allocation failed
        reader_unlock(log);
        return 0;
    }

    char* ptr = *dst;
    current = log->head;
    while (current != NULL) {
        memcpy(ptr, current->data, current->data_length);
        ptr += current->data_length;
        current = current->next;
    }
    *ptr = '\0';
    reader_unlock(log);
    return total_length;
}

// Appends a new entry to the log (no-op stub)
void add_to_log(server_log log, const char* data, int data_len) {
    // TODO: Append the provided data to the log
    // This function should handle concurrent access
    if (log == NULL || data == NULL || data_len <= 0) {
        return; // TODO: maybe change the behavior
    }

    writer_lock(log);

    struct LogNode* node = malloc(sizeof(struct LogNode));
    if (!node) {
        writer_unlock(log);
        return; // TODO: maybe change the behavior
    }

    node->data = malloc(data_len + 1);
    if (!node->data) {
        free(node);
        writer_unlock(log);
        return; // TODO: maybe change the behavior
    }

    // creates the node with the data
    memcpy(node->data, data, data_len);
    node->data[data_len] = '\0'; 
    node->data_length = data_len;
    node->next = NULL;

    if (log->tail == NULL)
    {
        log->tail->next = node; 
    } else {
        log->head = node; 
    }
    log->tail = node; 
    writer_unlock(log);
}
