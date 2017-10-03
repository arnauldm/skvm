#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#include "util.h"
#include "serial.h"

int serial_sock, cli_sock;

void sigpipe_handler (int signo)
{
    close (cli_sock);
    cli_sock = serial_wait ();
    if (cli_sock == -1)
        pexit ("serial_wait");
}


void init_serial (char *serial_file)
{
    unlink (serial_file);
    serial_sock = create_serial (serial_file);
    if (serial_sock < 0)
        pexit ("create_serial()");

    cli_sock = serial_wait ();
    if (cli_sock == -1)
        pexit ("serial_wait");

    if (signal (SIGPIPE, sigpipe_handler) == SIG_ERR)
        pexit ("signal");
}


int create_serial (char *serial_file)
{
    int sock, ret;
    struct sockaddr_un endpoint;

    sock = socket (AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0)
        pexit ("socket");

    endpoint.sun_family = AF_UNIX;
    strcpy (endpoint.sun_path, serial_file);

    ret =
        bind (sock, (struct sockaddr *) &endpoint,
              sizeof (struct sockaddr_un));
    if (ret < 0)
        pexit ("bind");

    ret = listen (sock, 5);
    if (ret < 0)
        pexit ("listen");

    return sock;
}


int serial_wait (void)
{
    return accept (serial_sock, 0, 0);
}


ssize_t serial_in (char *buf, size_t len)
{
    return read (cli_sock, buf, len);
}


ssize_t serial_out (char *buf, size_t len)
{
    return write (cli_sock, buf, len);
}
