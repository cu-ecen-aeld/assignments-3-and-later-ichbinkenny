#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/syslog.h>
#include <sys/types.h>
#include <unistd.h>

#define AESD_TMP_FILE_PATH "/var/tmp/aesdsocketdata"
#define RECV_SEND_BUFF_SIZE 1024
#define MAX_START_BUFFER_SIZE 512

int server_socket;
struct addrinfo *address_info;
struct addrinfo hints;
int log_file_handle = -1;
bool should_terminate = false;

void delete_tmp_file() { unlink(AESD_TMP_FILE_PATH); }

void cleanup() {
  closelog();
  close(server_socket);
  if (-1 != log_file_handle) {
    close(log_file_handle);
  }
  delete_tmp_file();
}

void cleanup_and_exit() {
  printf("Caught signal, exiting\n");
  syslog(LOG_USER, "Caught signal, exiting\n");
  cleanup();
  delete_tmp_file();
  exit(0);
}

void open_tmp_file() {
  if (-1 == log_file_handle) {
    log_file_handle = open(AESD_TMP_FILE_PATH, O_CREAT | O_RDWR);
  }
  if (-1 == log_file_handle) {
    cleanup();
    exit(-1);
  }
}

int write_data_to_tmp_file(const char *data) {
  if (-1 != log_file_handle) {
    printf("Writing data: %s to the tmp file.\n", data);
    return write(log_file_handle, data, strlen(data));
  }
  return -1;
}

int send_log_file_to_client(int client_socket) {
  if (-1 != log_file_handle) {
    char message_buffer[RECV_SEND_BUFF_SIZE] = {0};
    int num_received_bytes = 0;
    // Seek to beginning of file.
    lseek(log_file_handle, 0, SEEK_SET);
    while (0 < (num_received_bytes = read(log_file_handle, message_buffer,
                                          RECV_SEND_BUFF_SIZE))) {
      int status = 0;
      printf("Read bytes: %s\n", message_buffer);
      if (0 > (status = send(client_socket, message_buffer, num_received_bytes,
                             0))) {
        return status;
      }
    }
    printf("Sent log file contents!\n");
    // The final loop should either set this to 0 for an EOF,
    // or -1 if an error occurred.
    return num_received_bytes;
  }
  printf("Log file was invalid...\n");
  return -1;
}

void run_server(int server_socket) {
  if (-1 == listen(server_socket, 8)) {
    cleanup();
    perror("Failed to start listening for clients...\n");
    exit(-1);
  }
  freeaddrinfo(address_info);
  while (!should_terminate) {
    printf("Waiting for client...\n");
    struct sockaddr client_addr_info = {0};
    socklen_t client_addr_len;
    int client_socket;
    char client_ip_addr[INET_ADDRSTRLEN];
    if (-1 != (client_socket = accept(server_socket, &client_addr_info,
                                      &client_addr_len))) {
      inet_ntop(AF_INET, (struct sockaddr_in *)(&client_addr_info),
                client_ip_addr, sizeof(client_ip_addr));
      syslog(LOG_INFO, "Accepted connection from %s", client_ip_addr);
      printf("Accepted connection from %s\n", client_ip_addr);
      // Read data, write to file, send response.
      // We have to potentially resize this array, so we use OS given memory
      // instead of a char[]

      int num_read_bytes = 0;
      int current_position = 0;
      int iterations = 1;

      char *string_data = calloc(MAX_START_BUFFER_SIZE, sizeof(char));

      while (
          (num_read_bytes = recv(client_socket, string_data + current_position,
                                 MAX_START_BUFFER_SIZE, 0)) > 0) {
        // we have data, now we need to see if a newline was found.
        if (NULL != strchr(string_data, '\n')) {
          // FOUND NEWLINE. Leave function
          break;
        }

        // If we're here, we ran out of buffer room before the newline was sent.
        // Realloc the data to try and find the newline.
        iterations++;
        current_position += num_read_bytes;
        string_data =
            (char *)realloc(string_data, iterations * MAX_START_BUFFER_SIZE);

        // Validate that we got new memory
        if (NULL == string_data) {
          close(client_socket);
          free(string_data);
          cleanup();
          exit(-1);
        }
      }

      // Have message, now need to write to log file.
      printf("Received message: %s\n", string_data);
      syslog(LOG_USER, "%s\n", string_data);
      if (-1 == write_data_to_tmp_file(string_data)) {
        printf("Failed to write data to file. Errno: %d\n", errno);
        close(client_socket);
        free(string_data);
        cleanup();
        exit(-1);
      }

      // Now, read all the log file into a message and send it back to the
      // client.
      printf("Sending temp file to client...\n");
      if (-1 == send_log_file_to_client(client_socket)) {
        printf("Failed to send data to client. Errno: %d\n", errno);
        close(client_socket);
        free(string_data);
        cleanup();
        exit(-1);
      }

      free(string_data);
      // write_tmp_file_data_to_socket(client_socket);

      // Close connection
      syslog(LOG_USER, "Closed connection from %s\n", client_ip_addr);
      close(client_socket);
    }
  }

  cleanup();
}

int main(int argc, char *argv[]) {

  // setup signal handling
  signal(SIGTERM, cleanup_and_exit);
  signal(SIGINT, cleanup_and_exit);

  open_tmp_file();
  openlog("", 0, LOG_USER);
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;
  if (0 != getaddrinfo(NULL, "9000", &hints, &address_info)) {
    cleanup();
    perror("Failed to get addr info...\n");
    return -1;
  }

  server_socket = socket(AF_INET, SOCK_STREAM, 0);

  int tmp = 0;

  if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &tmp, sizeof(int)) ==
      -1) {
    perror("Unable to set socket option for reusing address.\n");
  }

  if (0 !=
      bind(server_socket, address_info->ai_addr, sizeof(struct addrinfo))) {
    cleanup();
    perror("Failed to bind...\n");
    return -1;
  }

  // check for a -d flag to see if we are to fork
  // and run as a daemon.
  if (argc == 2 && 0 == strcmp(argv[1], "-d")) {
    printf("Found request to run as daemon.\n");
    int pid = fork();
    if (-1 == pid) {
      cleanup();
      return -1;
    } else if (0 != pid) {
      // This is a parent, but we want the child to run on its own.
      // We need to exit here, without cleanup.
      exit(0);
    }

    // At this point, we are in the child process, but need to reassign our
    // session group
    if (-1 == setsid()) {
      // Failed to get a session id. Close.
      cleanup();
      exit(-1);
    }

    // Set our working directory to the root of the system.
    if (-1 == chdir("/")) {
      cleanup();
      exit(-1);
    }

    // Pipe stdout, stdin, and stderr for this process to /dev/null so no prints
    // are displayed.
    int dev_null_fd = open("/dev/null", O_WRONLY);
    dup2(dev_null_fd, 0);
    dup2(dev_null_fd, 1);
    dup2(dev_null_fd, 2);

    // We may want to also handle stderr since errors are now going to log file.
    // Skipping for now.
    run_server(server_socket);
    cleanup_and_exit();
  } else {
    run_server(server_socket);
    cleanup_and_exit();
  }
}
