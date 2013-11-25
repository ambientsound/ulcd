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

int verbose = 0;
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
        "  -v           Verbose operation.\n"
        "  -h           This help screen.\n"
        );
    fprintf(stderr, "System commands:\n"
        "  reset\n"
        "    Try to reset the device to a sane state.\n"
        );
    fprintf(stderr, "Serial commands:\n"
        "  baudrate <110|300|600|1200|2400|4800|9600|19200|38400|57600|115200|500000>\n"
        "    Set target baud rate.\n"
        );
    fprintf(stderr, "System commands:\n"
        "  version\n"
        "    Check device model, SPE version and PmmC version.\n"
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
    fprintf(stderr, "Text commands:\n"
        "  textreset\n"
        "    Sets the text parameters to sane default values.\n"
        "  write <string>\n"
        "    Writes the specified string to the display.\n"
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
exec_cmd(int argc, char **argv)
{
    char line[1024];
    const char *cmd;
    int r = ERROK;
    param_t i;
    long l;

    cmd = argv[0];

    if (!(strcmp(cmd, "reset"))) {
        r = ulcd_reset(ulcd);
        fprintf(stderr, "%s\n", ulcd->err);

    } else if (!(strcmp(cmd, "clear"))) {
        return ulcd_gfx_cls(ulcd);

    } else if (!(strcmp(cmd, "on"))) {
        return ulcd_display_on(ulcd);

    } else if (!(strcmp(cmd, "off"))) {
        return ulcd_display_off(ulcd);

    } else if (!(strcmp(cmd, "contrast"))) {
        if (argc > 1) {
            i = atoi(argv[1]);
            if (i >= 0 && i <= 15) {
                return ulcd_gfx_contrast(ulcd, i);
            }
        }
        return ulcd_error(ulcd, EXIT_FAILURE, "needs an integer value between 0 and 15.");

    } else if (!(strcmp(cmd, "baudrate"))) {
        if (argc > 1) {
            return ulcd_set_baud_rate(ulcd, atol(argv[1]));
        }
        return ulcd_error(ulcd, EXIT_FAILURE, "needs an integer value.");

    } else if (!(strcmp(cmd, "textreset"))) {
        return ulcd_txt_reset(ulcd);

    } else if (!(strcmp(cmd, "write"))) {
        strncpy(line, argv[1], 1022);
        l = strlen(line);
        line[l] = '\n';
        line[l+1] = '\0';
        return ulcd_txt_putstr(ulcd, line, NULL);

    } else if (!(strcmp(cmd, "version"))) {
        if (!ulcd_get_info(ulcd)) {
            printf("%s SPE=%d PmmC=%d\n", ulcd->model, ulcd->spe_version, ulcd->pmmc_version);
        }
        return ulcd->error;

    } else if (!(strcmp(cmd, "drawtest"))) {
        signal(SIGINT, sig_interrupt);
        return interactive_draw_test(ulcd);

    } else {
        return ulcd_error(ulcd, EXIT_FAILURE, "unrecogzined command.");
    }

    return r;
}

int
main(int argc, char** argv)
{
    char *param;
    int retval = 0;
    int c;

    ulcd = ulcd_new();
    if ((param = getenv("ULCD_BAUD_RATE")) != NULL) {
        if (ulcd_set_baud_rate(ulcd, atol(param))) {
            fprintf(stderr, "Error setting initial baud rate: %s\n", ulcd->err);
        }
    }
    if ((param = getenv("ULCD_DEVICE")) != NULL) {
        strncpy(ulcd->device, param, STRBUFSIZE);
    }

    while ((c = getopt(argc, argv, "d:b:vh")) != -1) {
        switch(c) {
            case 'h':
                print_syntax();
                return EXIT_SUCCESS;
            case 'v':
                verbose = 1;
                break;
            case 'd':
                strncpy(ulcd->device, optarg, BUFSIZE);
                break;
            case 'b':
                if (ulcd_set_baud_rate(ulcd, atol(optarg))) {
                    fprintf(stderr, "Error setting initial baud rate: %s\n", ulcd->err);
                }
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

    if (ulcd_open_serial_device(ulcd)) {
        fprintf(stderr, "%s\n", ulcd->error, ulcd->err);
        ulcd_free(ulcd);
        return ulcd->error;
    }

    ulcd_set_serial_parameters(ulcd);

    if (argc-optind == 0) {
        fprintf(stderr, "%s: must specify a command.\n", PACKAGE);
        print_syntax();
        ulcd_free(ulcd);
        return EXIT_FAILURE;
    }

    retval = exec_cmd(argc-optind, argv+optind);

    if (retval != ERROK) {
        fprintf(stderr, "Command `%s' failed: %s\n", argv[optind], ulcd->err);
    }

    ulcd_free(ulcd);

    return retval;
}
