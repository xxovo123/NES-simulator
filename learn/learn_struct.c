#include <stdio.h>
#include <string.h>

struct Student {
    int id;
    char name[20];
    float score;
};

int main(){
    struct Student s1;
    s1.id = 1001;
    strcpy(s1.name,"Alice");
    s1.score = 90.5;

    printf("ID: %d\n",s1.id);
    printf("Name: %s\n",s1.name);
    printf("Score: %.2f\n",s1.score);

    return 0;
}


/*
    q1：char 数组的赋值方式
    q2：结构体为什么申明时需要使用struct关键字，如何不使用struct关键字
    q3：strcpy传入的参数的意义，为什么第一个是char *, 第二个是const char *，strcpy对于传入参数的要求
    q4：char *和 const char *的区别
*/ 