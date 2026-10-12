<p align="center">
  <br>
  <img src="logo.jpg" alt="VAWK" width="320">
  <br>
</p>

<h1 align="center">VAWK</h1>

<p align="center">
  <b>Visual Ansi Widget Kit</b><br>
  <i>带有浓厚「静态」「索引」风格的 TUI 框架</i>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/C%2B%2B-23-blue?logo=c%2B%2B">
  <img src="https://img.shields.io/badge/license-MIT-green">
  <img src="https://img.shields.io/badge/platform-Linux-red?logo=linux">
  <img src="https://img.shields.io/badge/status-WIP-orange">
</p>

---

面向 Linux 终端、零外部依赖的 C++23 TUI 框架，自下而上分为 **vaterm**（终端原语）、**vatui**（帧缓冲渲染）、**vawk**（事件与装配）三层。

框架几乎不依赖动态机制：事件类型在编译期定义，页面预先声明二维导航图，方向键在图上移动游标以实现聚焦——**聚焦、事件、渲染、输入，本质都是「游标走索引」**。

> 状态：开发中。

---

MIT License · 详见 [LICENSE](LICENSE)。
