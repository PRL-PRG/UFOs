#define tryPerr(res, errPred, f, errMsg, gotoE) ({    \
  res = f;                               \
  if(errPred){                           \
    perror(errMsg);                      \
    goto gotoE;                          \
  }                                      \
})

#define tryPerrInt(res, f, errMsg, gotoE)    tryPerr(res, 0 != res,    f, errMsg, gotoE)
#define tryPerrNull(x, f, errMsg, gotoE)     tryPerr(x, NULL == x, f, errMsg, gotoE)
#define tryPerrNegOne(res, f, errMsg, gotoE) tryPerr(res, -1 == res,   f, errMsg, gotoE)
