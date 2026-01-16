/*
题目 1：定义一个表示 IPv4 地址的紧凑结构体
要求：
包含 4 个 unsigned char 字段（a.b.c.d）
使用 __attribute__((packed))
打印其大小，并验证是否为 4 字节
*/

#include <stdio.h>

struct __attribute__((packed)) IPv4Address
{
    unsigned char a;
    unsigned char b;
    unsigned char c;
    unsigned char d;
};

int main(){
    printf("IPv4Address size: %d\n",sizeof(struct IPv4Address));
    if (sizeof(struct IPv4Address) == 4){
        printf("IPv4Address is packed\n");
    }
}
