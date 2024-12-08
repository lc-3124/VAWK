/*
 * (c) Lc3124 2024 
 * License (MIT)
 *
 * 一个控件显示自己是必要的，
 * 这就是显示相关的类
 */
#pragma once 

//VAWLK
#include "../VaTui.hpp"
#include "../Va_config.hpp"
//std
#include <vector>
#include <string>

#define VABFnofront = -1 
#define VABFnoColor = -1
#define VABFnowidth = -1 
#define VABFNOCHANGE = -2


/*
 * 显示缓冲区类
 * 实际上也提供了写入读取画图等操作
 */
class VaDisplayingBuffer
{
    private:
        struct DisplayingPixel_struct //描述屏幕上一个位置上的字符的结构体
        {
            char* Front;            //文字     -1为透明，而不是空格 
            char* Color;            //Ansi颜色 -1为透明 
            char* speacial;         //Ansi特效 
            int Width;              //该字符实际占用屏幕宽度 -1则表示当前显示单元为多字节字符后的过渡显示单元，Color和speacial将继承左边元素
        };

        //可拓展的二位维向量容器,根据屏幕大小来初始化
        using vadsbuffermap=std::vector< std::vector<DisplayingPixel_struct >>;
        vadsbuffermap Writingbuffer,Drawingbuffer;
        int num_maxwidth = 0;
        int num_maxlength = 0;

    public:
        /*初始化*/
        //在需要重新初始化的时候初始化(因为程序运行过程中可能更改控件大小)
        void flushinit(int Width, int Length); 
        VaDisplayingBuffer(int Width, int Length);//初始化，只是封装一下flushinit就可以了
        /*析构*/
        ~VaDisplayingBuffer();

        /*对VaEntity的接口*/

        //写入数据的一些方法,允许多字节的Utf字符，有一套机制来处理
        void PutPixel(int x,int y,DisplayingPixel_struct chara);            //放置显示单元在相应位置
        void mvprint(int x,int y,char* string);                            //放置字符串
        void mvcolorprint(int x,int y,vacolor_t front,vacolor_t back,char* string);//带颜色放置

        //设置一个显示单元上的特性,注意，在这里用-2表示不更改某个成员
        void SetPixel(int x ,int y,char* Front ,char* Color, char* speacial,int Width);

        //其他绘制相关
        void colorbar(int x1,int y1,int x2,int y2,vacolor_t front,vacolor_t back);//只操作颜色
        void frontbar(int x1,int y1,int x2,int y2,char* chara);//只操作字符 

        /*对外接口*/
        vadsbuffermap giveBuffer();         //暴露自己的Drawingmap副本

};
