#define DOCTEST_CONFIG_IMPLEMENT
#include "doctest.h"
#include <windows.h>

// 声明外部测试函数
extern void runInputMapperTests();

TEST_CASE("basic test")
{
    REQUIRE(1 != 2);
}

TEST_CASE("Input Mapper System Tests")
{
    printf("\n========================================\n");
    printf("运行输入映射系统完整测试套件\n");
    printf("========================================\n\n");
    
    runInputMapperTests();
    
    printf("\n输入映射测试完成\n");
}

// 自定义 main 函数以设置控制台 UTF-8 编码
int main(int argc, char** argv)
{
    // 设置控制台输出编码为 UTF-8
    SetConsoleOutputCP(65001);
    
    // 运行 doctest
    doctest::Context context;
    context.applyCommandLine(argc, argv);
    int res = context.run();
    
    return res;
}