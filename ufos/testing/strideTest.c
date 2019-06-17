#include <stdint.h>
#include <stdio.h>

#include <printf.h>
#define strideOf(_type) ({  \
  _type* _t = (_type*) 0;  \
  (uint64_t) (_t+1);       \
  })

struct st {
    char c;
    double d;
    char v;
};

static int32_t gcd(int a, int b) {
    int remainder = a % b;
    if (remainder == 0)
        return b;
    return gcd(b, remainder);
}

int main(){

    printf("%lu\n", strideOf(char));
    printf("%lu\n", strideOf(int));
    printf("%lu\n", strideOf(long));
    printf("%lu\n", strideOf(unsigned long long));
    printf("%lu\n", strideOf(float));
    printf("%lu\n", strideOf(double));

    printf("%lu vs %lu\n", sizeof(struct st), strideOf(struct st));

    printf("%u\n", gcd(100,30));
    printf("%u\n", gcd(30, 100));

}
