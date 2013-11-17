#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <ulcd43.h>
#include "config.h"
#include "ulcd.h"

#define BUFSIZE 1024

struct ulcd_t *ulcd;

int quit = 0;


void
sig_interrupt(int signal)
{
    if (signal == SIGINT) {
        fprintf(stderr, "Caught SIGINT, shutting down.\n");
        quit = 1;
    }
}

void
print_syntax(void)
{
    fprintf(stderr, "Usage: %s [options] command [parameters]\n", PACKAGE);
    fprintf(stderr, "Options:\n"
        "  -d device    Serial device\n"
        "  -b rate      Serial baud rate\n"
        );
    fprintf(stderr, "Commands:\n"
        "  clear\n"
        "    Clear the screen and reset parameters\n"
        );
    fprintf(stderr, "Interactive commands:\n"
        "  drawtest\n"
        "    Start an interactive drawing program.\n"
        );
}

int
main(int argc, char** argv)
{
    const char *cmd;
    int retval = 0;
    int c;

    signal(SIGINT, sig_interrupt);

    ulcd = ulcd_new();
    ulcd->device = getenv("ULCD_DEVICE");
    ulcd->baudrate = atol(getenv("ULCD_BAUDRATE"));

    while ((c = getopt(argc, argv, "d:b:")) != -1) {
        switch(c) {
            case 'd':
                ulcd->device = malloc(BUFSIZE);
                strncpy(ulcd->device, optarg, BUFSIZE);
                break;
            case '?':
                if (optopt == 'c') {
                    fprintf(stderr, "Option -%c requires an argument.\n", optopt);
                } else if (isprint(optopt)) {
                    fprintf(stderr, "Unknown option `-%c'.\n", optopt);
                } else {
                    fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
                }
                break;
            default:
                abort();
        }
    }

    if (optind >= argc) {
        fprintf(stderr, "%s: must specify a command.\n", PACKAGE);
        print_syntax();
        ulcd_free(ulcd);
        return EXIT_FAILURE;
    }

    if (ulcd_open_serial_device(ulcd)) {
        fprintf(stderr, "Could not open serial device %s", ulcd->device);
        ulcd_free(ulcd);
        return EXIT_FAILURE;
    }
    ulcd_set_serial_parameters(ulcd);

    cmd = argv[optind];
    if (!(strcmp(cmd, "clear"))) {
        retval = ulcd_gfx_cls(ulcd);
    } else if (!(strcmp(cmd, "drawtest"))) {
        retval = interactive_draw_test(ulcd);
    }

    ulcd_free(ulcd);

    if (retval != 0) {
        fprintf(stderr, "%s failed: error", cmd);
    }

    return retval;
}
