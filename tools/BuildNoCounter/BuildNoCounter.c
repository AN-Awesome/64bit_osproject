#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
    printf("\n========BUILD COUNTER========\n");

    FILE* pFile = fopen("./tools/BuildNoCounter/count.ini", "r");
    if (pFile == NULL) return 0;
    char str[1024];
    fgets(str, 1024, pFile);
    int build_count = atoi(str);

    fclose(pFile);

    pFile = fopen("./tools/BuildNoCounter/count.ini", "w");
    printf("NOW BUILD COUNT : %d -> %d\n\n", build_count, build_count + 1);
    fprintf(pFile, "%d", build_count + 1);

    fclose(pFile);

    return 0;
}