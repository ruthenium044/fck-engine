// fckc_math.h

#ifndef FCKC_MATH_H_INCLUDED
#define FCKC_MATH_H_INCLUDED

#define fck_min(a, b) ((a) < (b) ? (a) : (b))
#define fck_max(a, b) ((a) > (b) ? (a) : (b))
#define fck_clamp(v, min, max) ((v) < (min) ? (min) : ((v) > (max) ? (max) : (v)))

#endif // !FCKC_MATH_H_INCLUDED