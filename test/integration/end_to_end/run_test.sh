#!/bin/bash
#
# Copyright (c) 2021-2025 Huawei Device Co., Ltd.
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# ============================================================================
# 分布式相机完整流程测试运行脚本
# Distributed Camera Complete Workflow Test Runner
# ============================================================================

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 日志函数
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

print_header() {
    echo ""
    echo "=================================================="
    echo "  $1"
    echo "=================================================="
    echo ""
}

# 获取脚本所在目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../../../.." && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
REPORT_DIR="${SCRIPT_DIR}/reports"

# 创建必要的目录
mkdir -p "${BUILD_DIR}"
mkdir -p "${REPORT_DIR}"

# 解析命令行参数
BUILD_ONLY=false
RUN_ONLY=false
CLEAN_BUILD=false
VERBOSE=false
DEVICE_IP=""
GENERATE_REPORT=false

while [[ $# -gt 0 ]]; do
    case $1 in
        --build-only)
            BUILD_ONLY=true
            shift
            ;;
        --run-only)
            RUN_ONLY=true
            shift
            ;;
        --clean)
            CLEAN_BUILD=true
            shift
            ;;
        --verbose)
            VERBOSE=true
            shift
            ;;
        --device)
            DEVICE_IP="$2"
            shift 2
            ;;
        --report)
            GENERATE_REPORT=true
            shift
            ;;
        --help)
            echo "用法: $0 [选项]"
            echo ""
            echo "选项:"
            echo "  --build-only    仅构建测试，不运行"
            echo "  --run-only      仅运行测试，不重新构建"
            echo "  --clean         清理构建目录后重新构建"
            echo "  --verbose       显示详细输出"
            echo "  --device IP     运行在指定设备上"
            echo "  --report        生成HTML格式测试报告"
            echo "  --help          显示此帮助信息"
            exit 0
            ;;
        *)
            log_error "未知选项: $1"
            exit 1
            ;;
    esac
done

# ============================================================================
# 步骤1: 环境检查
# ============================================================================
print_header "步骤1: 环境检查"

check_command() {
    if command -v $1 &> /dev/null; then
        log_success "$1 已安装"
        return 0
    else
        log_warning "$1 未安装"
        return 1
    fi
}

# 检查必要的工具
check_command cmake || exit 1
check_command make || exit 1

# 检查GTest
if [ -d "/usr/include/gtest" ] || [ -d "/usr/local/include/gtest" ]; then
    log_success "GTest 已安装"
else
    log_warning "GTest 未找到，将尝试使用源码"
fi

# ============================================================================
# 步骤2: 构建测试
# ============================================================================
if [ "$RUN_ONLY" = false ]; then
    print_header "步骤2: 构建测试"

    cd "${SCRIPT_DIR}"

    if [ "$CLEAN_BUILD" = true ]; then
        log_info "清理构建目录..."
        rm -rf "${BUILD_DIR}"
    fi

    mkdir -p "${BUILD_DIR}"
    cd "${BUILD_DIR}"

    log_info "配置CMake..."
    CMAKE_FLAGS=""
    if [ "$VERBOSE" = true ]; then
        CMAKE_FLAGS="-DCMAKE_VERBOSE_MAKEFILE=ON"
    fi

    cmake ${CMAKE_FLAGS} .. || {
        log_error "CMake配置失败"
        exit 1
    }

    log_info "编译测试..."
    make -j$(nproc) || {
        log_error "编译失败"
        exit 1
    }

    log_success "构建完成"
fi

if [ "$BUILD_ONLY" = true ]; then
    log_success "测试可执行文件位于: ${BUILD_DIR}/dcamera_complete_test"
    exit 0
fi

# ============================================================================
# 步骤3: 运行测试
# ============================================================================
print_header "步骤3: 运行测试"

TEST_EXECUTABLE="${BUILD_DIR}/dcamera_complete_test"

if [ ! -f "${TEST_EXECUTABLE}" ]; then
    log_error "测试可执行文件不存在: ${TEST_EXECUTABLE}"
    exit 1
fi

