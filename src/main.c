
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <unistd.h>    /* write, close */
#include <arpa/inet.h> /* htonl, htons */
#include <sys/socket.h>
#include <ifaddrs.h> /* getifaddrs(), etc. */
#include <net/if.h>  /* IFF_LOOPBACK */
#include <netdb.h>   /* getnameinfo(), etc. */

/*----------------------------------------------------------------------------*/
/* Macros */

/* Comment to disable interface listing on "listen" mode. */
#define LIST_INTERFACES

#define PORT    1337
#define MAX_BUF 256 /* Max size of message/response */

#define LENGTH(ARR) (sizeof(ARR) / sizeof((ARR)[0]))

#define ERR(...)                      \
    do {                              \
        fprintf(stderr, "snc: ");     \
        fprintf(stderr, __VA_ARGS__); \
        fputc('\n', stderr);          \
    } while (0)

/*----------------------------------------------------------------------------*/
/* Internal structures and enums */

typedef struct {
    const char* alias;
    const char* real;
} IpAlias;

typedef enum EMode {
    MODE_ERR,
    MODE_LISTEN,
    MODE_CONNECT,
} EMode;

/*----------------------------------------------------------------------------*/
/* Util functions */

/* Get the program mode from the arguments */
static EMode get_mode(int argc, char** argv) {
    if (argc < 2)
        return MODE_ERR;

    for (int i = 0; argv[1][i] != '\0'; i++) {
        switch (argv[1][i]) {
            case 'l': /* Listen */
                return MODE_LISTEN;
            case 'c': /* Connect */
                if (argc < 3) {
                    ERR("Not enough arguments for option \"c\".");
                    return MODE_ERR;
                }

                return MODE_CONNECT;
            case 'h': /* Help */
                return MODE_ERR;
            case '-': /* "snc -h" -> "snc h" */
                break;
            default:
                ERR("Unknown option \"%s\".", argv[1]);
                return MODE_ERR;
        }
    }

    ERR("Not enough arguments.");
    return MODE_ERR;
}

/* If the specified IP address is an alias (e.g. "localhost"), get the real IP
 * address associated with it. Otherwise return the argument unchanged.  */
static const char* unalias_ip(const char* ip) {
    IpAlias aliases[] = {
        /* alias,       real */
        { "localhost", "127.0.0.1" },
    };

    for (size_t i = 0; i < LENGTH(aliases); i++)
        if (!strcmp(ip, aliases[i].alias))
            return aliases[i].real;

    return ip;
}

static inline void list_interfaces(void) {
    struct ifaddrs* ifaddr;
    if (getifaddrs(&ifaddr) == -1) {
        ERR("Failed to list interfaces.");
        return;
    }

    fprintf(stderr, "---------------------------\n"
                    "Listening on any interface:\n");

    for (struct ifaddrs* ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        /* Ignore non-INET address families and loopback interfaces */
        if (ifa->ifa_addr->sa_family != AF_INET ||
            (ifa->ifa_flags & IFF_LOOPBACK) != 0)
            continue;

        char host[NI_MAXHOST];
        int code = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), host,
                               NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
        if (code != 0)
            continue;

        fprintf(stderr, "%s: %s\n", ifa->ifa_name, host);
    }

    fprintf(stderr, "---------------------------\n");

    freeifaddrs(ifaddr);
}

/*----------------------------------------------------------------------------*/
/* Main modes */

/* Main function for the "listen" mode. */
static void snc_listen(void) {
    /*
     * Create the socket descriptor for listening.
     *
     * domain:   AF_INET     (IPv4 address)
     * type:     SOCK_STREAM (TCP, etc.)
     * protocol: 0           (Let the kernel decide, usually TPC)
     */
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (!listen_fd) {
        ERR("Failed to create socket.");
        exit(1);
    }

#ifdef LIST_INTERFACES
    list_interfaces();
#endif

    /* Declare and clear struct */
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(struct sockaddr_in));

    /*
     * Fill the struct we just declared.
     * See: https://www.gta.ufrj.br/ensino/eel878/sockets/sockaddr_inman.html
     */
    server_addr.sin_family      = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); /* htonl -> uint32_t */
    server_addr.sin_port        = htons(PORT);       /* htons -> uint16_t */

    /* Bind socket file descriptor to the struct we just filled (but casted) */
    bind(listen_fd, (struct sockaddr*)&server_addr, sizeof(struct sockaddr));

    /* 10 is the max number of connections to queue */
    listen(listen_fd, 10);

    int conn_fd = accept(listen_fd, NULL, NULL);

    char c = 0;
    while (read(conn_fd, &c, 1) && c != EOF)
        putchar(c);

    close(conn_fd);
    close(listen_fd);
}

/* Main function for the "connect" mode. See `snc_listen' for more comments. */
static void snc_connect(const char* ip) {
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (!socket_fd) {
        ERR("Failed to create socket.");
        exit(1);
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(struct sockaddr_in));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port   = htons(PORT);

    if (!inet_pton(AF_INET, ip, &server_addr.sin_addr)) {
        ERR("IP error.");
        exit(1);
    }

    if (connect(socket_fd, (struct sockaddr*)&server_addr,
                sizeof(struct sockaddr)) < 0) {
        ERR("Connection error.");
        exit(1);
    }

    char c;
    while ((c = getchar()) != EOF)
        write(socket_fd, &c, 1);

    /* Need to send EOF so it knows when to stop */
    write(socket_fd, &c, 1);
    close(socket_fd);
}

int main(int argc, char** argv) {
    EMode mode = get_mode(argc, argv);
    if (mode == MODE_ERR) {
        fprintf(stderr,
                "Usage:\n"
                "    %s h       - Show this help\n"
                "    %s l       - Start in listen mode\n"
                "    %s c <IP>  - Connect to specified IP address\n",
                argv[0], argv[0], argv[0]);
        return 1;
    }

    switch (mode) {
        case MODE_LISTEN: /* snc l */
            snc_listen();
            break;

        case MODE_CONNECT: /* snc c IP */
            snc_connect(unalias_ip(argv[2]));
            break;

        default:
            ERR("Fatal: Unhandled mode.");
            exit(1);
    }

    return 0;
}
