#include <iostream>

#include "headers.hpp"
#include "status.hpp"

#define TEST(expr)                                      \
if(!(expr)) {                                           \
    printf("%s, line %d: err\n", __FILE__, __LINE__);   \
    return 0;                                           \
}

#define TEST_STATUS(expr)                               \
if(!(expr)) {                                           \
    printf("%s, line %d: err\n", __FILE__, __LINE__);   \
    return Status::FAILURE;                             \
}

#define TEST_SUCCESS(val) TEST(val == Status::SUCCESS);

#define TEST_SUITE(name, content)                           \
int name##_test() {                                         \
    content;                                                \
    printf("[*] %s: " #name " test success\n", __FILE__);   \
    return 1;                                               \
}

int fileio_test();
int headers_test();
int disk_manager_test();
int buffer_manager_test();
int bptree_test();
int table_manager_test();
int bptree_iter_test();
int join_test();
