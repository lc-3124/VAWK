/*
 * (c) Lc3124 2024 
 * License (MIT)
 *
 * VaDisplayingBuffer 的实现文件
 */
#include "../include/core/display/VaDisplayingBuffer.hpp"
#include "../include/core/VaTui.hpp" 

void VaDisplayingBuffer::flushinit(int Width, int Length)
{
    this->num_maxwidth=Width;
    this->num_maxlength=Length;
    //重新初始化缓冲区的大小

    //二维缓冲数字的外层维度为Y(高度),内层为X(宽度)
    this->Writingbuffer.resize(Length);
    for(int index =0; index < Length ; index ++)
    {
        this->Writingbuffer.at(index).resize(Width);
        for(int i =0; i < Width ; i++)
        {
            /*
             * 没有完成！！！！
             * 要用枚举常量填充初始值！！
             */
            Writingbuffer.at(index)[i].Width    = 1;
           // Writingbuffer.at(index)[i].Color= const_cast<char*>(                //设置初始颜色,这里使用了不安全的类型转换
                    //Tui.Color._SetColor256(VaConfig::num_term_max_row_length(), //前景
                    //    VaConfig::num_term_max_col_width())                     //背景
                    //);
            Writingbuffer.at(index)[i].FrontColor    = const_cast<char*>("");
            Writingbuffer.at(index)[i].BackColor    = const_cast<char*>("");
            Writingbuffer.at(index)[i].Front    = const_cast<char*>("");            //同样不安全
            Writingbuffer.at(index)[i].speacial = const_cast<char*>("");          //别在意了
        }
    } 

    //初始化另一个(用来传递、覆盖的缓冲区)
    this->Drawingbuffer.resize(Length);
    for(int index =0; index < Length ; index ++)
    {
        this->Drawingbuffer.at(index).resize(Width);
    }

}

//构造函数
VaDisplayingBuffer::VaDisplayingBuffer(int Width,int Length)
{
    this->flushinit(Width,Length);
}
//析构函数
VaDisplayingBuffer::~VaDisplayingBuffer()
{

}