# 如果指定了设备IP，则在设备上运行
if [ -n "$DEVICE_IP" ]; then
    log_info "在设备 ${DEVICE_IP} 上运行测试..."

    # 推送到设备
    log_info "推送测试程序到设备..."
    hdc -s ${DEVICE_IP} file send "${TEST_EXECUTABLE}" "/data/local/tmp/" || {
        log_error "推送失败"
        exit 1
    }

    # 设置执行权限
    hdc -s ${DEVICE_IP} shell "chmod +x /data/local/tmp/dcamera_complete_test"

    # 清空日志
    log_info "清空设备日志..."
    hdc -s ${DEVICE_IP} shell "hilog -r"

    # 运行测试
    log_info "执行测试..."
    hdc -s ${DEVICE_IP} shell "/data/local/tmp/dcamera_complete_test" || {
        log_warning "测试在设备上运行时出现错误"
    }

    # 获取报告
    log_info "获取测试报告..."
    hdc -s ${DEVICE_IP} file recv "/data/local/tmp/COMPLETE_TEST_REPORT.md" "${REPORT_DIR}/" || {
        log_warning "无法获取测试报告"
    }
else
    # 本地运行
    log_info "本地运行测试..."

    # 设置环境变量
    export DH_LOG_LEVEL=INFO
    export DCAMERA_TEST_MODE=1

    # 运行测试
    cd "${BUILD_DIR}"

    if [ "$VERBOSE" = true ]; then
        "${TEST_EXECUTABLE}" "$@"
    else
        "${TEST_EXECUTABLE}" "$@" 2>&1 | tee test_output.log
    fi

    TEST_RESULT=$?

    # 复制报告
    if [ -f "COMPLETE_TEST_REPORT.md" ]; then
        cp COMPLETE_TEST_REPORT.md "${REPORT_DIR}/"
        log_success "测试报告已复制到: ${REPORT_DIR}/COMPLETE_TEST_REPORT.md"
    fi
fi

# ============================================================================
# 步骤4: 生成HTML报告
# ============================================================================
if [ "$GENERATE_REPORT" = true ]; then
    print_header "步骤4: 生成HTML报告"

    # 检查报告文件
    REPORT_FILE="${REPORT_DIR}/COMPLETE_TEST_REPORT.md"
    if [ ! -f "${REPORT_FILE}" ]; then
        log_warning "未找到测试报告: ${REPORT_FILE}"
        exit 0
    fi

    log_info "将Markdown报告转换为HTML..."

    # 检查pandoc
    if command -v pandoc &> /dev/null; then
        pandoc "${REPORT_FILE}" -o "${REPORT_DIR}/COMPLETE_TEST_REPORT.html" \
            --standalone \
            --toc \
            --css=<(echo '
                body { font-family: Arial, sans-serif; margin: 40px; line-height: 1.6; }
                h1 { color: #2c3e50; border-bottom: 2px solid #3498db; padding-bottom: 10px; }
                h2 { color: #34495e; margin-top: 30px; }
                table { border-collapse: collapse; width: 100%; margin: 20px 0; }
                th, td { border: 1px solid #ddd; padding: 12px; text-align: left; }
                th { background-color: #3498db; color: white; }
                tr:nth-child(even) { background-color: #f2f2f2; }
                .passed { color: green; font-weight: bold; }
                .failed { color: red; font-weight: bold; }
                code { background-color: #f4f4f4; padding: 2px 6px; border-radius: 3px; }
                pre { background-color: #f4f4f4; padding: 15px; border-radius: 5px; overflow-x: auto; }
            ') || {
            log_warning "HTML报告生成失败"
        }
    else
        log_warning "pandoc未安装，跳过HTML报告生成"
        log_info "安装pandoc: sudo apt-get install pandoc"
    fi
fi

# ============================================================================
# 步骤5: 显示摘要
# ============================================================================
print_header "测试完成"

if [ -f "${REPORT_DIR}/COMPLETE_TEST_REPORT.md" ]; then
    log_success "测试报告已生成: ${REPORT_DIR}/COMPLETE_TEST_REPORT.md"

    # 提取关键信息
    echo ""
    log_info "测试摘要:"
    grep -E "^(状态|总耗时|平均FPS|内存)" "${REPORT_DIR}/COMPLETE_TEST_REPORT.md" || true
    echo ""
fi

exit 0
