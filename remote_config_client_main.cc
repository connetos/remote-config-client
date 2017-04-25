#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <getopt.h>
#include <cstring>
#include <string>
using namespace std;
#include "remote_config_client.hh"

/**
 * usage:
 * @argv0: Argument 0 when the program was called (the program name itself).
 * @exit_value: The exit value of the program.
 *
 * Print the program usage.
 * If @exit_value is 0, the usage will be printed to the standard output,
 * otherwise to the standard error.
 **/
static void
usage(const char *argv0, int exit_value) {
    FILE *output;
    const char *progname = strrchr(argv0, '/');

    if (progname != NULL)
        progname++;             // Skip the last '/'
    if (progname == NULL)
        progname = argv0;

    if (exit_value == 0)
        output = stdout;
    else
        output = stderr;

    fprintf(output, "Usage: %s -s < device addr > -u < username> -p < password > [-c < command >] [-f < commands file >] [-h]\n", progname);
    fprintf(output, "\n");
    fprintf(output, "           -s < device addr >            : device addr\n");
    fprintf(output, "           -u < username >               : the user name to login in\n");
    fprintf(output, "           -p < password >               : the password for the given user\n");
    fprintf(output, "           -c < commands >               : the command string containing multiple commands to be executed\n");
    fprintf(output, "           -f < commands file >          : the file name of the excecuting commands\n");
    fprintf(output, "           -h                            : usage (this message)\n");
    fprintf(output, "\n");
    exit (exit_value);

}

int main(int argc, char**argv){
    int             ch;
    string          remote_config_addr;
    uint16_t        remote_config_port = 22;
    string          username;
    string          password;
    string          cmd_file;
    string          cmd_str;

    if (argc == 1) {
        usage(argv[0], 0);
    }

    while ((ch = getopt(argc, argv, "s:c:f:u:p:h")) != -1) {
        switch (ch) {
        case 's':
            remote_config_addr = optarg;
            break;
        case 'f':
            cmd_file = optarg;
            break;
        case 'c':
            cmd_str = optarg;
            break;
        case 'u':
            username = optarg;
            break;
        case 'p':
            password = optarg;
            break;
        case 'h':
            usage(argv[0], 0);
            break;
        default:
            usage(argv[0], 1);
            break;
        }
    }
    if(remote_config_addr.empty())
    {
        printf("<device addr> must be specified using '-s' option.\n");
        return -1; 
    }
    RemoteConfigClient remote_config_client(username, password);
    int ret = -1;
    ret = remote_config_client.start_connection(remote_config_addr, remote_config_port);
    if(ret < 0)
    {
        printf("Start connection failed.\n");
        return -1;
    }
    ret = remote_config_client.authenticate();
    if(ret < 0)
    {
        printf("Authentication failed.\n");
        return -1;
    }
    ret = remote_config_client.request_remote_config();
    if(ret < 0)
    {
        printf("Request remote config failed.\n");
        return -1;
    }
    if (cmd_str.length() != 0)
    {
        remote_config_client.send_commands_from_string(cmd_str);
    }
    else
    {
        remote_config_client.send_commands_from_file(cmd_file);
    }
    remote_config_client.close_connection();
    return 0;
}

