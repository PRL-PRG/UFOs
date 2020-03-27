#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

int main (int argc,char *argv[]) {
    if (argc != 4) {
        printf("usage: gen FILE N_ITEMS VALUE");
        exit(1);
    }

    char *path = argv[1]; 
    size_t n = atoi(argv[2]);
    uint32_t v = atoi(argv[3]);

    FILE *fp = fopen(path, "w");
    for (size_t i = 0; i < n; i++) {
        fwrite(&v, sizeof(uint32_t), 1, fp);
    }    
    fclose(fp);
}
