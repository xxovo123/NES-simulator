# **NES 模拟器开发：核心参考资源指南**

构建模拟器时，你需要处理 CPU、PPU（图形）、APU（音频）和存储映射。以下是每个模块最推荐的学习资源。

## **1\. 终极宝库：NESdev Wiki (必须收藏)**

这是全球最权威的 NES 开发百科全书。几乎所有模拟器开发者遇到的问题在这里都有答案。

* **网址**: [https://www.nesdev.org/wiki/Nesdev\_Wiki](https://www.nesdev.org/wiki/Nesdev_Wiki)  
* **必读页面**:  
  * CPU: 了解 2A03 的寄存器和指令细节。  
  * PPU: 图像渲染的核心逻辑、扫描线（Scanlines）和渲染时序。  
  * iNES: 了解 .nes 文件的结构。  
  * Standard controllers: 了解如何读取手柄输入。

## **2\. CPU 模拟：6502 指令集手册**

NES 的 CPU 是基于 MOS 6502 裁剪而来的。你需要一份极其详尽的指令列表。

* **Obelisk 6502 Reference**: [http://www.obelisk.me.uk/6502/](http://www.obelisk.me.uk/6502/)  
  * **用途**: 提供每条指令的寻址模式（Addressing Modes）、受影响的标志位（Flags）以及消耗的周期数。  
* **6502.org**: [http://www.6502.org/tutorials/6502opcodes.html](http://www.6502.org/tutorials/6502opcodes.html)  
  * **用途**: 指令集的快速查阅表。

## **3\. PPU (图形处理) 深度文档**

PPU 是 NES 模拟器中最难的部分，因为它是一个并行的处理器。

* **PPU Rendering 流程图**: NESdev 上的渲染时序图（Rendering Diagram）是实现背景滚动和精灵显示的关键。  
* **Brad Taylor 的 NTSC PPU 说明书**: 这是一个比较老但在社区广为流传的底层技术文档。

## **4\. 调试与验证：测试 ROM (极其重要)**

你不需要盲目猜代码写得对不对，前辈们已经写好了自动化测试工具。

* **nestest**: [https://www.nesdev.org/wiki/Emulator\_tests](https://www.nesdev.org/wiki/Emulator_tests)  
  * **如何使用**:  
    1. **获取文件**: 下载 nestest.nes 和配套的 nestest.log（黄金标准日志）。  
    2. **自动模式设置**: 将 CPU 的程序计数器（PC）硬编码初始设置为 0xC000（这是该测试 ROM 的自动化执行入口）。  
    3. **日志输出**: 让你的模拟器在每执行一条指令前，向控制台或文件打印当前 CPU 状态，格式需严格匹配：PC Opcode A:XX X:XX Y:XX P:XX SP:XX CYC:XXX。  
    4. **比对**: 使用文本比对工具（如 Linux 的 diff 或 Windows 的 WinMerge）将你的输出与 nestest.log 进行对比。只要有一行不匹配，就说明该指令的逻辑或周期计算有误。  
* **blargg's tests**: 用于测试更精细的时序问题和声音（APU）问题。

## **5\. 经典实战参考代码/教程**

* **JavidX9 的 NES 系列**: 他的代码结构非常清晰，非常适合配合 C/C++ 学习。  
* **FCEUX 源码**: 虽然它是用 C++ 写的且代码量巨大，但在你遇到某个冷门 Mapper 搞不定时，参考它的实现是最稳妥的。

## **建议阅读顺序**

1. **先读 iNES 文件格式**: 搞清楚怎么把马里奥的数据读进内存。  
2. **读 6502 寻址模式**: 这是理解指令执行逻辑的前提。  
3. **读 CPU 内存映射 (Memory Map)**: 搞清楚内存 pull 的 0x0000 到 0xFFFF 对应的都是什么（RAM, PPU 寄存器, ROM）。  
4. **最后再啃 PPU**: 在你的 CPU 能够通过 nestest 验证之前，不要去碰图形渲染，否则你会陷入调试泥潭。