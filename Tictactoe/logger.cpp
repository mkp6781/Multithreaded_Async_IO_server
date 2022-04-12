#include <iostream>
#include <cstdio>
#include <fstream>
#include <logger.h>
#include "tictactoe.h"
#include <mutex>

std::mutex Log::m_mutex;
// std::string Log::filePath;
// std::ofstream Log::ofs;

// Log::Log(){
    // std::string filePath = "/logDir/log.txt";
    // std::ofstream ofs(filePath.c_str(), std::ios_base::out | std::ios_base::app);
// }

void Log::LogInfo(const std::string &logMsg){
    m_mutex.lock();
    std::string filePath = "game_server.log";
    std::ofstream ofs(filePath.c_str(), std::ios_base::out | std::ios_base::app);
    ofs<<logMsg<<"\n";
    ofs.close();
    m_mutex.unlock();
}