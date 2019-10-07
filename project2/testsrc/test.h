#include <stdio.h>

#define TEST(expr) if(!(expr)) { printf("%s, line %d: err\n", __FILE__, __LINE__); return 0; }

#define TEST_SUITE(name, content) int name##_test() {  printf(#name " test\n"); content; return 1; }

int fileio_test();