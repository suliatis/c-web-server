#include <fcntl.h>
#include <netinet/in.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/_endian.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

int main() {
  int sock = socket(AF_INET, SOCK_STREAM, 0);
  printf("[SERVER] socket: %d\n", sock);
  int port = htons(8080);
  printf("[SERVER] port: %d\n", ntohs(port));
  struct sockaddr_in addr_in = {
      .sin_family = AF_INET,
      .sin_port = port,
      .sin_addr = 0,
  };
  int bound = bind(sock, (struct sockaddr *)&addr_in, sizeof(addr_in));
  printf("[SERVER] bound: %d\n", bound);

  listen(sock, 10);
  int accepted = accept(sock, 0, 0);
  printf("[SERVER] accepted: %d\n", accepted);

  char request_buffer[256] = {0};
  ssize_t bytes_recvd = recv(accepted, request_buffer, 256, 0);
  printf("[SERVER] bytes received: %zd\n", bytes_recvd);

  // GET /file.html
  char *path = request_buffer + 5; // skipping the method
  *strchr(path, ' ') = 0;          // remove anything after the first space
  char relative_path[256] = {0};
  sprintf(relative_path, "%s%s", ".", relative_path);
  printf("[SERVER] path resolved: %s\n", relative_path);
  int open_file = open(relative_path, O_RDONLY);
  printf("[SERVER] file opened: %d\n", open_file);

  char read_buffer[256] = {0};
  ssize_t bytes_read = 1;
  while (bytes_read > 0) {
    bytes_read = read(open_file, read_buffer, 256);
    printf("[SERVER] bytes read: %zd\n", bytes_read);
    ssize_t byetes_wrote = write(accepted, read_buffer, bytes_read);
    printf("[SERVER] bytes wrote: %zd\n", byetes_wrote);
  }

  close(open_file);
  close(accepted);
  close(sock);
}
