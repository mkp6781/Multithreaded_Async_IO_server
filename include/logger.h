#include <iostream>
#include <cstring>
#include <mutex>

class Log{
private:
    static std::mutex m_mutex;
public:
    static void LogInfo(const std::string &log_msg);
};