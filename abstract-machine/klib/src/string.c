#include <klib.h>
#include <klib-macros.h>
#include <stdint.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

size_t strlen(const char *s) {
    size_t res = 0;
    while(*s++) res++;
    return res;
}

char *strcpy(char *dst, const char *src) {
    char *res = dst;
    for(; (*src) && (*dst=*src); dst++, src++) ;
    return res;
}

// 到\0不停, 但之后是补齐\0, 超过n不补\0
char *strncpy(char *dst, const char *src, size_t n) {
    char *res = dst;
    for (; *src && n && (*dst=*src); dst++, src++, n--) ;
    for (; n && (*dst=0); dst++, n--);  // 补0
    return res;
}

char *strcat(char *dst, const char *src) {
    int i=strlen(dst);
    strcpy(dst+i, src);
    return dst;
}

// 感觉和标准实现一样
// s1>s2->1
// s1=s2->0
// s1<s2->-1
// 如果一个是另一个头子串, 则返回子串后一个的ascii*符号
int strcmp(const char *s1, const char *s2) {
    int flag;
    for (; *s1==*s2; s1++, s2++) ;
    if (!*s1) return 0-(int)*s2;
    if (!*s2) return (int)*s1;
    flag = ((int)*s1-(int)*s2)>0; // 不可能等于0, >0则为正数
    return flag*2-1;
}

// 到\0就不比了
int strncmp(const char *s1, const char *s2, size_t n) {
    int flag;
    for (; *s1 && n && (*s1==*s2); s1++, s2++, n--) ;
    if (!*s1) return 0-(int)*s2;
    if (!*s2) return (int)*s1;
    flag = ((int)*s1-(int)*s2)>0; // 不可能等于0, >0则为正数
    return flag*2-1;
}

void *memset(void *s, int c, size_t n) {
    char *p = s;
    while(n--){
        *p = (char)c&0xff;
        p++;
    }
    return s;
}

// 与memcpy不同是能正确处理重叠
void *memmove(void *dst, const void *src, size_t n) {
    char tmp[n];
    memcpy(tmp, src, n);
    return memcpy(dst, tmp, n);
}

void *memcpy(void *out, const void *in, size_t n) {
    char *res = out;
    const char * sin = in;
    for (; n && (*res=*sin); res++, sin++, n--) ;
    return out;
}

int memcmp(const void *s1, const void *s2, size_t n) {
    int flag;
    const char * ss1=s1;
    const char * ss2=s2;
    for (; n && (*ss1==*ss2); ss1++, ss2++, n--) ;
    if (!*ss1) return 0-(int)*ss2;
    if (!*ss2) return (int)*ss1;
    flag = ((int)*ss1-(int)*ss2)>0; // 不可能等于0, >0则为正数
    return flag*2-1;
}

#endif
