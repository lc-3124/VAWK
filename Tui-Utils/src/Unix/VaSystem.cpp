/*
 * (c) 2025 Lc3124 
 * License (MIT)
 * VaTui::System的实现 
 */

#ifndef _VASYSTEM_CPP_
#define _VASYSTEM_CPP_

//Va
#include "VaTui.hpp"
//std
#include <iostream>
#include <ctime>
#include <cstdlib>
//unix
#include <unistd.h>
#include <sys/utsname.h>
#include <pwd.h>

std::string VaTui::System::getUserName() {
    uid_t uid = getuid();
    struct passwd *pw = getpwuid(uid);
    if (pw!= nullptr) {
        return std::string(pw->pw_name);
    }
    return "";

}

// 获取当前时间
std::string VaTui::System::getCurrentTime() {
    time_t now = time(nullptr);
    char buffer[80];
    struct tm *timeinfo = localtime(&now);
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
    return std::string(buffer);

}

//获取环境变量
std::string VaTui::System::getRunningEnvironment(const char *index) {
    const char *env = std::getenv(index);
    std::string result = "";
    if (env!= nullptr) {
        result += env;
    }
    return result;

}

// 获取设备名称（这里通过utsname结构体获取系统信息中的机器名部分来近似表示设备名称）
std::string VaTui::System::getDeviceName() {
    struct utsname uts;
    if (uname(&uts)!= -1) {
        return std::string(uts.machine);
    }
    return "";

}

// 获取主机名
std::string VaTui::System::getHostName() {
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname))!= -1) {
        return std::string(hostname);
    }
    return "";
}

// 获取运行目录
std::string VaTui::System::getRunningDirectory() {
    char buffer[256];
    if (getcwd(buffer, sizeof(buffer))!= nullptr) {
        return std::string(buffer);
    }
    return "";

}

std::string VaTui::System::getSystemOuput(const char * cmd) {
    FILE* pipe = popen(cmd, "r");
    if (!pipe) {
        std::cerr << "popen failed!" << std::endl;
        return "";
    }

    char buffer[128];
    std::string result = "";
    while (!feof(pipe)) {
        if (fgets(buffer, 128, pipe)!= nullptr) {
            result += buffer;
        }
    }

    pclose(pipe);

    return result;
}
#endif
