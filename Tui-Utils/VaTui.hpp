/*
 * (c) 2024 Lc3124
 * License (MIT)
 *
 * 这个文件是VaTui的声明
 * VaTui包括5个子类，用于
 * 颜色操作，光标操作，终端控制，系统信息获取，utf字节识别和处理
 *
 * 可以看作是一个符号列表
 *
 */

/*
 * PS(29 Dec 24):
 * 我打算重新决定VaTui类的结构，于是现在现有的源代码都没法马上编译
 * Makefile也要重写
 * 于是我把win_src的分支删掉了
 *
 * 这个文件我将来会打上详细的英文注释
 *
 */

#ifndef _VATUI_FILE_H_
#define _VATUI_FILE_H_

#include "VaTuiEnums.hpp"
#include <string>

class VaTui
{
  public:
    class Color;
    class Cursor;
    class System;
    class Term;
    class Utf;
};

/*
 * VaColor class
 * VaColor 类是用于管理和操作终端文本颜色以及相关显示效果的核心类，
 * 它提供了一系列静态方法来方便地设置文本的颜色、效果等，
 * 并且包含了一些用于颜色转换、混合等实用功能的函数
 */
class VaTui::Color
{
  private:
    /*
     * `fastOutput`是一个私有的成员，
     * 用来实现快速输出文字到终端，
     * 实际上是对`系统调用write()向标准输出写数据`的封装
     */
    static inline void fastOutput ( const char *str );

  public:
    /*
     * 以下为颜色输出相关的函数
     * 使用Ansi转义序列的颜色控制字符实现
     * 对终端有一定要求，如果终端不支持这些Ansi序列
     * 或者对转义字符的解释不符合预期效果
     * 就会导致乱码等
     *
     * 以下方法按照返回值类型来分为2种
     * 例如
     *
     * _SetColor256:
     * 生成用于设置16位颜色模式下文本前景色和背景色的 ansi 转义序列字符串，
     * 返回该字符串指针。
     * 按照 ansi
     * 转义序列中针对16位颜色模式的特定格式，结合传入的前景色和背景色参数，
     * 使用 snprintf 函数构造相应字符串，
     * 存储在 escapecommand 数组中供后续输出到终端来改变颜色显示。
     *
     * _SetColor256(int front, int background):
     * 借助 fastoutput 函数输出 _setcolor256 生成的 ansi 转义序列，
     * 实现将终端文本的前景色和背景色设置为指定的16位颜色模式颜色。
     *
     * 使用样例见手册页
     */
    static const char *_SetColor4bit ( int front, int background );
    static void        SetColor4bit ( int front, int background );
    static const char *_SetColor256 ( int front, int background );
    static void        SetColor256 ( int front, int background );
    static const char *_set_background_color_RGB ( int R, int B, int G );
    static void        set_background_color_RGB ( int R, int B, int G );
    static const char *_set_front_color_RGB ( int R, int B, int G );
    static void        set_front_color_RGB ( int R, int B, int G );

    /*
     * 设置Ansi文字特效
     *
     * SetEffect:
     * 根据传入的文本显示效果枚举值（effect）以及是否启用该效果的布尔值（isEnable），
     * 生成对应的 ANSI 转义序列字符串，返回该字符串指针。
     * 如果 isEnable 为 true，则按照启用效果的 ANSI 转义序列格式，
     * 使用 snprintf 函数构造相应字符串；
     * 若为 false，则按照禁用效果的格式构造字符串，
     * 存储在 escapecommand 数组中供后续输出使用。
     *
     * 同理，SetEffect用来立即应用_SetEffect的构造结果
     */
    static const char *_SetEffect ( short effect, bool isEnable );
    static void        SetEffect ( short effect, bool isEnable );
    static const char *_ColorEffectReset ();
    static void        ColorEffectReset ();

    /*
     * 其它
     *
     * 以下是一些其他的实用颜色处理相关函数，
     * 包括颜色模式之间的转换、颜色混合以及颜色反转等功能，
     * 为更复杂的颜色操作需求提供支持，
     * 方便在不同颜色应用场景中进行灵活的颜色调整和处理。
     *
     * PS:(Lc3124)
     * 说实话，我也不清楚什么时候才会真正用到颜色混合相关的函数
     * 但其他的，比如颜色格式的转换完全可以用来兼容不同颜色支持的终端
     * 我相信它们有不错的效果
     */

    //这些函数名称和签名使其功能显而易见，所以不过多注释
    static int  RgbToAnsi256Color ( int r, int g, int b );
    static void Ansi256ColorToRGB ( int ansi256Color, int &r, int &g, int &b );
    static int  MixAnsi256Colors ( int color1, int color2 );
    static int  AntiAnsi256Color ( int colorcode );
    static int  Ansi16ColorToAnsi256 ( int ansi16Color );
    static int  Ansi256ColorToAnsi16 ( int ansi256Color );
    // 4bit颜色是有前景和背景之分的，这里根据isFrontOrBack来决定操作对象
    static int Ansi256ColorToAnsi4bit ( int ansi256Color, bool isFrontOrBack );
    //因为16色和256色兼容，所以只做4到16色转换
    //但是4bit颜色是有前景和背景之分的，这里转换后就没有这样的区分了
    static int Ansi4bitColorToAnsi16 ( int ansi4bitColor );
};

/*
 * Functions related to cursor actions.
 * VaCursor 类封装了一系列与终端光标操作相关的函数，通过生成和输出 ANSI
 * 转义序列来控制终端光标的位置、显示状态等，为在终端应用中实现自定义的光标行为提供了便捷的方式。
 */
