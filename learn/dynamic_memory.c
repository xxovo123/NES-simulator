#include <stdio.h>
#include <stdlib.h>// 包含 malloc, calloc, realloc, free
#include <string.h> // 包含 memset (可选)


/**
 * C 语言动态内存管理深度示例
 * 涵盖：malloc vs calloc 的对比、内存初始化、realloc 扩容及安全释放
 */

void print_array(const char *label,int *arr,int n){
    printf("%s: ",label);
    for(int i=0;i< n;i++){
        printf("%d ",arr[i]);
    }
    printf("\n");
}


int main(){
    int n=5;
    // --- 1. malloc 示例 ---
    // malloc 只负责分配指定字节数的内存，内容是“随机”的（残留数据）
    int *m_arr = (int *)malloc(n*sizeof(int)); //q1:malloc 的返回值 和常规用法 q2：malloc的原理，为什么分配的内存中内容是随机的
    if(m_arr == NULL){
        perror("malloc 失败");
        return 1;
    }
    print_array("malloc 分配后的内容（可能是随机值）",m_arr,n);

    // --- 2. calloc 示例 ---
    // calloc(数量, 每个元素大小) 
    // 它有两个特点：1. 参数分离；2. 自动将分配的内存初始化为 0
    int *c_arr = (int *)calloc(n,sizeof(int)); //q3:为什么calloc可以将分配的内存初始化为0，这样和malloc是否存在性能上的差距 q4：一般什么时候用malloc 什么时候用calloc
    if(c_arr == NULL){
        free(m_arr);
        perror("calloc 失败");
        return 1;
    }
    print_array("calloc 分配后的内容 (自动清零)", c_arr, n);
    // --- 3. 为什么选择 calloc? ---
    // calloc 虽然比 malloc 稍慢（因为有清零操作），但更安全，能防止读取到“垃圾数据”。
    // 如果你打算立即用循环给数组赋值，用 malloc 效率更高。

    // --- 4. realloc 深度用法 ---
    // 假设我们需要把 c_arr 的长度从 5 增加到 8
    int new_n = 8;
    // 关键点：必须使用临时指针接收 realloc 的结果
    // 如果直接 c_arr = realloc(c_arr, ...)，万一失败，原内存地址会丢失导致内存泄漏
    int *temp = (int *)realloc(c_arr,new_n * sizeof(int)); //q5:realloc的用法和原理
    if(temp != NULL){
        c_arr = temp;
        // realloc 增加的内存部分是不保证初始化的，即使原数组是 calloc 分配的
        for(int i=n;i<new_n;i++){
            c_arr[i] = (i+1) * 100;
        }
        print_array("realloc 扩容后的内容", c_arr, new_n);
    } else {
        printf("realloc 失败，保留原内存\n");
    }

    // --- 5. 内存释放的安全准则 ---
    // 释放顺序不重要，但每个分配的指针都必须释放
    free(m_arr); //q6：当函数传入的参数的类型和函数需要的类型对应不上时，C语言会做强制类型转换吗，讲解以下比较难以理解的例子（数组，指针，二维数组。。。）
    m_arr  = NULL; // 释放后置 NULL，防止“双重释放”错误 

    free(c_arr);
    c_arr = NULL;
    printf("所有动态内存已安全释放。\n");

    return 0;
}