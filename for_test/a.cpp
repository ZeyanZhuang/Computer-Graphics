#include <iostream>
using namespace std;

int main() {
    int n = 5;
    const char* path = "aa.ppm";
    FILE* f = fopen(path, "w");
    
    fprintf(f, "P3\n%d %d\n255\n", 5, 5);
    for(int i = 0; i < n * n; ++i) {
        fprintf(f, "%d %d %d", i, i*i%256, i*i*i%256);
    }
    return 0;
}