class VaTui::Cursor
{
  private:
    //封装write,为特效字符串提供输出
    static inline void fastOutput ( const char *str );

  public:
    /*
     * Functions related to cursor actions.
     * 以下是与光标操作相关的一系列公共函数，提供了不同类型的光标操作功能，
     * 例如移动到指定位置、按方向移动以及控制光标显示隐藏等，
     * 方便开发者根据具体需求灵活操控终端光标。
     */

    /*
     * 以下程序用来控制光标移动
     * 通过构建Ansi转义序列的相关字符和参数来使终端控制光标移动
     * 但是需要终端的支持
     */
    static const char *_CursorMoveTo ( int h, int w );
    static void        CursorMoveTo ( int h, int w );

    //这个CursorMove方法用来移动光标，dr传入枚举常量参数,ds传入移动方向
    static const char *_CursorMove ( int dr, int ds );

    static void        CursorMove ( int dr, int ds );
    static const char *_CursorReset ();

    // 返回用于将光标重置到默认位置（通常是终端屏幕左上角，即第一行第一列）的
    // ANSI 转义序列字符串。
    static void        CursorReset ();
    static const char *_CursorHide ();

    static void        CursorHide ();
    static const char *_CursorShow ();
    static void        CursorShow ();
};

/*
 * 这个某块用来获取各种系统信息
 * 比如用户名，运行环境，所在目录等等
 */
class VaTui::System
{
  public:
    static std::string getUserName ();
    static std::string getCurrentTime ();

    //这个需要说明的是此函数用于获取环境变量，index为变量名，返回变量值
    static std::string getRunningEnvironment ( const char *index );

    static std::string getDeviceName ();
    static std::string getHostName ();
    static std::string getRunningDirectory ();
    static std::string getSystemOuput ( const char *cmd );
};

/*
 * 这个模块用于控制终端来实现输入输出和一些关键的操作
 * 包括了检测终端对某个功能的支持情况、非阻塞获取键盘输入、
 * 屏幕清空、光标位移、启用或禁用回显等等
 *
 * 在linux下的实现是对terminfo,termios等库的封装
 */
class VaTui::Term
{

  public:
    //这个函数会获取一次终端设置,保存在一个静态变量中
    static void getTerminalAttributes ();
    static void setTerminalAttributes ( const struct termios &newAttrs );
    static void setTerminalAttrsSafely ( const termios &newAttrs );
    static void getTerminalAttrsSafely ();

    //保存(刷新)终端设置
    static void SaveTerm ();
    //恢复终端设置
    static void RestoreTerm ();
    //生成用来清空屏幕的Ansi字符串
    static const char *_Clear ();
    //上一个函数的直接输出版本
    static void Clear ();
    //清除一行
    static const char *_ClearLine ();
    static void        ClearLine ();
    //启用回显
    static void enableEcho ();
    static void disableEcho ();
    //启用控制台缓冲
    static void enableConsoleBuffering ();
    static void disableConsoleBuffering ();
    //获取终端大小
    static void getTerminalSize ( int &rows, int &cols );
    //设置光标位置
    static void setCursorPosition ( int row, int col );
    static void saveCursorPosition ();
    static void restoreCursorPosition ();
    //无缓冲输出
    static void fastOutput ( const char *str );
    //类似getch()
    static char nonBufferedGetKey ();
    //返回终端类型
    static const char *getTerminalType ();
    //启、禁用行缓冲
    static void setLineBuffering ( bool enable );
    //类似getch()，但没有终端回显，副作用是执行完后会把回显打开
    static char getCharacter ();
    //检查feature是否支持
    static bool isTerminalFeatureSupported ( const char *feature );

    //关于什么速度设置，我不清楚，按照《Linux/Unix编程手册》实现的
    void        setCharacterDelay ( int milliseconds );
    static int  getInputSpeed ();
    static void setInputSpeed ( int speed );
    static void setOutputSpeed ( int speed );
    //无阻塞获取按键，失败就返回-1，否则不返回固定的值
    static int getkeyPressed ( char &k );
    //根据传入的枚举常量参数来设置光标形状
    static void setCursorShape ( CursorShape shape );
    //所有更改终端和fastOutput都有可能影响到标准输出,
    //如果你不是在开发一个完全的TUI程序
    //请使用这个函数刷新标准输出
    static void FlushStdOut ();
};

/*
 * 这个模块用来识别Utf字节，
 * 当然还有一些其他的功能，也会在之后加入新的功能
 */
class VaTui::Utf
{
  public:
    // 检测一段含有UTF字符的字符串的第一个有效字符的内存宽度
    // 详见手册
    size_t getUtf8CharWidth ( const char *s );
    // 根据一个 utf8字符的首位字节来确定该utf8字符的字节数
    static int  getUtf8ByteCount ( char c );
    static bool isAscii ( char c );
    // 判断一个字节是不是utf字符编码字符的开头字节
    static bool isUtf8StartByte ( char c );
    // 判断一个常规意义上的字符串是不是一个完整的utf8字符
    static bool isUtf8Char ( const char *bytes, int len );
    // 如方法名
    static bool isGbkChar ( const char *bytes, int len );
    // 从一段字符串中获取第index个Utf8字符,返回是否成功
    static bool getUtf8CharaInString ( std::string resource, std::string &save,
                                       int index );
    // 获取一个字符串的utf8字符数量
    static int getUtf8StringLen ( std::string resource );
    /* TODO */
    // 获取一个字符串在常规显示屏上的等宽字体tty上占用的宽度
    static int getStrWidthOs ( std::string resource );
};

#endif
