#pragma once

#include "vars.h"

#define ceilDiv(a,b) ({         \
    mVar(a, _a); mVar(b, _b);   \
    (_a / _b) + (0 != _a % _b); \
  })

#define gcd(a,b) ({           \
    mVar(a, _a); mVar(b, _b); \
    __typeof__(a) _rem;       \
    do{                       \
      _rem = _a % _b;         \
      _a = _b;                \
      _b = _rem;              \
    }while(_rem != 0);        \
    _a;                       \
  })
