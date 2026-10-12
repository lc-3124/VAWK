#include <vatui.hpp>
#include <vaterm.hpp>

/**
 * lc3124(261012): 这个程序打算实现一个输入框，用户可以直接在内部输入文字、编辑文本，
 * 然后点击提交，得到反馈。用来测试vatui的整套输入链路可用性。
 * 程序有一定复杂性，我打算用OOP来架构这个demo
 */

class ui 
{
  public:
  
  vatui::VaTui& tui;
  vaterm::input inputer;

  class input_box;
  class submit_button;
  class notice_box;



};
