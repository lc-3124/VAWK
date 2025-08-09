/*
 * (c) 2025 Lc3124
 * License (MIT)
 * VaTui::Utf的实现
 */

#ifndef _VAUTF_CPP_
#define _VAUTF_CPP_

#include "VaTui.hpp"


//get the width of a UTF character 
size_t VaTui::Utf::getUtf8CharWidth(const char* s) {
    if (!s ||!*s) return 0;
    unsigned char c = static_cast<unsigned char>(*s);
    if (c < 0x80) return 1;
    else if ((c & 0xE0) == 0xC0) return 2;
    else if ((c & 0xF0) == 0xE0) return 3;
    else if ((c & 0xF8) == 0xF0) return 4;
    else return 0;

}

// recognize a ASCII character 
bool VaTui::Utf::isAscii(char c) {
    return (c & 0x80) == 0;

}

// 判断是否为 UTF-8 多字节序列的开头字节
bool VaTui::Utf::isUtf8StartByte(char c) {
    // 将 char 类型转换为 unsigned char 类型，避免符号位影响按位与操作
    unsigned char uc = static_cast<unsigned char>(c);
    // UTF-8 多字节序列的后续字节以 10 开头，所以只要不是以 10 开头就是开头字节
    return (uc & 0xC0) != 0x80;
}


// 判断是否为 UTF-8 编码字符
bool VaTui::Utf::isUtf8Char(const char* bytes, int len) {
    if (len <= 0) return false;
    char firstByte = bytes[0];
    if (isAscii(firstByte)) return true;
    if (isUtf8StartByte(firstByte)) {
        int numBytes;
        if ((firstByte & 0xE0) == 0xC0) numBytes = 2;
        else if ((firstByte & 0xF0) == 0xE0) numBytes = 3;
        else if ((firstByte & 0xF8) == 0xF0) numBytes = 4;
        else return false;
        return len == numBytes;

    }
    return false;

}

// 判断是否为 GBK 编码字符  
bool VaTui::Utf::isGbkChar(const char* bytes, int len) {
    if (len!= 2) return false;
    unsigned char firstByte = static_cast<unsigned char>(bytes[0]);
    unsigned char secondByte = static_cast<unsigned char>(bytes[1]);
    return ((firstByte >= 0x81 && firstByte <= 0xFE) && (secondByte >= 0x40 && secondByte <= 0xFE && secondByte!= 0x7F));

}

int VaTui::Utf::getUtf8ByteCount(char c) {
    unsigned char uc = static_cast<unsigned char>(c);
    if ((uc & 0x80) == 0) {
        // 单字节字符 (0xxxxxxx)
        return 1;
    } else if ((uc & 0xE0) == 0xC0) {
        // 双字节字符 (110xxxxx)
        return 2;
    } else if ((uc & 0xF0) == 0xE0) {
        // 三字节字符 (1110xxxx)
        return 3;
    } else if ((uc & 0xF8) == 0xF0) {
        // 四字节字符 (11110xxx)
        return 4;
    }
    // 无效的 UTF-8 起始字节
    return -1; 
}

// 懒得写注释了，这段代码没有问题
bool VaTui::Utf::getUtf8CharaInString( std::string resource , std::string &save , int index )
{
    save = "";
    int i = 0;
    // NOTICE! 这里utf_cnt 从-1开始，但实际上index是从0开始索引的，这是个魔法数字 
    int utf_cnt = -1; 
    while (i < resource.size())
    {
        if (VaTui::Utf::isUtf8StartByte(resource[i]))
        {
            utf_cnt++;
            if (utf_cnt == index)
            {
                int byteCount = VaTui::Utf::getUtf8ByteCount(resource[i]);
                if (i + byteCount > resource.size()) {
                    return false;
                }
                for (int j = 0; j < byteCount; j++)
                {
                    save.push_back(resource[i + j]);
                }
                return true;
            }
            i += VaTui::Utf::getUtf8ByteCount(resource[i]);
        }
        else
        {
            i++;
        }
    }
    return false;
}

int VaTui::Utf::getUtf8StringLen( std::string resource )
{
    int i=0 , cnt=0 ;
    while( resource.size() != i )
    {
        if(VaTui::Utf::isUtf8StartByte(resource.at(i)))
        {
            cnt ++ ;
        }
        i++;
    }
    return cnt;
}


#endif
