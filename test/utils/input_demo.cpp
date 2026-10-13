// -----------------------------------------------------------------------
//  input_demo.cpp — vaterm 层：模拟一个输入框（打一键，上一字）
//
//  只用 vaterm：
//    - vaterm::terminal  — raw 模式、清屏/清行、写与 flush
//    - vaterm::cursor    — 定位、隐藏/显示光标
//    - vaterm::color     — 一点配色
//    - vaterm::input     — 输入引擎（waitInput 等）
//
//  行为：
//    每按一个可打印键（含 CJK），就在“输入框”那一行里多显示一个字；
//    Backspace 删一字；Enter 清空；Esc / Ctrl+C 退出。
//  这是纯 vaterm 的演示，不涉及 vatui 的帧缓冲。
// -----------------------------------------------------------------------

#include <vaterm.hpp>
#include <vaterm/input.hpp>   // vaterm::input / InputData / encode_utf8
#include <vaterm/term.hpp>    // vaterm::terminal
#include <vaterm/cursor.hpp>  // vaterm::cursor
#include <vaterm/color.hpp>   // vaterm::color

#include <cstdio>
#include <string>

vaterm::terminal term;
vaterm::input input;
std::string data;

void output_bar()
{
  vaterm::terminal::write(vaterm::terminal::clear_screen());
  vaterm::terminal::write(vaterm::cursor::move_to(0,0));
  vaterm::terminal::write("input: ");
  vaterm::terminal::write(data);
}

int main () 
{
  // 进入 raw 模式，开始接收输入
  if(!term.enter_raw()){
    vaterm::terminal::write("Entering raw mode failed !");
  }
  if(!input.start()){
    vaterm::terminal::write("Staring input framework failed!");
  }

  vaterm::input::dumper du(input);
  du.reset();
  while( true )
  {
    output_bar();
    auto in = du.dump();
    if(in.inputType == vaterm::InputData::InputType::KEYBD)
    {
      if(in.value.keybd.isCtrl == true &&
          in.value.keybd.cp == 'c')
      {
        break;
      }
      else if (in.value.keybd.code == vaterm::InputData::KeyCode::BACKSPACE)
      {
        vaterm::utf::pop_back(data);
      }
      else
        data+=vaterm::KeyPrintf(in.value.keybd);
    }
  }
}
