#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>


#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

int printf(const char *fmt, ...) {
    va_list ap;
    int d;
    unsigned int u;
    char c;
    char *s;
    char ds[34];
    int res=0;

    va_start(ap, fmt);
    while (*fmt){
        switch (*fmt){
        case '%':
            fmt++;
            switch (*fmt){
            // char and string
            case '%':
                putch('%');
                break;
            case 'c':
                c = (char)va_arg(ap, int);
                putch(c);
                break;
            case 's':
                s = va_arg(ap, char*);
                res += strlen(s)-1;
                while (*s) putch(*s++);
                break;
            // 整数
            case 'b': 
                d = va_arg(ap, int);
                res += itoa(d, ds, 2)-1;
                s = ds;
                while (*s) putch(*s++);
                break;
            case 'd': 
                d = va_arg(ap, int);
                res += itoa(d, ds, 10)-1;
                s = ds;
                while (*s) putch(*s++);
                break;
            case 'o': 
                d = va_arg(ap, int);
                res += itoa(d, ds, 8)-1;
                s = ds;
                while (*s) putch(*s++);
                break;
            case 'x': 
                d = va_arg(ap, unsigned int);
                res += utoa(d, ds, 16)-1;
                s = ds;
                while (*s) putch(*s++);
                break;
            case 'u':
                u = va_arg(ap, unsigned int);
                res += utoa(u, ds, 10)-1;
                s = ds;
                while (*s) putch(*s++);
                break;
            case 'p':
                u = va_arg(ap, unsigned int);
                res += utoa(u, ds, 16)-1;
                s = ds;
                putch('0');
                putch('x');
                while (*s) putch(*s++);
                break;
            }
            break;
        default: 
            putch(*fmt);
            break;
        }
        fmt++;
        res++;
    }
    return res;
}

int vsprintf(char *out, const char *fmt, va_list ap) {
  panic("Not implemented");
}

int sprintf(char *out, const char *fmt, ...) {
  panic("Not implemented");
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
  panic("Not implemented");
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  panic("Not implemented");
}

#endif
