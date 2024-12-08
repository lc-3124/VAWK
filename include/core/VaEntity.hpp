/*
 * (c) Lc3124 2024
 * License (MIT)
 *
 * 这个文件是VAWK的基础，控件类、窗口类、Va类等等都是从这里继承的
 *
 * 详情请看定义
 */
#pragma once

//VAWK
#include "display/VaDisplayingBuffer.hpp"
//std
#include <vector>

/*
 * 提供一个一个DisplayBuffer，
 * 来为需要显示功能的子类提供一个显示缓冲区
 * 为之后的控件间递归调用draw方法和覆盖缓冲区做铺垫
 *
 * 6个虚方法对应了将来的draw,handle,process循环
 * 其中一部分实际是为了给用户和不同类型的控件来派生以实现用户的功能
 * "_"的方法会包裹同名方法被调用，同时进行额外的操作
 *
 * 向量容器和blongTo指针用来标记子控件和自己从属的对象 
 * 一个Va实体可以调用子对象的draw()等方法
 * 也可以引用子对象的显示缓冲区的一部分来覆盖自己的显示缓冲区
 *
 */ 
class VaEntity
{
    private:
        VaDisplayingBuffer *DisplayBuffer=nullptr;
        virtual void draw()=0;
        virtual void handle()=0;
        virtual void process()=0;
    
        VaEntity *blongTo=nullptr;
        std::vector<VaEntity> SubObjList;
 
        //用从属对象的显示缓冲覆盖自己的
        void CoverWith(VaEntity *cover);

        int x_,y_,w_,l_;//作为显示相关控件时自己的位置大小

    public:
        virtual void _draw(){draw();};
        virtual void _handle(){handle();};
        virtual void _process(){process();};
   
        void registersub(VaEntity *object);  //注册一个实体
        void deletesub(VaEntity *object);    //从列表删除一个实体
        void releasesub(VaEntity *object);   //删除的同时释放实体，孤悬指针需要另外处理
        
        void registeron(VaEntity *owner);   //换绑从属对象
};
