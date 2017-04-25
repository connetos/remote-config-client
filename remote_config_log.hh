#ifndef __REMOTE_CONFIG_LOG_HH__
#define __REMOTE_CONFIG_LOG_HH__

#include <string>
using namespace std;

class RemoteConfigLogger
{
public:
    RemoteConfigLogger(string log_path);
    ~RemoteConfigLogger();
    void log(string line);
private:
    string _log_path;
};
#endif
