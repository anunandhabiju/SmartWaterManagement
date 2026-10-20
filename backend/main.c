/**
 * @file main.c
 * @brief Entry point for the Smart Water Distribution and Management System
 */

#include "server.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

static void handle_signal(int sig) {
    (void)sig;
    printf("\nGraceful shutdown signal received...\n");
    server_stop();
}

int main(int argc, char *argv[]) {
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    const char *port = DEFAULT_PORT;
    if (argc >= 2) {
        port = argv[1];
    }

    server_start(port);
    return 0;
}
