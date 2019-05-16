#define tryPerrInt(res, f, err, gotoE) ({       \
  res = f;                               \
  if(0 != res){                          \
    perror(err);                         \
    goto gotoE;                          \
  }                                      \
})

#define tryPerrNull(x, f, err, gotoE) ({       \
  x = f;                                 \
  if(NULL == x){                         \
    perror(err);                         \
    goto gotoE;                          \
  }                                      \
})
