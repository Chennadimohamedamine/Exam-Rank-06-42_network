#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>

typedef struct
{
    int  id;
    char msg[1000000];
} Client;

static Client  clients[1024];
static int     max_fd  = 0;
static int     next_id = 0;
static char    send_buf[10000000];
static char    recv_buf[10000000];
static fd_set  master_fds;
static fd_set  read_fds;
static fd_set  write_fds;

static void fatal(const char *msg)
{
    write(STDERR_FILENO, msg, strlen(msg));
    exit(EXIT_FAILURE);
}

static void broadcast(int sender_fd)
{
    size_t len = strlen(send_buf);

    for (int fd = 0; fd <= max_fd; fd++)
        if (fd != sender_fd && FD_ISSET(fd, &write_fds))
            send(fd, send_buf, len, 0);
}

static void handle_new_connection(int server_fd)
{
    int client_fd = accept(server_fd, NULL, NULL);
    if (client_fd < 0)
        return;

    if (client_fd > max_fd)
        max_fd = client_fd;

    clients[client_fd].id = next_id++;
    memset(clients[client_fd].msg, 0, sizeof(clients[client_fd].msg));
    FD_SET(client_fd, &master_fds);

    sprintf(send_buf, "server: client %d just arrived\n", clients[client_fd].id);
    broadcast(client_fd);
}

static void handle_disconnect(int fd)
{
    sprintf(send_buf, "server: client %d just left\n", clients[fd].id);
    broadcast(fd);
    FD_CLR(fd, &master_fds);
    close(fd);
}

static void handle_incoming_data(int fd)
{
    int bytes = recv(fd, recv_buf, sizeof(recv_buf) - 1, 0);

    if (bytes <= 0)
    {
        handle_disconnect(fd);
        return;
    }

    recv_buf[bytes] = '\0';

    int j = strlen(clients[fd].msg);
    for (int i = 0; i < bytes && (size_t)j < sizeof(clients[fd].msg) - 1; i++, j++)
    {
        clients[fd].msg[j] = recv_buf[i];
        if (clients[fd].msg[j] == '\n')
        {
            clients[fd].msg[j] = '\0';
            sprintf(send_buf, "client %d: %s\n", clients[fd].id, clients[fd].msg);
            broadcast(fd);
            memset(clients[fd].msg, 0, sizeof(clients[fd].msg));
            j = -1;
        }
    }
}

int main(int argc, char **argv)
{
    if (argc != 2)
        fatal("Wrong number of arguments\n");

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1)
        fatal("Fatal error\n");

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = htonl(2130706433); // 127.0.0.1
    addr.sin_port        = htons(atoi(argv[1]));

    if (bind(server_fd, (const struct sockaddr *)&addr, sizeof(addr)) != 0)
        fatal("Fatal error\n");

    if (listen(server_fd, 10) != 0)
        fatal("Fatal error\n");

    max_fd = server_fd;
    FD_ZERO(&master_fds);
    FD_SET(server_fd, &master_fds);

    while (1)
    {
        read_fds  = master_fds;
        write_fds = master_fds;

        if (select(max_fd + 1, &read_fds, &write_fds, NULL, NULL) < 0)
            continue;

        for (int fd = 0; fd <= max_fd; fd++)
        {
            if (!FD_ISSET(fd, &read_fds))
                continue;

            if (fd == server_fd)
            {
                handle_new_connection(server_fd);
                break;
            }
            else
            {
                handle_incoming_data(fd);
                break;
            }
        }
    }
}