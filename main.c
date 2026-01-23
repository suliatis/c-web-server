#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdarg.h>
#include <stddef.h>
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

struct Str {
  char *data;
  size_t len;
  size_t cap;
};

int str_create(struct Str *str, size_t cap) {
  char *data = malloc(cap);
  if (data == NULL) {
    return -1;
  }

  memset(data, 0, cap);
  str->data = data;
  str->cap = cap;
  str->len = 0;
  return 0;
}

void str_free(struct Str *str) {
  free(str->data);
  str->data = NULL;
  str->len = 0;
  str->cap = 0;
}

ssize_t str_append(struct Str *str, const char *bytes, size_t len) {
  size_t nxt_len = str->len + len;
  size_t nxt_cap = str->cap;
  char *nxt_data = str->data;

  if (nxt_len > str->cap) {
    nxt_cap = str->cap * 2;
    if (nxt_len > nxt_cap) {
      nxt_cap = nxt_len * 2;
    }

    nxt_data = realloc(str->data, nxt_cap);
    if (nxt_data == NULL) {
      return -1;
    }
  }

  strncpy(&nxt_data[str->len], bytes, len);

  str->len = nxt_len;
  str->cap = nxt_cap;
  str->data = nxt_data;

  return len;
}

ssize_t str_recv(struct Str *str, int fd, size_t len) {
  char *buffer = malloc(len);
  if (buffer == NULL) {
    return -1;
  }

  ssize_t bytes_recvd = recv(fd, buffer, len - 1, 0);
  if (bytes_recvd < 0) {
    free(buffer);
    return -1;
  }

  ssize_t bytes_appended = str_append(str, buffer, bytes_recvd);
  if (bytes_appended < 0) {
    free(buffer);
    return -1;
  }

  free(buffer);
  return bytes_appended;
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

  struct Str request = {0};
  str_create(&request, 256);
  str_recv(&request, accepted, 256);

  // char request_buffer[256] = {0};
  // ssize_t bytes_recvd = recv(accepted, request_buffer, 255, 0);
  // request_buffer[bytes_recvd] = '\0';
  // log_info("Bytes received: %zd", bytes_recvd);

  char method[16];
  char path[256];
  char protocol[16];
  int words =
      sscanf(request.data, "%15s /%255s %15s", method, path, protocol);
  if (words < 3) {
    log_error("Malformed request: %s", request.data);

    char *statusline = "HTTP/1.1 400 Bad Request\r\n";
    char *contenttype = "Content-Type: text/html\r\n";
    char *connection = "Connection: close\r\n";
    char *empty = "\r\n";
    char body[256];
    sprintf(body, "Malformed request: %s", request.data);

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
