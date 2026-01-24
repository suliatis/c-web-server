#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/_types/_ssize_t.h>
#include <sys/socket.h>

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

// TODO: keep str null terminated by reserving an extra byte if necessary
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

  return (ssize_t)len;
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

  ssize_t bytes_appended = str_append(str, buffer, (size_t)bytes_recvd);
  if (bytes_appended < 0) {
    free(buffer);
    return -1;
  }

  free(buffer);
  return bytes_appended;
}

bool str_ends_with(const struct Str *str, const char *needle, size_t len) {
  if (str->len < len) {
    return false;
  }

  char *data = &str->data[str->len - len];
  return memcmp(data, needle, len) == 0;
}
