#include <stdio.h>

//使用 #pragma pack
#pragma pack(push,1)
struct PackedStudent_1 {
    char grade;
    int id;
    short age;
};
#pragma pack(pop)


//使用 GCC 的 __attribute__((packed))
struct __attribute__((packed)) PackedStudent_2{
    char grade;
    int id;
    short age;
};

// 普通结构体
struct Nomal{
    char grade;
    int id;
    short age;
};

int main(){
    printf("PackedStudent_1 size: %d\n",sizeof(struct PackedStudent_1));
    printf("PackedStudent_2 size: %d\n",sizeof(struct PackedStudent_2));
    printf("Nomal size: %d\n",sizeof(struct Nomal));

}

/*
    q1：一般来说，普通结构体是按几字节对齐，也就是普通结构体的对齐规则是什么？
    q2：紧凑型结构体和普通结构体在内存中的排布举例？
    q3：#pragma pack(push,1) 和 #pragma pack(pop) 是什么意思，这两行代码做了什么？
    q4：__attribute__((packed)) 是什么意思，这行代码做了什么？ 跟在struct 后面 变量名前面的部分应该怎么称呼？
*/



