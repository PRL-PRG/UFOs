#include <stdio.h>
#include <stdlib.h>
int main() {
    FILE* file = fopen("test.bin", "rb");
    if (file) {

        int nums[] = {5,4,3,2,1,0};
        int *x = (int *) malloc (sizeof(int) * 1);
        for (int i = 5; i >= 0; i--) {
            fseek(file, sizeof(int) * i, SEEK_SET);
            fread(x, sizeof(int), 1, file);
            printf("%i\n",*x);
        }
        fclose(file);
    }
}
