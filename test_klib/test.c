#include <am.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/******************* string.h **********************/
void test_ncmp(int (*f)(const char*, const char*, size_t)){
    // 测试相等的情况
    char str1[] = "Hello, world!";
    char str2[] = "Hello, world!";
    int result = f(str1, str2, 3);
    printf("Comparing '%s' and '%s' 3: %d\n", str1, str2, result);
    result = f(str1, str2, 0);
    printf("Comparing '%s' and '%s' 0: %d\n", str1, str2, result);

    str1[0] = 'A';
    str2[0] = 'B';
    result = f(str1, str2, 3);
    printf("Comparing '%s' and '%s' 3: %d\n", str1, str2, result);
    result = f(str1, str2, 0);
    printf("Comparing '%s' and '%s' 0: %d\n", str1, str2, result);

    // 测试第一个字符串大于第二个字符串的情况
    str1[0] = 'Z';
    str2[0] = 'A';
    result = f(str1, str2, 3);
    printf("Comparing '%s' and '%s' 3: %d\n", str1, str2, result);

    str2[0] = 'Z';
    str1[5] = '\0';
    str2[5] = '\0';
    str2[6] = '1';
    result = f(str1, str2, 8);
    printf("Comparing '%s' and '%s' 8: %d\n", str1, str2, result);

    // 测试第一个字符串小于第二个字符串的情况

    // 测试空字符串的情况
    char empty1[] = "";
    char empty2[] = "";
    result = f(empty1, empty2, 3);
    printf("Comparing '%s' and '%s' 3: %d\n", empty1, empty2, result);

    // 测试一个字符串是另一个字符串的前缀的情况
    char prefix[] = "Hel";
    char full[] = "Hello, world!";
    result = f(prefix, full, 2);
    printf("Comparing '%s' and '%s' 2: %d\n", prefix, full, result);
    result = f(prefix, full, 3);
    printf("Comparing '%s' and '%s' 3: %d\n", prefix, full, result);
    result = f(prefix, full, 4);
    printf("Comparing '%s' and '%s' 4: %d\n", prefix, full, result);
    result = f(prefix, full, 5);
    printf("Comparing '%s' and '%s' 5: %d\n", prefix, full, result);
}

void test_strncmp(){
    test_ncmp(strncmp);
}
void test_memcmp(){
    test_ncmp((int(*)(const char*, const char*, size_t))memcmp);
}

void test_strcmp(){
    // 测试相等的情况
    char str1[] = "Hello, world!";
    char str2[] = "Hello, world!";
    int result = strcmp(str1, str2);
    printf("Comparing '%s' and '%s': %d\n", str1, str2, result);

    // 测试第一个字符串小于第二个字符串的情况
    str1[0] = 'A';
    str2[0] = 'B';
    result = strcmp(str1, str2);
    printf("Comparing '%s' and '%s': %d\n", str1, str2, result);

    // 测试第一个字符串大于第二个字符串的情况
    str1[0] = 'Z';
    str2[0] = 'A';
    result = strcmp(str1, str2);
    printf("Comparing '%s' and '%s': %d\n", str1, str2, result);

    // 测试空字符串的情况
    char empty1[] = "";
    char empty2[] = "";
    result = strcmp(empty1, empty2);
    printf("Comparing '%s' and '%s': %d\n", empty1, empty2, result);

    // 测试一个字符串是另一个字符串的前缀的情况
    char prefix[] = "Hello";
    char full[] = "Hello, world!";
    result = strcmp(prefix, full);
    printf("Comparing '%s' and '%s': %d\n", prefix, full, result);
}

/******************* stdlib.h **********************/

/******************* stdio.h **********************/
// 直接看打印结果自己判断对不对(我不知道怎么检查)
void test_printf(){
    // 测试不同类型数据的打印
    int int_val = 123456;
    // float float_val = 3.14159;
    char char_val = 'A';
    char *str_val = "Hello, world!";

    printf("Integer: %d\n", int_val);
    // printf("Float: %f\n", float_val);
    printf("Character: %c\n", char_val);
    printf("String: %s\n", str_val);

    // 测试格式化输出
    // printf("Hexadecimal: %#x\n", int_val);
    // printf("Octal: %#o\n", int_val);
    // printf("Scientific notation: %e\n", float_val);
    // printf("Percentage: %.2f%%\n", float_val);

    // 测试宽度和精度控制
    // printf("Integer (width 10): %10d\n", int_val);
    // printf("Float (width 10, precision 3): %10.3f\n", float_val);
    // printf("String (width 20): %20s\n", str_val);

    // 测试转义序列
    printf("%%\n");
    printf("Newline:\n");
    printf("Tab:\tThis is a tab.\n");
    printf("Backslash: \\\n");

    // 测试错误情况
    printf("Invalid conversion: %d\n", char_val); // 整数转换字符类型
    printf("Missing argument: %d %d\n", int_val); // 缺少参数
}

// 

int main()
{
    test_printf();
    test_strcmp();
    test_strncmp();
    test_memcmp();
    return 0;
}

