#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

#include "src/str.c"

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

// TODO: use str struct instead of char
// TODO: create an str constructor for literals
static const char HTTP_HEADERS_END[] = "\r\n\r\n";

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

  // FIXME: handle str_create and str_recv(0 -> connection closed, -1 -> error)
  // errors
  // FIXME: free request_header
  struct Str request_header = {0};
  str_create(&request_header, 256);
  // TODO: while bytes recvd or limit (8192) reached
  while (!str_ends_with(&request_header, HTTP_HEADERS_END,
                        strlen(HTTP_HEADERS_END))) {
    ssize_t bytes_recvd = str_recv(&request_header, accepted, 256);
    log_info("Reading request header from %d, bytes received: %zd", accepted,
             bytes_recvd);
  }

  char method[16];
  char path[256];
  char protocol[16];
  int words =
      sscanf(request_header.data, "%15s /%255s %15s", method, path, protocol);
  if (words < 3) {
    log_error("Malformed request: %s", request_header.data);

    char *statusline = "HTTP/1.1 400 Bad Request\r\n";
    char *contenttype = "Content-Type: text/html\r\n";
    char *connection = "Connection: close\r\n";
    char *empty = "\r\n";
    char body[256];
    sprintf(body, "Malformed request: %s", request_header.data);

    write(accepted, statusline, strlen(statusline));
    write(accepted, contenttype, strlen(contenttype));
    write(accepted, connection, strlen(connection));
    write(accepted, empty, strlen(empty));
    write(accepted, body, strlen(body));

    goto close_all;
  }

  if (strcmp(method, "GET") == 0) {
    char static_path[256];
    sprintf(static_path, "static/%s", path);
    int open_file = open(static_path, O_RDONLY);
    if (open_file >= 0) {
      log_info("Static resource opened: %d, %s", open_file, path);
    } else {
      log_error("Opening static resource failed: %s with %s", path,
                strerror(errno));

      char *statusline = "HTTP/1.1 404 Not Found\r\n";
      char *contenttype = "Content-Type: text/html\r\n";
      char *connection = "Connection: close\r\n";
      char *empty = "\r\n";
      char body[256];
      sprintf(body, "File not found: %s", path);

      write(accepted, statusline, strlen(statusline));
      write(accepted, contenttype, strlen(contenttype));
      write(accepted, connection, strlen(connection));
      write(accepted, empty, strlen(empty));
      write(accepted, body, strlen(body));

      goto close_all;
    }

    char *statusline = "HTTP/1.1 200 OK\r\n";
    char *contenttype = "Content-Type: text/html\r\n";
    char *connection = "Connection: close\r\n";
    char *empty = "\r\n";

    write(accepted, statusline, strlen(statusline));
    write(accepted, contenttype, strlen(contenttype));
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
    log_info("Total bytes wrote to the client: %d %zd bytes", accepted,
             total_bytes);

    close(open_file);
  } else {
    log_error("Method not allowed: %s", method);

    char *statusline = "HTTP/1.1 405 Method Not Allowed\r\n";
    char *contenttype = "Content-Type: text/html\r\n";
    char *connection = "Connection: close\r\n";
    char *allow = "Allow: GET\r\n";
    char *empty = "\r\n";
    char body[256];
    sprintf(body, "Method not allowed: %s", method);

    write(accepted, statusline, strlen(statusline));
    write(accepted, contenttype, strlen(contenttype));
    write(accepted, connection, strlen(connection));
    write(accepted, allow, strlen(allow));
    write(accepted, empty, strlen(empty));
    write(accepted, body, strlen(body));

    goto close_all;
  }

close_all:
  close(accepted);
  close(sock);
}
