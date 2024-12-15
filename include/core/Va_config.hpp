/*
 * 声明一大堆常量、宏、typedef......等
 */

/**************************************/
/************ 宏孩儿 *********************/
/************ typedefs ****************/

typedef int vacolor_t;

/********** 枚举常量 **********************/
//vawk颜色模式
namespace vawkcolormode
{
    enum colormode
    {
        VAFULLCOLOR = 0,
        VA256COLOR  = 1,
        VA16COLOR   = 2,
        VA4BITCOLOR = 3,
        VABLACKWHITE= 4
    };
};

/********* 全局变量 与接口***********************/

class VaConfig
{
    private:
        static int term_max_row_length_;
        static int term_max_col_width_;  

        static vacolor_t defaultFrontColor;
        static vacolor_t defaultBackColor;

    public:
        static int num_term_max_row_length(){return term_max_row_length_;};
        static int num_term_max_col_width(){return term_max_col_width_;};
        static void set_num_term_WL(int Width,int Length){term_max_col_width_=Width;term_max_row_length_=Length;}; 

        //初始化函数
        VaConfig();
};
