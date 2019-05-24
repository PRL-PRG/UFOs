#include <stdio.h>
#include <stdlib.h>
int main() {
    FILE* file = fopen("test.bin", "wb");
    if (file) {
        int nums[] = {2,4,8,16,32,64};
        fwrite(nums, 6, sizeof(int), file);
        fclose(file);
    }
}
