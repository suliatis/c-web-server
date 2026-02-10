#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

#include "log.h"
#include "str.h"

static const struct Str HTTP_HEADERS_END = STR("\r\n\r\n");
static const struct Str HTTP_REQUEST_METHOD_GET = STR("GET");

int main(void) {
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
  struct Str http_request_header = {0};
  str_create(&http_request_header, 256);
  // TODO: while bytes recvd or limit (8192) reached
  while (!str_ends_with(&http_request_header, &HTTP_HEADERS_END)) {
    ssize_t bytes_recvd = str_recv(&http_request_header, accepted, 256);
    log_info("Reading request header from %d, bytes received: %zd", accepted,
             bytes_recvd);
  }

  struct StrCursor http_request_header_cursor = {0};
  str_cursor_init(&http_request_header_cursor, &http_request_header);
  struct Str request_line =
      str_cursor_next(&http_request_header_cursor, &STR_NL);

  struct StrCursor http_request_line_cursor = {0};
  str_cursor_init(&http_request_line_cursor, &request_line);

  struct Str http_request_method =
      str_cursor_next(&http_request_line_cursor, &STR_SPC);
  struct Str http_request_path =
      str_cursor_next(&http_request_line_cursor, &STR_SPC);
  struct Str http_request_version =
      str_cursor_next(&http_request_line_cursor, &STR_SPC);

  if (str_is_empty(&http_request_method) || str_is_empty(&http_request_path) ||
      str_is_empty(&http_request_version)) {
    log_error("Malformed request: %s", http_request_header.data);

    char *statusline = "HTTP/1.1 400 Bad Request\r\n";
    char *contenttype = "Content-Type: text/html\r\n";
    char *connection = "Connection: close\r\n";
    char *empty = "\r\n";
    char body[256];
    sprintf(body, "Malformed request: %s", http_request_header.data);

    write(accepted, statusline, strlen(statusline));
    write(accepted, contenttype, strlen(contenttype));
    write(accepted, connection, strlen(connection));
    write(accepted, empty, strlen(empty));
    write(accepted, body, strlen(body));

    goto close_all;
  }

  if (str_equals(&http_request_method, &HTTP_REQUEST_METHOD_GET)) {
    struct Str static_path = {0};
    struct Str static_path_prefix = STR("static/");
    str_copy(&static_path, &http_request_path);
    str_prepend(&static_path, &static_path_prefix);

    int open_file = open(static_path.data, O_RDONLY);
    if (open_file >= 0) {
      log_info("Static resource opened: %d, %s", open_file, static_path.data);
    } else {
      log_error("Opening static resource failed: %s with %s", static_path.data,
                strerror(errno));

      char *statusline = "HTTP/1.1 404 Not Found\r\n";
      char *contenttype = "Content-Type: text/html\r\n";
      char *connection = "Connection: close\r\n";
      char *empty = "\r\n";
      char body[256];
      sprintf(body, "File not found: %s", static_path.data);

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

    ssize_t bytes_read_size = 1;
    ssize_t total_bytes = 0;
    size_t chunk_size = 256;
    while (bytes_read_size > 0) {
      struct Str buffer = {0};
      str_create(&buffer, chunk_size);
      bytes_read_size = str_read(&buffer, open_file, chunk_size);
      ssize_t bytes_wrote = str_write(&buffer, open_file);
      total_bytes = total_bytes + bytes_wrote;
    }
    log_info("Total bytes wrote to the client: %d %zd bytes", accepted,
             total_bytes);

    close(open_file);
  } else {
    log_error("Method not allowed: %s", http_request_method.data);

    char *statusline = "HTTP/1.1 405 Method Not Allowed\r\n";
    char *contenttype = "Content-Type: text/html\r\n";
    char *connection = "Connection: close\r\n";
    char *allow = "Allow: GET\r\n";
    char *empty = "\r\n";
    char body[256];
    // FIXME: because http_request_method is a view getting the raw data
    // will return the whole string no matter what.
    sprintf(body, "Method not allowed: %s", http_request_method.data);

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
