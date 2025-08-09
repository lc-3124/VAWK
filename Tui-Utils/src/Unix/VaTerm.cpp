#include "VaTui.hpp"

// std
#include <cstdio>
#include <cstring>
#include <iostream>
#include <stdexcept>
// sys
#include <fcntl.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

// 全局变量，保存原始终端属性和当前终端属性
termios originalAttrs;
termios currentAttrs;

// 辅助函数，用于安全地设置终端属性，添加异常处理机制
void
    VaTui::Term::setTerminalAttrsSafely ( const termios &newAttrs )
{
    if ( tcsetattr ( STDIN_FILENO, TCSANOW, &newAttrs ) != 0 )
        {
            throw std::runtime_error ( "Failed to set terminal attributes." );
        }
    currentAttrs = newAttrs;
}

// 辅助函数，用于安全地获取终端属性，添加异常处理机制
void
    VaTui::Term::getTerminalAttrsSafely ()
{
    if ( tcgetattr ( STDIN_FILENO, &currentAttrs ) != 0 )
        {
            throw std::runtime_error ( "Failed to get terminal attributes." );
        }
}

// 保存终端原始设置
void
    VaTui::Term::SaveTerm ()
{
    getTerminalAttrsSafely ();
    originalAttrs = currentAttrs;
}

// 恢复终端原始设置
void
    VaTui::Term::RestoreTerm ()
{
    setTerminalAttrsSafely ( originalAttrs );
    // system("reset");
}

// 清空整个屏幕
const char *
    VaTui::Term::_Clear ()
{
    return "\033[2J";
}
void
    VaTui::Term::Clear ()
{
    fastOutput ( "\033[2J" );
}

// 清空从光标位置到行尾的区域
const char *
    VaTui::Term::_ClearLine ()
{
    return "\033[K";
}
void
    VaTui::Term::ClearLine ()
{
    fastOutput ( "\033[K" );
}

// 获取终端属性
void
    VaTui::Term::getTerminalAttributes ()
{
    getTerminalAttrsSafely ();
}

// 设置终端属性
void
    VaTui::Term::setTerminalAttributes ( const termios &newAttrs )
{
    setTerminalAttrsSafely ( newAttrs );
}

// 启用终端回显
void
    VaTui::Term::enableEcho ()
{
    termios newAttrs = currentAttrs;
    newAttrs.c_lflag |= ECHO;
    setTerminalAttrsSafely ( newAttrs );
}

// 禁用终端回显
void
    VaTui::Term::disableEcho ()
{
    termios newAttrs = currentAttrs;
    newAttrs.c_lflag &= ~ECHO;
    setTerminalAttrsSafely ( newAttrs );
}

// 启用控制台缓冲
void
    VaTui::Term::enableConsoleBuffering ()
{
    int flags = fcntl ( STDIN_FILENO, F_GETFL );
    fcntl ( STDIN_FILENO, F_SETFL, flags & ~O_SYNC );
}

// 禁用控制台缓冲
void
    VaTui::Term::disableConsoleBuffering ()
{
    int flags = fcntl ( STDIN_FILENO, F_GETFL );
    fcntl ( STDIN_FILENO, F_SETFL, flags | O_SYNC );
}

// 获取终端大小
void
    VaTui::Term::getTerminalSize ( int &rows, int &cols )
{
    struct winsize w;
    if ( ioctl ( STDOUT_FILENO, TIOCGWINSZ, &w ) == 0 )
        {
            rows = w.ws_row;
            cols = w.ws_col;
        }
    else
        {
            throw std::runtime_error ( "Failed to get terminal size." );
        }
}

// 设置光标位置
void
    VaTui::Term::setCursorPosition ( int row, int col )
{
    std::cout << "\033[" << row << ";" << col << "H";
    std::cout.flush (); // 手动刷新输出缓冲区，确保光标位置及时更新显示
}

// 保存光标位置
void
    VaTui::Term::saveCursorPosition ()
{
    std::cout << "\033[s";
    std::cout.flush (); // 刷新输出缓冲区，保证保存操作生效
}

// 恢复光标位置
void
    VaTui::Term::restoreCursorPosition ()
{
    std::cout << "\033[u";
    std::cout.flush (); // 刷新输出缓冲区，保证恢复操作生效
}

// 快速输出内容到终端
void
    VaTui::Term::fastOutput ( const char *str )
{
    write ( STDOUT_FILENO, str, strlen ( str ) );
}

