#include <cstring>
#include <string>
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <cassert>
#include "remote_config_client.hh"
using namespace std;

RemoteConfigClient::RemoteConfigClient(string& username,
		string& password):
        _session(NULL),
        _channel(NULL),
	_username(username),
	_password(password)
{
    string log_path = "./remote_config.log";
    _rclogger = new RemoteConfigLogger(log_path);
}

RemoteConfigClient::~RemoteConfigClient()
{
    if(_channel)
    {
        ssh_channel_free(_channel);
    }
    if(_session)
    {
        ssh_free(_session);
    }
    if(_rclogger)
    {
        delete _rclogger;
    }
}

int
RemoteConfigClient::start_connection(string& remote_config_addr, int remote_config_port)
{
    assert(!_session);
    _session = ssh_new();
    if(!_session)
    {
        return -1;
    }
    // set HOST, PORT and USER for ssh connection
    ssh_options_set(_session, SSH_OPTIONS_HOST, remote_config_addr.c_str());
    ssh_options_set(_session, SSH_OPTIONS_PORT, &remote_config_port);
    ssh_options_set(_session, SSH_OPTIONS_USER, _username.c_str());
    int ret = SSH_ERROR;
    // start ssh connection
    ret = ssh_connect(_session);
    if(ret != SSH_OK)
    {
        ssh_free(_session);
        _session = NULL;
        return -1;
    } 
    return 0;
}

int
RemoteConfigClient::verify_knownserver()
{
    assert(_session);
    int state;
    unsigned char *hash = NULL;
    size_t hlen;
    char *hexa;
    char buf[10];
    int ret = SSH_ERROR;
    ssh_key pubkey = NULL; 
    ret = ssh_get_publickey(_session, &pubkey);
    if(ret != SSH_OK || !pubkey)
    {
        return -1;
    }
    ret = ssh_get_publickey_hash(pubkey, SSH_PUBLICKEY_HASH_MD5, &hash, &hlen);
    if(ret != SSH_OK)
    {
        ssh_key_free(pubkey);
        return -1;   
    }
    state = ssh_is_server_known(_session);
    switch (state)
    {
        case SSH_SERVER_KNOWN_OK:
            break; /* ok */
        case SSH_SERVER_KNOWN_CHANGED:
            fprintf(stderr, "Host key for server changed: it is now:\n");
            ssh_print_hexa("Public key hash", hash, hlen);
            fprintf(stderr, "For security reasons, connection will be stopped\n");
            ssh_key_free(pubkey);
            free(hash);
            return -1;
        case SSH_SERVER_FOUND_OTHER:
            fprintf(stderr, "The host key for this server was not found but an other"
                    "type of key exists.\n");
            fprintf(stderr, "An attacker might change the default server key to"
                    "confuse your client into thinking the key does not exist\n");
            ssh_key_free(pubkey);
            free(hash);
            return -1;
         case SSH_SERVER_FILE_NOT_FOUND:
             fprintf(stderr, "Could not find known host file.\n");
             fprintf(stderr, "If you accept the host key here, the file will be"
                     "automatically created.\n");
         /* fallback to SSH_SERVER_NOT_KNOWN behavior */
         case SSH_SERVER_NOT_KNOWN:
             hexa = ssh_get_hexa(hash, hlen);
             fprintf(stderr,"The server is unknown. Do you trust the host key?\n");
             fprintf(stderr, "Public key hash: %s\n", hexa);
             free(hexa);
             if (fgets(buf, sizeof(buf), stdin) == NULL)
             {
                 ssh_key_free(pubkey);
                 free(hash);
                 return -1;
             }
             if (strncasecmp(buf, "yes", 3) != 0)
             {
                 ssh_key_free(pubkey);
                 free(hash);
                 return -1;
             }
             if (ssh_write_knownhost(_session) < 0)
             {
                 fprintf(stderr, "Error %s\n", strerror(errno));
                 ssh_key_free(pubkey);
                 free(hash);
                 return -1;
             }
             break;
         case SSH_SERVER_ERROR:
             fprintf(stderr, "Error %s", ssh_get_error(_session));
             ssh_key_free(pubkey);
             free(hash);
             return -1;
    }
    ssh_key_free(pubkey);
    free(hash);
    return 0;
}

int
RemoteConfigClient::authenticate()
{
    assert(_session);
    if(verify_knownserver() < 0)
    {
        return -1;
    }
    int ret = SSH_AUTH_ERROR;

    ret = ssh_userauth_password(_session, NULL, _password.c_str());
    if (ret != SSH_AUTH_SUCCESS) {
        return -1;
    }
    char* banner = ssh_get_issue_banner(_session);
    if (banner) {
        printf("%s\n",banner);
        ssh_string_free_char(banner);
    }
    return 0;      
}

int
RemoteConfigClient::request_remote_config()
{
    assert(_session);
    _channel = ssh_channel_new(_session);
    if(!_channel)
    {
        return -1;
    }
    int ret = SSH_ERROR;
    ret = ssh_channel_open_session(_channel);
    if(ret != SSH_OK)
    {
        ssh_channel_free(_channel);
        _channel = NULL;
        return -1;
    }
    ret = ssh_channel_request_subsystem(_channel, "remote_config");
    if(ret != SSH_OK)
    {
        ssh_channel_free(_channel);
        _channel = NULL;
        return -1;
    }
    return 0;
}

