
#define tryPerrInt(res, f, err) ({       \
  res = f;                               \
  if(0 != res){                          \
    perror(err);                         \
    return res;                          \
  }                                      \
})