// 非缓冲获取按键，改进以避免影响标准输出刷新
char
    VaTui::Term::nonBufferedGetKey ()
{
    char           buf = 0;
    struct termios old = { 0 };
    if ( tcgetattr ( 0, &old ) < 0 ) perror ( "tcgetattr()" );
    old.c_lflag &= ~ICANON; // 非规范模式
    old.c_cc[ VMIN ]  = 1;  // 最少读取一个字符
    old.c_cc[ VTIME ] = 0;  // 无超时
    if ( tcsetattr ( 0, TCSANOW, &old ) < 0 ) perror ( "tcsetattr()" );
    if ( read ( 0, &buf, 1 ) < 0 ) perror ( "read" );
    // 恢复原始终端属性
    if ( tcsetattr ( 0, TCSANOW, &old ) < 0 ) perror ( "tcsetattr()" );
    return (int) buf;
}

// 获取终端类型
const char *
    VaTui::Term::getTerminalType ()
{
    return std::getenv ( "TERM" );
}

// 设置行缓冲模式
void
    VaTui::Term::setLineBuffering ( bool enable )
{
    termios newAttrs = currentAttrs;
    if ( enable )
        {
            newAttrs.c_lflag |= ICANON;
        }
    else
        {
            newAttrs.c_lflag &= ~ICANON;
        }
    setTerminalAttrsSafely ( newAttrs );
}

// 获取一个字符，类似getch，改进以确保回显正确处理
char
    VaTui::Term::getCharacter ()
{
    disableEcho ();
    char c = nonBufferedGetKey ();
    enableEcho ();
    return c;
}

// 判断终端是否支持某一功能
bool
    VaTui::Term::isTerminalFeatureSupported ( const char *feature )
{
    const char *termType = getTerminalType ();
    if ( termType == nullptr )
        {
            return false;
        }
    return ( strstr ( termType, feature ) != nullptr );
}

// 设置字符输入延迟
void
    VaTui::Term::setCharacterDelay ( int milliseconds )
{
    termios newAttrs       = currentAttrs;
    newAttrs.c_cc[ VMIN ]  = 0;
    newAttrs.c_cc[ VTIME ] = milliseconds / 100;
    setTerminalAttrsSafely ( newAttrs );
}

// 获取输入速度
int
    VaTui::Term::getInputSpeed ()
{
    getTerminalAttrsSafely ();
    return cfgetospeed ( &currentAttrs );
}

// 设置输入速度
void
    VaTui::Term::setInputSpeed ( int speed )
{
    termios newAttrs = currentAttrs;
    cfsetospeed ( &newAttrs, static_cast<speed_t> ( speed ) );
    cfsetispeed ( &newAttrs, static_cast<speed_t> ( speed ) );
    setTerminalAttrsSafely ( newAttrs );
}

// 设置输出速度
void
    VaTui::Term::setOutputSpeed ( int speed )
{
    termios newAttrs = currentAttrs;
    cfsetospeed ( &newAttrs, static_cast<speed_t> ( speed ) );
    setTerminalAttrsSafely ( newAttrs );
}

// 检测是否有按键按下
int
    VaTui::Term::getkeyPressed ( char &k )
{
    struct termios oldt, newt;
    int            oldf;
    getTerminalAttrsSafely ();
    oldt = currentAttrs;
    newt = oldt;
    newt.c_lflag &= ~( ICANON | ECHO );
    oldf = fcntl ( STDIN_FILENO, F_GETFL );
    fcntl ( STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK );
    setTerminalAttrsSafely ( newt );

    char c;
    int  res = read ( STDIN_FILENO, &c, 1 );
    if ( res > 0 )
        {
            k = c;
            setTerminalAttrsSafely ( oldt );
            fcntl ( STDIN_FILENO, F_SETFL, oldf );
            return 1;
        }
    else
        {
            setTerminalAttrsSafely ( oldt );
            fcntl ( STDIN_FILENO, F_SETFL, oldf );
            k = static_cast<char> ( -1 );
            return -1;
        }
}

// 设置光标形状
void
    VaTui::Term::setCursorShape ( CursorShape shape )
{
    termios newAttrs = currentAttrs;
    switch ( shape )
        {
        case CURSOR_BLOCK:
            newAttrs.c_cflag &= ~( ECHOCTL );
            break;
        case CURSOR_UNDERLINE:
            newAttrs.c_cflag |= ( ECHOCTL | ECHOE );
            break;
        case CURSOR_VERTICAL_BAR:
            newAttrs.c_cflag |= ECHOCTL;
            break;
        }
    setTerminalAttrsSafely ( newAttrs );
}

#include <iostream>
void
    VaTui::Term::FlushStdOut ()
{
    std::cout << fflush ( stdout );
}
