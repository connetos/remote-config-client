#ifndef __REMOTE_CLI_CLIENT_HH__
#define __REMOTE_CLI_CLIENT_HH__

#include <string>
#include <libssh/libssh.h>
#include "remote_config_log.hh"

class RemoteConfigClient
{ 
public:
	RemoteConfigClient(string& username, string& password);
	~RemoteConfigClient();

	int start_connection(string& remote_cli_addr, int remote_cli_port);
	int authenticate();
        int request_remote_config();
        int send_commands_from_string(string& cmd_str);
        int send_commands_from_file(string& cmd_file);
        int send_config_command(string& cmd);
        int close_connection();
private:
        int verify_knownserver();
        ssh_session _session; 
        ssh_channel _channel;
	string& _username;
	string&	_password;
        RemoteConfigLogger* _rclogger;
};

#endif

