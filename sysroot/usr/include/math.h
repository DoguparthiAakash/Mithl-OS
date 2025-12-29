#ifndef _MATH_H
#define _MATH_H

#define HUGE_VAL (__builtin_huge_val())

// Stubs for functions if TCC uses them
double floor(double x);
double ceil(double x);
double pow(double x, double y);
double strtod(const char *nptr, char **endptr);
long double strtold(const char *nptr, char **endptr);
double ldexpl(double x, int exp);

#endif
