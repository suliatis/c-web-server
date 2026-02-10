#include "str.h"

#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

const struct Str STR_NL = STR("\r\n");
const struct Str STR_SPC = STR(" ");

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
  if (str_is_readonly(str)) {
    return;
  }

  free(str->data);
  str->data = NULL;
  str->len = 0;
  str->cap = 0;
}

// TODO: add consts for empty and/or null strs
bool str_is_readonly(const struct Str *str) { return str->cap == 0; }

bool str_is_empty(const struct Str *str) { return str->len == 0; }

bool str_equals(const struct Str *left, const struct Str *right) {
  return memcmp(left->data, right->data, left->len) == 0;
}

bool str_starts_with(const struct Str *str, const struct Str *prefix) {
  if (str->len < prefix->len) {
    return false;
  }

  return memcmp(str->data, prefix->data, prefix->len) == 0;
}

bool str_ends_with(const struct Str *str, const struct Str *suffix) {
  if (str->len < suffix->len) {
    return false;
  }

  char *data = &str->data[str->len - suffix->len];
  return memcmp(data, suffix->data, suffix->len) == 0;
}

size_t str_get_bytes(const struct Str *str, char *out, size_t len) {
  size_t min_len = len;
  if (str->len < len) {
    min_len = str->len;
  }
  memcpy(out, str->data, min_len);
  return min_len;
}

ssize_t str_copy(struct Str *dst, const struct Str *src) {
  int created = str_create(dst, src->cap);
  if (created < 0) {
    return created;
  }

  memcpy(dst->data, src->data, src->len);
  dst->len = src->len;
  return (ssize_t)dst->len;
}

// TODO: keep str null terminated by reserving an extra byte if necessary
ssize_t str_append_char(struct Str *str, const char *bytes, size_t len) {
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

ssize_t str_append(struct Str *str, const struct Str *postfix) {
  char *postfix_bytes = malloc(postfix->len);
  if (postfix_bytes == NULL) {
    return -1;
  }

  size_t postfix_len = str_get_bytes(postfix, postfix_bytes, postfix->len);
  ssize_t res = str_append_char(str, postfix_bytes, postfix_len);
  free(postfix_bytes);
  return res;
}

ssize_t str_prepend_char(struct Str *str, const char *bytes, size_t len) {
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

  memmove(&nxt_data[len], nxt_data, str->len);
  memcpy(nxt_data, bytes, len);

  str->len = nxt_len;
  str->cap = nxt_cap;
  str->data = nxt_data;

  return (ssize_t)len;
}

ssize_t str_prepend(struct Str *str, const struct Str *prefix) {
  char *prefix_bytes = malloc(prefix->len);
  if (prefix_bytes == NULL) {
    return -1;
  }

  size_t prefix_len = str_get_bytes(prefix, prefix_bytes, prefix->len);
  ssize_t res = str_prepend_char(str, prefix_bytes, prefix_len);
  free(prefix_bytes);
  return res;
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

  ssize_t bytes_appended = str_append_char(str, buffer, (size_t)bytes_recvd);
  if (bytes_appended < 0) {
    free(buffer);
    return -1;
  }

  free(buffer);
  return bytes_appended;
}

ssize_t str_read(struct Str *out, int fd, size_t len) {
  char *buffer = malloc(len);
  if (buffer == NULL) {
    return -1;
  }

  ssize_t bytes_read = read(fd, buffer, len - 1);
  if (bytes_read < 0) {
    free(buffer);
    return -1;
  }

  ssize_t bytes_appended = str_append_char(out, buffer, (size_t)bytes_read);
  if (bytes_appended < 0) {
    free(buffer);
    return -1;
  }

  free(buffer);
  return bytes_appended;
}

ssize_t str_write(const struct Str *str, int fd) {
  char *buffer = malloc(str->len);
  if (buffer == NULL) {
    return -1;
  }

  size_t buffer_len = str_get_bytes(str, buffer, str->len);
  ssize_t bytes_wrote = write(fd, buffer, buffer_len);
  free(buffer);
  return bytes_wrote;
}

void str_cursor_init(struct StrCursor *cursor, const struct Str *str) {
  cursor->data = str->data;
  cursor->len = str->len;
}

struct Str str_cursor_view(const struct StrCursor *cursor) {
  struct Str view = {
      .data = (char *)cursor->data,
      .len = cursor->len,
      .cap = 0,
  };

  return view;
}

size_t str_cursor_inc(struct StrCursor *out) {
  if (out->len == 0) {
    return 0;
  }

  out->data = &out->data[1];
  out->len = out->len - 1;
  return out->len;
}

size_t str_cursor_inc_by(struct StrCursor *out, size_t by) {
  while (by > 0) {
    by = by - 1;
    str_cursor_inc(out);
  }

  return out->len;
}

struct Str str_cursor_next(struct StrCursor *cursor, const struct Str *delim) {
  size_t steps = 0;
  struct Str out = {
      .data = (char *)cursor->data,
      .len = cursor->len,
      .cap = 0,
  };

  while (cursor->len > 0) {
    struct Str view = str_cursor_view(cursor);
    if (str_starts_with(&view, delim)) {
      str_cursor_inc_by(cursor, delim->len);
      break;
    }

    str_cursor_inc(cursor);
    steps = steps + 1;
  }

  out.len = steps;
  return out;
}
