//- input_demo.cpp — vaterm 层：模拟一个输入框，打一键，上一字
//
//  行为：
//    每按一个可打印键（含 CJK），就在“输入框”那一行里多显示一个字；
//    Backspace 删一字；Enter 清空；Esc / Ctrl+C 退出。
//  这个demo单纯演示vater::input的功能，不涉及其他层次

#include <vaterm.hpp>
#include <string>

// 初始化三个带状态的模块
vaterm::terminal term;
vaterm::input input;
vaterm::input::dumper du(input);

// 输出目标
std::string data;

void output_bar()
{
  vaterm::terminal::write(vaterm::terminal::clear_screen());
  vaterm::terminal::write(vaterm::cursor::move_to(0,0));
  vaterm::terminal::write("input: ");
  vaterm::terminal::write(data);
  vaterm::terminal::flush();
}

int main () 
{
  // 进入 raw 模式，开始接收输入
  if(!term.enter_raw()){
    vaterm::terminal::write("Entering raw mode failed !");
  }

  // 启动输入引擎
  if(!input.start()){
    vaterm::terminal::write("Staring input framework failed!");
  }
  while( true )
  {
    output_bar();
    auto in = du.dump(); ///< 可以不填写参数，默认为-1
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
