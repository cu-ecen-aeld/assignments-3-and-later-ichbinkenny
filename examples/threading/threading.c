#include "threading.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// Optional: use these functions to add debug or error prints to your
// application
#define DEBUG_LOG(msg, ...)
// #define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg, ...) printf("threading ERROR: " msg "\n", ##__VA_ARGS__)

void *threadfunc(void *thread_param) {

  struct thread_data *thread_data_store = (struct thread_data *)(thread_param);
  // TODO: wait, obtain mutex, wait, release mutex as described by thread_data
  // structure hint: use a cast like the one below to obtain thread arguments
  // from your parameter
  // struct thread_data* thread_func_args = (struct thread_data *) thread_param;

  // wait for obtain
  usleep(thread_data_store->wait_time * 1000);
  // Lock mutex
  if (0 != pthread_mutex_lock(thread_data_store->mutex)) {
    thread_data_store->thread_complete_success = false;
    return (void *)thread_data_store;
  }


  usleep(thread_data_store->release_time);

  pthread_mutex_unlock(thread_data_store->mutex);

  thread_data_store->thread_complete_success = true;

  return (void *)thread_data_store;
}

bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,
                                  int wait_to_obtain_ms,
                                  int wait_to_release_ms) {
  struct thread_data *thread_data_store = (struct thread_data*)malloc(sizeof(struct thread_data));
  /**
   * TODO: allocate memory for thread_data, setup mutex and wait arguments, pass
   * thread_data to created thread using threadfunc() as entry point.
   *
   * return true if successful.
   *
   * See implementation details in threading.h file comment block
   */
  thread_data_store->mutex = mutex;
  thread_data_store->wait_time = wait_to_obtain_ms;
  thread_data_store->release_time = wait_to_release_ms;
  thread_data_store->thread_complete_success =
      false; // Haven't completed yet so not successful.
  // Create the thread.

  //pthread_mutex_lock(mutex);
  int thread_create_status =
      pthread_create(thread, NULL, &threadfunc, (void *)thread_data_store);
  if (thread_create_status != 0) {
    free(thread_data_store);
    return false;
  }
    
  //free(thread_data_store);
  return true;
}
