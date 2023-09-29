#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <string.h>

#define _GNU_SOURCE
#define __USE_GNU
#include <pthread.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#include "threads.h"

THREADS_INFO *threads_info = NULL;

int threads_current = 1;             // Current active thread
int threads_last = 1;                // Last thread ID
int threads_available = 0;           // Number of available threads
int threads_running = 0;             // Number of threads currently running

size_t threads_max = 0;              // Total number of threads

pthread_mutex_t threads_shared_mutex;
pthread_cond_t threads_shared_condition = PTHREAD_COND_INITIALIZER;

/**
 * Get the number of available CPU threads.
 *
 * This function retrieves the number of available CPU threads on the system. It uses
 * platform-specific methods to determine the number of threads.
 *
 * @return The number of available CPU threads.
 */
int threads_get_max() {
    int num_threads = 0;

#ifdef _WIN32
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    num_threads = sysInfo.dwNumberOfProcessors;
#else
    num_threads = sysconf(_SC_NPROCESSORS_ONLN);
#endif

    return num_threads;
}

/**
 * Thread function that handles the execution of tasks.
 *
 * @param arg Pointer to the THREADS_INFO structure for the thread.
 */
void *threads_fn(void *arg) {
    THREADS_INFO *ti = (THREADS_INFO *)arg;

    while (1) {
        // Wait for awake
        pthread_mutex_lock(&ti->mutex);
        ti->status = THREADS_STAT_FREE; // free
        pthread_cond_wait(&ti->condition, &ti->mutex);
    
        pthread_mutex_lock(&threads_shared_mutex);
        threads_running++;
        pthread_mutex_unlock(&threads_shared_mutex);

        ti->status = THREADS_STAT_RUNNING; // running
        pthread_mutex_unlock(&ti->mutex);

        // Awake
        ti->function(ti);
    }

    pthread_exit(NULL);
    return NULL;
}

/**
 * Initializes thread management and user data.
 *
 * @param num_threads       The number of threads to initialize.
 * @param user_data_size    The size for user data allocation.
 *
 * @return 0 if successful, -1 on error.
 */
int threads_init(size_t num_threads, uint32_t user_data_size) {
    // Check if the THREADS_INFO has already been initialized
    if (threads_info)
        threads_free();

    // Allocate memory for the array of pointers to thread-specific info
    threads_info = (THREADS_INFO *)calloc(num_threads, sizeof(THREADS_INFO));
    if (!threads_info) {
        perror("Error allocating memory for threads_info");
        return -1;
    }

    // Store the number of threads globally
    threads_max = num_threads;

    threads_current = 1;                // Current active thread
    threads_last = 1;                   // Last thread ID
    threads_available = threads_max;    // Number of available threads
    threads_running = 0;                // Number of threads currently running

    pthread_mutex_init(&threads_shared_mutex, NULL);
    pthread_cond_init(&threads_shared_condition, NULL);

    // Initialize each buffer individually
    for (size_t i = 0; i < threads_max; ++i) {
        threads_info[i].status = -1; // THREADS_STAT_FREE only when thread is started
        threads_info[i].data = malloc(user_data_size);
        if (!threads_info[i].data) {
            perror("Error allocating memory for user data\n");
            threads_free();
            return -1;
        }

        pthread_mutex_init(&threads_info[i].mutex, NULL);
        pthread_cond_init(&threads_info[i].condition, NULL);

        if (pthread_create(&threads_info[i].thread_hnd, NULL, threads_fn, (void *)&threads_info[i])) {
            perror("Error creating thread\n");
            threads_free();
            return -1;
        }
    }

    return 0;
}

/**
 * Frees allocated memory and resources for thread management.
 */
void threads_free() {
    // Check if the threads_info have been initialized
    if (!threads_info)
        return;

    // Free threads & data individually
    for (size_t i = 0; i < threads_max; ++i) {
        pthread_cancel(threads_info[i].thread_hnd);
        pthread_join(threads_info[i].thread_hnd, NULL);
        threads_info[i].thread_hnd = 0;

        pthread_mutex_destroy(&threads_info[i].mutex);

        free(threads_info[i].data);
        threads_info[i].data = NULL;
    }

    free(threads_info);
    threads_info = NULL;

    pthread_mutex_destroy(&threads_shared_mutex);
    pthread_cond_destroy(&threads_shared_condition);

    threads_max = 0;

    threads_current = 1;                // Current active thread
    threads_last = 1;                   // Last thread ID
    threads_available = threads_max;    // Number of available threads
    threads_running = 0;                // Number of threads currently running
}

