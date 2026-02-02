#!/bin/bash
# Sink端工作流测试执行脚本 (Linux/Mac)

set -e

# 颜色输出
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
NC='\033[0m' # No Color

echo "========================================"
echo "Sink端工作流测试套件 - Linux/Mac"
echo "========================================"
echo ""

# 检查CMake
if ! command -v cmake &> /dev/null; then
    echo -e "${RED}错误: 未找到CMake，请先安装CMake${NC}"
    exit 1
fi

# 创建构建目录
mkdir -p build
cd build

echo -e "${YELLOW}[1/4] 配置项目...${NC}"
cmake ..
if [ $? -ne 0 ]; then
    echo -e "${RED}错误: CMake配置失败${NC}"
    cd ..
    exit 1
fi

echo ""
echo -e "${YELLOW}[2/4] 编译测试...${NC}"
make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
if [ $? -ne 0 ]; then
    echo -e "${RED}错误: 编译失败${NC}"
    cd ..
    exit 1
fi

echo ""
echo -e "${YELLOW}[3/4] 运行测试...${NC}"
./test_sink_workflow --gtest_output=xml:sink_test_results.xml
if [ $? -ne 0 ]; then
    echo -e "${RED}错误: 测试执行失败${NC}"
    cd ..
    exit 1
fi

echo ""
echo -e "${GREEN}[4/4] 测试完成！${NC}"
echo ""
echo "测试结果文件: build/sink_test_results.xml"
echo ""

# 检查测试结果
if grep -q "\[  PASSED  \]" sink_test_results.xml; then
    echo -e "${GREEN}所有测试通过！${NC}"
else
    echo -e "${YELLOW}存在失败的测试用例，请查看测试报告${NC}"
fi

cd ..
echo ""
echo "========================================"
echo "测试执行完成"
echo "========================================"
