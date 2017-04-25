#include <time.h>
#include <fstream>
#include <iomanip>
#include "remote_config_log.hh"

RemoteConfigLogger::RemoteConfigLogger(string log_path):
    _log_path(log_path)
{
}

RemoteConfigLogger::~RemoteConfigLogger()
{
}

void RemoteConfigLogger::log(string line)
{
    time_t now = time(NULL);
    char datetime[64] = "";
    strftime(datetime, sizeof(datetime), "%Y-%m-%d %H:%M:%S", localtime(&now));
    string time_str = string(datetime);
    ofstream ofs(_log_path.c_str(), ios::app);
    ofs << time_str << "\t" << line << endl;
    ofs.close();
}
