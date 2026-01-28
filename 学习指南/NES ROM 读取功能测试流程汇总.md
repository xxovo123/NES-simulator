这是一个关于如何编写、编译和运行 NES ROM 读取测试的完整流程汇总。

# ---

**NES ROM 读取功能测试流程汇总**

## **1\. 目录结构确认**

首先确保你的项目根目录结构如下，**所有命令均在 project\_root 下执行**：

Plaintext

project\_root/  
├── code/  
│   ├── ines.c        \# 实现文件  
│   └── ines.h        \# 头文件  
└── test/  
    ├── nestest.nes   \# 待测试的 ROM 文件  
    └── test\_loader.c \# (新建) 测试入口代码

## **2\. 编写测试代码**

在 test/ 目录下创建 test\_loader.c。

**注意**：直接引用头文件文件名，路径问题交给编译器处理。

C

// test/test\_loader.c  
\#**include** \<stdio.h\>  
\#**include** "ines.h"  // 编译器会通过 \-I 参数找到它

int main(int argc, char\* argv\[\]) {  
    // 1\. 检查命令行参数  
    if (argc \< 2) {  
        printf("Usage: %s \<rom\_path\>\\n", argv\[0\]);  
        return 1;  
    }

    const char\* filename \= argv\[1\];  
    printf("Loading: %s...\\n", filename);

    // 2\. 调用核心加载函数  
    NesRom\* rom \= load\_nes\_rom(filename);

    if (\!rom) {  
        printf("Error: Failed to load ROM.\\n");  
        return 1;  
    }

    // 3\. 打印详细信息进行验证  
    printf("\\n=== Validation Info \===\\n");  
    printf("Mapper ID: %d\\n", rom-\>mapper\_id);  
    printf("Mirroring: %s\\n", rom-\>mirroring ? "Vertical" : "Horizontal");  
    printf("Has Trainer: %s\\n", rom-\>has\_trainer ? "Yes" : "No");  
      
    // 验证 PRG 和 CHR 大小 (基于 ines.h 定义的结构)  
    printf("PRG Header Size: %d units (Total %d KB)\\n",   
           rom-\>header.prg\_size, rom-\>header.prg\_size \* 16);  
    printf("CHR Header Size: %d units (Total %d KB)\\n",   
           rom-\>header.chr\_size, rom-\>header.chr\_size \* 8);

    // 4\. 清理内存  
    free\_nes\_rom(rom);  
    printf("\\nTest Finished.\\n");  
    return 0;  
}

## **3\. 编译命令 (在根目录执行)**

使用 gcc 编译，关键点在于链接两个 .c 文件并指定头文件搜索路径。

Bash

gcc test/test\_loader.c code/ines.c \-Icode \-o test\_nes

* test/test\_loader.c：测试主程序。  
* code/ines.c：NES 功能实现，必须包含否则报错。  
* \-Icode：告诉编译器头文件 ines.h 在 code 文件夹里。  
* \-o test\_nes：生成的可执行文件名为 test\_nes。

## **4\. 运行验证**

将 test/nestest.nes 作为参数传递给生成的可执行文件。

**Linux / macOS / Git Bash:**

Bash

./test\_nes test/nestest.nes

**Windows (CMD / PowerShell):**

DOS

test\_nes.exe test\\nestest.nes

## **5\. 预期输出结果**

如果代码逻辑正确，针对标准的 nestest.nes，你应该看到如下输出（数值参考自 ines.c 的解析逻辑）：

Plaintext

ROM Loaded: PRG=16 KB, Mapper=0    \<-- 来自 load\_nes\_rom 内部的 printf  
Loading: test/nestest.nes...

\=== Validation Info \===  
Mapper ID: 0                       \<-- 标准 NROM Mapper  
Mirroring: Horizontal              \<-- 水平镜像 (值为 0\)  
Has Trainer: No  
PRG Header Size: 1 units (Total 16 KB)  
CHR Header Size: 1 units (Total 8 KB)

Test Finished.

### ---

**常见问题排查**

* **报错 undefined reference to 'load\_nes\_rom'**: 编译时漏掉了 code/ines.c。  
* **报错 ines.h: No such file or directory**: 编译时漏掉了 \-Icode。  
* **报错 Failed to open ROM**: 运行命令时路径参数写错，确保 test/nestest.nes 路径正确且文件存在。