/**
 * Waits for a thread with a specific order to complete its execution.
 *
 * This function is used to wait for a thread with a designated order to finish its execution
 * before proceeding. Each thread is assigned a unique order, and this function ensures that
 * the thread with the specified order finishes before allowing further progress.
 *
 * @param ti Pointer to the THREADS_INFO structure of the completed thread.
 */
void threads_wait_for_orderer_continue(THREADS_INFO *ti) {
    // Wait for the thread with the specified tid to complete
    pthread_mutex_lock(&threads_shared_mutex);

    pthread_mutex_lock(&ti->mutex);
    int tid = THREAD_GET_ID(ti);
    ti->status = THREADS_STAT_WAIT_FOR_ORDERER_CONTINUE; // wait to continue
    pthread_mutex_unlock(&ti->mutex);

    while (tid != threads_current) {
        pthread_cond_wait(&threads_shared_condition, &threads_shared_mutex);
    }

    pthread_mutex_unlock(&threads_shared_mutex);
}

/**
 * Notifies that a thread has completed and releases resources.
 *
 * @param ti Pointer to the THREADS_INFO structure of the completed thread.
 */
void threads_completed(THREADS_INFO *ti) {
    pthread_mutex_lock(&threads_shared_mutex);

    threads_current++;
    if (!threads_current)
        threads_current = 1;

    threads_available++;
    threads_running--;

    pthread_cond_broadcast(&threads_shared_condition);

    pthread_mutex_unlock(&threads_shared_mutex);
}

/**
 * Waits for all threads to complete their execution.
 *
 * This function continuously checks if there are any threads running and waits for them to finish
 * using a small sleep interval. It ensures that the program waits until all threads have completed
 * their execution before proceeding.
 */
void threads_wait_for_completion() {
    while (1) {
        pthread_mutex_lock(&threads_shared_mutex);
        int done = !threads_running && threads_available == threads_max;
        pthread_mutex_unlock(&threads_shared_mutex);
        if (done)
            return;
        usleep(100000); // Sleep for 100 milliseconds on Unix-like systems
    }
}

/**
 * Retrieves a free slot for thread-specific task.
 *
 * This function searches for a free slot in thread-specific task and assigns it. Each thread has
 * its own set of data, and this function looks for an available user data and returns it through the
 * `user_data` pointer.
 *
 * @param user_data     Pointer to a pointer that will receive the address of the thread-specific user data.
 * @return              The index of the thread where the buffer was assigned.
 */
int threads_get_free_slot(void **user_data) {
    static int last_idx = 0;
    while (1) {
        pthread_mutex_lock(&threads_shared_mutex);
        if (threads_available) {
            if (++last_idx >= threads_max) last_idx = 0;
            pthread_mutex_lock(&threads_info[last_idx].mutex);
            if (threads_info[last_idx].status == THREADS_STAT_FREE) {
                threads_available--;
                threads_info[last_idx].status = THREADS_STAT_RESERVED;
                *user_data = threads_info[last_idx].data;
                pthread_mutex_unlock(&threads_info[last_idx].mutex);
                pthread_mutex_unlock(&threads_shared_mutex);
                return last_idx;
            }
            pthread_mutex_unlock(&threads_info[last_idx].mutex);
        }
//        pthread_cond_broadcast(&threads_shared_condition);
        pthread_mutex_unlock(&threads_shared_mutex);
        usleep(1000);
    }
    return -1;
}

/**
 * Starts a task in a specific slot with a given function and data.
 *
 * This function starts a task in the specified slot with the provided function and data. It checks
 * if the slot is within the valid range and if it's reserved. If so, it assigns the data and function
 * to the thread and signals it to start.
 *
 * @param slot     The slot in which to start the task.
 * @param function The function to be executed.
 * @param data     The data to be passed to the function.
 * @return 0 if successful, 1 otherwise.
 */
int threads_start_task(int slot, void *(*function)(void *), void *data) {
    if (slot >= threads_max)
        return 1;

    pthread_mutex_lock(&threads_info[slot].mutex);

    if (threads_info[slot].status != THREADS_STAT_RESERVED) {
        pthread_mutex_unlock(&threads_info[slot].mutex);
        return 1;
    }

    threads_info[slot].data = data;
    threads_info[slot].function = function;
    threads_info[slot].tid = threads_last++;
    if (!threads_last)
        threads_last = 1;

    while (threads_info[slot].status == THREADS_STAT_RESERVED) {
        pthread_cond_signal(&threads_info[slot].condition);
        pthread_mutex_unlock(&threads_info[slot].mutex);
        usleep(1);
        pthread_mutex_lock(&threads_info[slot].mutex);
    }

    pthread_mutex_unlock(&threads_info[slot].mutex);

    return 0;
}
