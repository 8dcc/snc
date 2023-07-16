
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <unistd.h>    /* sleep, write, close */
#include <arpa/inet.h> /* htonl, htons */
#include <sys/socket.h>

typedef struct sockaddr sockaddr;
typedef struct sockaddr_in sockaddr_in;

#define MSG "Ping"

/**
 * @brief Entry point of the program
 * @param argc Number of arguments
 * @param argv Vector of string arguments
 * @return Exit code
 */
int main(/* int argc, char** argv */) {
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(sockaddr_in));

    /* Fill the struct we just declared.
     * See: https://www.gta.ufrj.br/ensino/eel878/sockets/sockaddr_inman.html */
    server_addr.sin_family      = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); /* htonl -> uint32_t */
    server_addr.sin_port        = htons(1337);       /* htons -> uint16_t */

    /* Bind socket file descriptor to the struct we just filled (but casted) */
    bind(listen_fd, (sockaddr*)&server_addr, sizeof(sockaddr));

    listen(listen_fd, 10);

    int conn_fd = 0;
    for (;;) {
        conn_fd = accept(listen_fd, (sockaddr*)NULL, NULL);

        write(conn_fd, MSG, strlen(MSG));

        close(conn_fd);
        sleep(1);
    }

    return 0;
}
