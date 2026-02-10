#ifndef STR_H
#define STR_H

#include <stdbool.h>
#include <stddef.h>
#include <unistd.h>

/* === TYPES === */
struct Str {
  char *data;
  size_t len;
  size_t cap;
};

struct StrCursor {
  const char *data;
  size_t len;
};

/* === MACROS === */
#define STR(s) (struct Str){.data = (char *)s, .len = sizeof(s) - 1, .cap = 0}

/* === CONSTANTS === */
extern const struct Str STR_NL;
extern const struct Str STR_SPC;

/* === FUNCTION PROTOTYPES === */
int str_create(struct Str *str, size_t cap);
void str_free(struct Str *str);

bool str_is_readonly(const struct Str *str);
bool str_is_empty(const struct Str *str);
bool str_equals(const struct Str *left, const struct Str *right);
bool str_starts_with(const struct Str *str, const struct Str *prefix);
bool str_ends_with(const struct Str *str, const struct Str *suffix);

size_t str_get_bytes(const struct Str *str, char *out, size_t len);

ssize_t str_copy(struct Str *dst, const struct Str *src);
ssize_t str_append_char(struct Str *str, const char *bytes, size_t len);
ssize_t str_append(struct Str *str, const struct Str *postfix);
ssize_t str_prepend_char(struct Str *str, const char *bytes, size_t len);
ssize_t str_prepend(struct Str *str, const struct Str *prefix);

ssize_t str_recv(struct Str *str, int fd, size_t len);
ssize_t str_read(struct Str *out, int fd, size_t len);
ssize_t str_write(const struct Str *str, int fd);

void str_cursor_init(struct StrCursor *cursor, const struct Str *str);

struct Str str_cursor_view(const struct StrCursor *cursor);

size_t str_cursor_inc(struct StrCursor *out);
size_t str_cursor_inc_by(struct StrCursor *out, size_t by);
struct Str str_cursor_next(struct StrCursor *cursor, const struct Str *delim);
#endif
