#include <stdio.h>

int main(int argc, char** argv) {
    int x = 5;
    int y = 7;
    int z = add_together(x, y);
    puts(z);
}

int add_together(int x, int y) {
    int result = x + y;
    return result;
}