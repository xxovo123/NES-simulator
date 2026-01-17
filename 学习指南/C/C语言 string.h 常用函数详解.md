# **C 语言 \<string.h\> 常用函数指南**

\<string.h\> 库提供了两类函数：一类以 str 开头，用于处理以 \\0 结尾的**字符串**；另一类以 mem 开头，用于直接处理**内存块**。

## **1\. memcmp 函数详解**

memcmp 用于比较两块内存区域的前 n 个字节。

### **函数原型**

int memcmp(const void \*str1, const void \*str2, size\_t n);

### **参数与返回值**

* **str1 / str2**: 指向要比较的内存块的指针。  
* **n**: 要比较的字节数。  
* **返回值**:  
  * 0: 两块内存内容完全相同。  
  * \> 0: 在第一个不同的字节上，str1 的值大于 str2。  
  * \< 0: 在第一个不同的字节上，str1 的值小于 str2。

### **代码示例**

\#include \<stdio.h\>  
\#include \<string.h\>

int main() {  
    char a\[\] \= "ABCDE";  
    char b\[\] \= "ABCCE";

    // 比较前 3 个字节 \-\> 返回 0 (相等)  
    if (memcmp(a, b, 3\) \== 0\) {  
        printf("前3个字节相同\\n");  
    }

    // 比较前 4 个字节 \-\> 'D' (68) \> 'C' (67)，返回正数  
    if (memcmp(a, b, 4\) \> 0\) {  
        printf("在第4个字节处，a 大于 b\\n");  
    }

    return 0;  
}

### **注意事项：memcmp vs strcmp**

1. **终止符**: strcmp 遇到 \\0 就会停止比较，而 memcmp 会严格执行完 n 个字节的检查。  
2. **数据类型**: memcmp 可以比较结构体、数组等任何二进制数据，而 strcmp 仅限字符串。  
3. **安全性**: 使用 memcmp 比较结构体时要注意“字节对齐”产生的填充位（Padding），这些填充位的值是随机的，可能导致逻辑上相等的结构体在内存比较时返回不等。

## **2\. string.h 其他常用函数**

我们将常用函数分为四类：

### **A. 内存操作类 (Memory Ops)**

| 函数 | 功能描述 |
| :---- | :---- |
| **memset** | 将内存块的前 n 个字节设置为特定的值（通常用于初始化为 0）。 |
| **memcpy** | 内存拷贝。**注意**：源和目的区域不能重叠。 |
| **memmove** | 增强版内存拷贝。即使源和目的区域重叠也能正确处理。 |

### **B. 字符串复制与拼接 (Copy & Cat)**

| 函数 | 功能描述 |
| :---- | :---- |
| **strcpy** | 将源字符串（含 \\0）复制到目标缓冲区。 |
| **strncpy** | 复制前 n 个字符。如果源串短，则补 \\0；如果长，目标串可能没有 \\0。 |
| **strcat** | 将源字符串连接到目标字符串的末尾。 |

### **C. 字符串比较与长度 (Comp & Length)**

| 函数 | 功能描述 |
| :---- | :---- |
| **strlen** | 计算字符串长度，不包括末尾的 \\0。 |
| **strcmp** | 比较两个字符串。 |
| **strncmp** | 比较两个字符串的前 n 个字符。 |

### **D. 查找与分割 (Search & Split)**

| 函数 | 功能描述 |
| :---- | :---- |
| **strchr** | 查找字符在字符串中第一次出现的位置。 |
| **strstr** | 查找子串在主串中第一次出现的位置。 |
| **strtok** | 分割字符串（注意：该函数会修改原字符串且非线程安全）。 |

## **3\. 综合代码演示**

\#include \<stdio.h\>  
\#include \<string.h\>

int main() {  
    char dest\[20\];  
    char src\[\] \= "Hello, World\!";

    // 1\. memset: 初始化内存  
    memset(dest, 0, sizeof(dest));

    // 2\. memcpy: 拷贝内存  
    memcpy(dest, src, 5); // 拷贝 "Hello"  
    printf("memcpy 结果: %s\\n", dest);

    // 3\. strlen: 获取长度  
    printf("src 长度: %zu\\n", strlen(src));

    // 4\. strstr: 查找子串  
    char \*sub \= strstr(src, "World");  
    if (sub) printf("找到子串: %s\\n", sub);

    return 0;  
}  