bool need_outputting(string command_name, bool is_command_success, int mode)
{
    if(!is_command_success)
    {
        return true;
    }
    else if(mode < 2
            && command_name.substr(0, sizeof("configure") - 1) != "configure")
    {
        return true;
    }
    else if(command_name.substr(0, sizeof("configure") - 1) == "configure"
            || command_name.substr(0, sizeof("edit") - 1) == "edit"
            || command_name.substr(0, sizeof("up") - 1) == "up"
            || command_name.substr(0, sizeof("top") - 1) == "top"
            || command_name.substr(0, sizeof("set") - 1) == "set"
            || command_name.substr(0, sizeof("delete") - 1) == "delete"
            || command_name.substr(0, sizeof("exit") - 1) == "exit")
    {
        return false;
    }
    return true;
}

bool need_logging(string command_name, int mode)
{
    if(mode >= 2
       &&
       (command_name.substr(0, sizeof("set") - 1) == "set"
        || command_name.substr(0, sizeof("delete") - 1) == "delete"
        || command_name.substr(0, sizeof("commit") - 1) == "commit"))
    {
        return true;
    }
    return false;
}

int
RemoteConfigClient::send_config_command(string& cmd)
{
    char buf[4096] = "";
    size_t buf_size = sizeof(buf);
    size_t cmd_len = cmd.size();
    if(cmd_len > buf_size)
    {
        fprintf(stderr, "Command is too long!\n");
        return -1;
    }
    // send command to server end
    memset(buf, 0, buf_size);
    strncpy(buf, cmd.c_str(), cmd_len);
    if(ssh_channel_write(_channel, buf, cmd_len) == SSH_ERROR)
    {
        fprintf(stderr, "Send command to remote switch failed.\n");
        return -1;
    }

    int n = 0;
    bool got_first_part = false;
    int ret = SSH_ERROR;
    ssh_channel channels[2];
    struct timeval select_timeout;
    select_timeout.tv_sec = 0;
    select_timeout.tv_usec = 200000;
    struct timeval* time_val_ptr = NULL;
    int is_command_success = 0;
    int mode = 0;
    while(1)
    {
        if(got_first_part)
        {
            time_val_ptr = &select_timeout;
        }
        else
        {
            time_val_ptr = NULL;
        }
        channels[0]=_channel;
        channels[1]=NULL;
        ret = ssh_channel_select(channels, NULL, NULL, time_val_ptr);
        if(ret == SSH_OK)
        {
            int len = 0;
            ssh_channel_read(_channel, &len, sizeof(int), 0);
            ssh_channel_read(_channel, &is_command_success, sizeof(int), 0);
            ssh_channel_read(_channel, &mode, sizeof(int), 0);
            len = ntohl(len);
            is_command_success = ntohl(is_command_success);
            mode = ntohl(mode);
            if(!is_command_success && !got_first_part)
            {
                printf(">> %s", cmd.c_str());
            }
            if(!got_first_part)
            {
                got_first_part = true;
            }
            int msg_len = len - 3 * sizeof(int); 
            if(msg_len < 0)
            {
                fprintf(stderr, "Invalid message length!\n");
                return -1;
            }
            size_t bytes_received = 0;
            while (bytes_received < msg_len)
            {
                int bytes_left = msg_len - bytes_received;
                int bytes_to_read = bytes_left < buf_size - 1  ? bytes_left : buf_size - 1;
                memset(buf, 0, buf_size);
            
                n = ssh_channel_read(_channel, buf, bytes_to_read, 0);
                bytes_received += n;
             
                if(n > 0)
                {
                    if(need_outputting(cmd, is_command_success, mode)) 
                    {
                        printf("%s", buf);
                    }
                }
                else if(n == 0)
                {
                    break;
                }
            }
        }
        else if(ret == SSH_EINTR)
        {
            continue;
        }
        // select timeouts
        else if(ret == SSH_AGAIN)
        {
            break;
        }
        else
        {
            fprintf(stderr, "Read error!\n");
            return -1;
        }
    }   
    if(need_logging(cmd, mode))
    {
        string line = _username + "\t" + cmd.substr(0, cmd.size() - 1) + "\t";
        if(is_command_success) 
        {
            line += "0";
        }
        else
        {
            line += "-1";
        }
        _rclogger->log(line);
    }
    if(!is_command_success)
    {
        return -1;
    }
    return 0;
}

int
RemoteConfigClient::send_commands_from_string(string& cmd_str)
{
    string command;
    size_t pos = 0;
    size_t current = 0;
    size_t len = 0;
    pos = cmd_str.find(';', current);
    
    while(pos != string::npos)
    {
        if(pos <= current)
        {
            break;
        }
        len = pos - current;
        command = cmd_str.substr(current, len);
        command += "\n";
        send_config_command(command);
        current = pos + 1;
        pos = cmd_str.find(';', current);
    }
    if(current < cmd_str.length())
    {
        command = cmd_str.substr(current);
        command += "\n";
        send_config_command(command);
    }
    return 0;
}

int
RemoteConfigClient::send_commands_from_file(string& cmd_file)
{
    string command;
    ifstream ifcmd;
    ifcmd.open(cmd_file.c_str(), ifstream::in);
    if(!ifcmd.good())
    {
        fprintf(stderr, "File '%s' does not exist.\n", cmd_file.c_str());
    }
    while(ifcmd.good())
    {
        getline(ifcmd, command);
        if(command.empty() || command[0] == '#')
        {
            continue;
        }
        command += "\n";
        send_config_command(command);
    }
    ifcmd.close();
    return 0;
}

int
RemoteConfigClient::close_connection()
{
    if(_channel)
    {
        ssh_channel_close(_channel);
        ssh_channel_send_eof(_channel);
        ssh_channel_free(_channel);
        _channel = NULL;
    }
    if(_session)
    {
        ssh_disconnect(_session);
        ssh_free(_session);
        _session = NULL;
    }
    return 0;
}
