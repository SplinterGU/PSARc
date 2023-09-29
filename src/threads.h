#ifndef __THREADS_H
#define __THREADS_H

#include <stdint.h>
#include <pthread.h>

/**
 * Enumeration representing the status of a thread.
 */
enum {
    THREADS_STAT_FREE = 0,                /**< Thread is free. */
    THREADS_STAT_RESERVED,                /**< Thread is reserved for a task. */
    THREADS_STAT_RUNNING,                 /**< Thread is running a task. */
    THREADS_STAT_WAIT_FOR_ORDERER_CONTINUE /**< Thread is waiting for orderer's signal to continue. */
};

/**
 * Structure representing thread information.
 */
typedef struct {
    int tid;                    /**< Thread ID. */
    int status;                 /**< Thread status: FREE, RESERVED, RUNNING, WAIT_FOR_ORDERER_CONTINUE. */
    pthread_t thread_hnd;       /**< Thread handle. */
    pthread_mutex_t mutex;      /**< Thread mutex. */
    pthread_cond_t condition;   /**< Thread condition. */
    void *data;                 /**< Custom data associated with the thread. */
    void *(*function)(void *);  /**< User-defined function to be executed by the thread. */
} THREADS_INFO;

/**
 * Macro to retrieve the user data associated with a thread.
 */
#define THREAD_GET_USER_DATA(ti) ((THREADS_INFO *)(ti))->data

/**
 * Macro to retrieve the ID of a thread.
 */
#define THREAD_GET_ID(ti) ((THREADS_INFO *)(ti))->tid

/**
 * Get the number of available CPU threads.
 *
 * @return The number of available CPU threads.
 */
int threads_get_max();

/**
 * Initializes thread management and user data.
 *
 * @param num_threads       The number of threads to initialize.
 * @param user_data_size    The size for user data allocation.
 *
 * @return 0 if successful, -1 on error.
 */
int threads_init(size_t num_threads, uint32_t user_data_size);

/**
 * Frees allocated memory and resources for thread management.
 */
void threads_free();

/**
 * Waits for a thread with a specific order to complete its execution.
 *
 * This function is used to wait for a thread with a designated order to finish its execution
 * before proceeding. Each thread is assigned a unique order, and this function ensures that
 * the thread with the specified order finishes before allowing further progress.
 *
 * @param ti Pointer to the THREADS_INFO structure of the completed thread.
 */
void threads_wait_for_orderer_continue(THREADS_INFO *ti);

/**
 * Notifies that a thread has completed and releases resources.
 *
 * @param ti Pointer to the THREADS_INFO structure of the completed thread.
 */
void threads_completed(THREADS_INFO *ti);

/**
 * Waits for all threads to complete their execution.
 *
 * This function continuously checks if there are any threads running and waits for them to finish
 * using a small sleep interval. It ensures that the program waits until all threads have completed
 * their execution before proceeding.
 */
void threads_wait_for_completion();

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
int threads_get_free_slot(void **user_data);

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
int threads_start_task(int slot, void *(*function)(void *), void *data);

#endif /* __THREADS_H */
