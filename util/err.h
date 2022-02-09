#ifndef LIBUTIL_ERR_H
#define LIBUTIL_ERR_H

#include <stdarg.h>

void vwarn(const char *fmt, va_list args) __asm__("libutil_vwarn");
void vwarnx(const char *fmt, va_list args) __asm__("libutil_vwarnx");
void warn(const char *fmt, ...) __asm__("libutil_warn");
void warnx(const char *fmt, ...) __asm__("libutil_warnx");
__attribute__((noreturn)) void verr(int eval, const char *fmt, va_list args) __asm__("libutil_verr");
__attribute__((noreturn)) void verrx(int eval, const char *fmt, va_list args) __asm__("libutil_verrx");
__attribute__((noreturn)) void err(int eval, const char *fmt, ...) __asm__("libutil_err");
__attribute__((noreturn)) void errx(int eval, const char *fmt, ...) __asm__("libutil_errx");
#endif
