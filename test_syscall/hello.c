#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main() {
    char buf[1024] = {0};
    write(1, "hello world\n", strlen("hello world\n"));
    while (1) {
        scanf("%s", buf);
        printf("buf------------ %s\n", buf);
    }
}
