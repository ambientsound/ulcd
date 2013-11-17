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
        "  -d device    Specify serial device.\n"
        "  -b rate      Specify serial baud rate.\n"
        );
    fprintf(stderr, "System commands:\n"
        "  reset\n"
        "    Try to reset the device to a sane state.\n"
        );
    fprintf(stderr, "Serial commands:\n"
        "  baudrate <110|300|600|1200|2400|4800|9600|19200|38400|57600|115200|500000>\n"
        "    Set target baud rate.\n"
        );
    fprintf(stderr, "Graphics commands:\n"
        "  on\n"
        "    Turns the screen on with full contrast.\n"
        "  off\n"
        "    Turns the screen off.\n"
        "  contrast <0-15>\n"
        "    Sets the screen contrast. A value of 0 means off.\n"
        "  clear\n"
        "    Clear the screen and reset parameters.\n"
        );
    fprintf(stderr, "Interactive commands:\n"
        "  drawtest\n"
        "    Start an interactive drawing program.\n"
        );
}

char *
get_param(int argc, char **argv, int s)
{
    if (s + optind >= argc) {
        return NULL;
    }
    return argv[optind+s];
}

int
main(int argc, char** argv)
{
    char *param;
    const char *cmd;
    int verbose = 0;
    int retval = 0;
    int c;
    param_t i;
    long l;

    ulcd = ulcd_new();
    if ((param = getenv("ULCD_BAUD_RATE")) != NULL) {
        if (ulcd_set_baud_rate(ulcd, atol(param))) {
            fprintf(stderr, "Error setting initial baud rate: %s\n", ulcd->err);
        }
    }
    if ((param = getenv("ULCD_DEVICE")) != NULL) {
        strncpy(ulcd->device, param, STRBUFSIZE);
    }

    while ((c = getopt(argc, argv, "d:b:v")) != -1) {
        switch(c) {
            case 'v':
                verbose = 1;
                break;
            case 'd':
                strncpy(ulcd->device, optarg, BUFSIZE);
                break;
            case 'b':
                ulcd_set_baud_rate(ulcd, atol(optarg));
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

    if (verbose) {
        printf("Serial: %s\n", ulcd->device);
        printf("Baud: %d, const %d\n", ulcd->baud_rate, ulcd->baud_const);
    }

    if (optind >= argc) {
        fprintf(stderr, "%s: must specify a command.\n", PACKAGE);
        print_syntax();
        ulcd_free(ulcd);
        return EXIT_FAILURE;
    }

    if (ulcd_open_serial_device(ulcd)) {
        fprintf(stderr, "Error %d: %s\n", ulcd->error, ulcd->err);
        ulcd_free(ulcd);
        return EXIT_FAILURE;
    }
    ulcd_set_serial_parameters(ulcd);

    cmd = argv[optind];

    do {
        if (!(strcmp(cmd, "reset"))) {
            ulcd_reset(ulcd);
            fprintf(stderr, "%s\n", ulcd->err);
        } else if (!(strcmp(cmd, "clear"))) {
            retval = ulcd_gfx_cls(ulcd);
        } else if (!(strcmp(cmd, "on"))) {
            retval = ulcd_display_on(ulcd);
        } else if (!(strcmp(cmd, "off"))) {
            retval = ulcd_display_off(ulcd);
        } else if (!(strcmp(cmd, "contrast"))) {
            if (optind + 1 < argc) {
                i = atoi(argv[optind+1]);
                if (i >= 0 && i <= 15) {
                    retval = ulcd_gfx_contrast(ulcd, i);
                    break;
                }
            }
            fprintf(stderr, "Contrast needs an integer value between 0 and 15.\n");
            retval = 1;
        } else if (!(strcmp(cmd, "baudrate"))) {
            if (optind + 1 < argc) {
                l = atol(argv[optind+1]);
                retval = ulcd_set_baud_rate(ulcd, l);
            }
        } else if (!(strcmp(cmd, "drawtest"))) {
            signal(SIGINT, sig_interrupt);
            retval = interactive_draw_test(ulcd);
        } else {
            fprintf(stderr, "%s: command `%s' unrecognized.", PACKAGE, cmd);
            print_syntax();
            ulcd_free(ulcd);
            return EXIT_FAILURE;
        }
    }
    while (0);

    if (retval != ERROK) {
        fprintf(stderr, "Command `%s' failed: %s\n", cmd, ulcd->err);
    }

    ulcd_free(ulcd);

    return retval;
}
