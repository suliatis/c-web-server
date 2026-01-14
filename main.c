#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

static void log_info(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  printf("[SERVER:INFO] ");
  vprintf(fmt, ap);
  printf("\n");
  va_end(ap);
}

static void log_error(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  printf("[SERVER:ERROR] ");
  vprintf(fmt, ap);
  printf("\n");
  va_end(ap);
}

int main() {
  int sock = socket(AF_INET, SOCK_STREAM, 0);
  log_info("Socket created: %d", sock);
  int port = 8080;
  struct sockaddr_in addr_in = {
      .sin_family = AF_INET,
      .sin_port = htons(port),
      .sin_addr.s_addr = INADDR_ANY,
  };
  int bound = bind(sock, (struct sockaddr *)&addr_in, sizeof(addr_in));
  if (bound >= 0) {
    log_info("Socket bound to localhost:%d", port);
  } else {
    log_error("Binding socket failed: %s", strerror(errno));
  }

  listen(sock, 10);
  log_info("Listening for connections");
  int accepted = accept(sock, 0, 0);
  if (accepted >= 0) {
    log_info("Connection accepted: %d", accepted);
  } else {
    log_error("Accepting connection failed: %s", strerror(errno));
  }

  char request_buffer[256] = {0};
  ssize_t bytes_recvd = recv(accepted, request_buffer, 256, 0);
  log_info("Bytes received: %zd", bytes_recvd);

  // GET /file.html
  char *path = request_buffer + 5; // skipping the method
  *strchr(path, ' ') = 0;          // remove anything after the first space
  int open_file = open(path, O_RDONLY);
  if (open_file >= 0) {
    log_info("Static resource opened: %d, %s", open_file, path);
  } else {
    log_error("Opening static resource failed: %s with %s", path,
              strerror(errno));
  }

  char *statusline = "HTTP/1.1 200 OK\r\n";
  char *contenttype = "Content-Type: text/html\r\n";
  // TODO: implement content length in bytes
  // char *contentlength = "Content-Length: 1\r\n";
  char *connection = "Connection: close\r\n";
  char *empty = "\r\n";

  write(accepted, statusline, strlen(statusline));
  write(accepted, contenttype, strlen(contenttype));
  // write(accepted, contentlength, strlen(contentlength));
  write(accepted, connection, strlen(connection));
  write(accepted, empty, strlen(empty));

  char read_buffer[256] = {0};
  ssize_t bytes_read = 1;
  ssize_t total_bytes = 0;
  while (bytes_read > 0) {
    bytes_read = read(open_file, read_buffer, 256);
    ssize_t bytes_wrote = write(accepted, read_buffer, bytes_read);
    total_bytes = total_bytes + bytes_wrote;
  }
  log_info("Total bytes wrote to the client: %d %zd bytes", accepted, total_bytes);

  close(open_file);
  close(accepted);
  close(sock);
}
