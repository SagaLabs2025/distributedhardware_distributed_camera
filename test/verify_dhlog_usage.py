#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
DHLOG使用验证工具
用途：自动检查测试文件中的DHLOG和LogCapture使用情况
"""

import os
import re
import sys
from pathlib import Path
from typing import List, Dict, Tuple
from datetime import datetime

class DHLOGVerifier:
    """DHLOG使用验证器"""

    def __init__(self, project_root: str):
        self.project_root = Path(project_root)
        self.test_dir = self.project_root / "test"
        self.issues = []
        self.stats = {
            "total_files": 0,
            "dhlog_files": 0,
            "logcapture_files": 0,
            "verification_points": 0,
            "issues": 0
        }

    def verify_all(self) -> bool:
        """执行所有验证"""
        print("=" * 60)
        print("DHLOG使用验证工具")
        print("=" * 60)
        print()

        results = []

        # 验证Source端测试
        print("[1/5] 验证Source端测试...")
        source_result = self.verify_source_tests()
        results.append(("Source端测试", source_result))
        print()

        # 验证Sink端测试
        print("[2/5] 验证Sink端测试...")
        sink_result = self.verify_sink_tests()
        results.append(("Sink端测试", sink_result))
        print()

        # 验证集成测试
        print("[3/5] 验证集成测试...")
        integration_result = self.verify_integration_tests()
        results.append(("集成测试", integration_result))
        print()

        # 验证LogCapture工具
        print("[4/5] 验证LogCapture工具...")
        logcapture_result = self.verify_logcapture_tool()
        results.append(("LogCapture工具", logcapture_result))
        print()

        # 生成统计报告
        print("[5/5] 生成统计报告...")
        print()
        self.print_summary(results)

        return all(result for _, result in results)

    def verify_source_tests(self) -> bool:
        """验证Source端测试"""
        test_file = self.test_dir / "unit" / "source" / "test_source_workflow.cpp"

        if not test_file.exists():
            print(f"[ERROR] 文件不存在: {test_file.relative_to(self.project_root)}")
            self.issues.append(f"Source测试文件不存在")
            return False

        content = test_file.read_text(encoding='utf-8')
        self.stats["total_files"] += 1

        # 检查DHLOG使用
        dhlog_count = content.count("DHLOGI") + content.count("DHLOGW") + content.count("DHLOGE")
        if dhlog_count > 0:
            print(f"[OK] 找到 {dhlog_count} 次DHLOG调用")
            self.stats["dhlog_files"] += 1
        else:
            print("[WARN] 未找到DHLOG使用")
            self.issues.append("Source测试未使用DHLOG")
            self.stats["issues"] += 1

        # 检查LogCapture使用
        logcapture_count = content.count("LogCapture")
        if logcapture_count > 0:
            print(f"[OK] 找到 {logcapture_count} 次LogCapture调用")
            self.stats["logcapture_files"] += 1
        else:
            print("[WARN] 未找到LogCapture使用")
            self.issues.append("Source测试未使用LogCapture")
            self.stats["issues"] += 1

        # 检查StartCapture/StopCapture配对
        start_count = content.count("StartCapture")
        stop_count = content.count("StopCapture")
        print(f"  StartCapture调用: {start_count}次")
        print(f"  StopCapture调用: {stop_count}次")

        if start_count == stop_count:
            print("[OK] StartCapture/StopCapture配对正确")
        else:
            print(f"[ERROR] StartCapture/StopCapture不匹配 ({start_count} vs {stop_count})")
            self.issues.append(f"Source测试StartCapture/StopCapture不匹配")
            self.stats["issues"] += 1

        # 统计验证点
        verification_points = (
            content.count("EXPECT_TRUE(LogCapture") +
            content.count("EXPECT_TRUE(WaitForLog") +
            content.count("EXPECT_EQ(LogCapture")
        )
        self.stats["verification_points"] += verification_points
        print(f"  验证点数量: {verification_points}")

        return start_count == stop_count and dhlog_count > 0 and logcapture_count > 0

    def verify_sink_tests(self) -> bool:
        """验证Sink端测试"""
        test_file = self.test_dir / "unit" / "sink" / "test_sink_workflow.cpp"

        if not test_file.exists():
            print(f"[ERROR] 文件不存在: {test_file.relative_to(self.project_root)}")
            self.issues.append("Sink测试文件不存在")
            return False

        content = test_file.read_text(encoding='utf-8')
        self.stats["total_files"] += 1

        # 检查DHLOG使用
        dhlog_count = content.count("DHLOGI") + content.count("DHLOGW") + content.count("DHLOGE")
        if dhlog_count > 0:
            print(f"[OK] 找到 {dhlog_count} 次DHLOG调用")
            self.stats["dhlog_files"] += 1
        else:
            print("[WARN] 未找到DHLOG使用")
            self.issues.append("Sink测试未使用DHLOG")
            self.stats["issues"] += 1

        # 检查LogCapture使用
        logcapture_count = content.count("LogCapture")
        if logcapture_count > 0:
            print(f"[OK] 找到 {logcapture_count} 次LogCapture调用")
            self.stats["logcapture_files"] += 1
        else:
            print("[WARN] 未找到LogCapture使用")
            self.issues.append("Sink测试未使用LogCapture")
            self.stats["issues"] += 1

        # 检查StartCapture/StopCapture配对
        start_count = content.count("StartCapture")
        stop_count = content.count("StopCapture")
        print(f"  StartCapture调用: {start_count}次")
        print(f"  StopCapture调用: {stop_count}次")

        if start_count == stop_count:
            print("[OK] StartCapture/StopCapture配对正确")
        else:
            print(f"[ERROR] StartCapture/StopCapture不匹配 ({start_count} vs {stop_count})")
            self.issues.append(f"Sink测试StartCapture/StopCapture不匹配")
            self.stats["issues"] += 1

        # 统计验证点
        verification_points = (
            content.count("EXPECT_TRUE(logCapture") +
            content.count("EXPECT_EQ(logCapture")
        )
        self.stats["verification_points"] += verification_points
        print(f"  验证点数量: {verification_points}")

        return start_count == stop_count and dhlog_count > 0 and logcapture_count > 0

    def verify_integration_tests(self) -> bool:
        """验证集成测试"""
        integration_dir = self.test_dir / "integration"

        if not integration_dir.exists():
            print("[INFO] 集成测试目录不存在")
            return True

        all_passed = True
        cpp_files = list(integration_dir.rglob("*.cpp"))

        for cpp_file in cpp_files:
            self.stats["total_files"] += 1
            print(f"检查: {cpp_file.relative_to(self.project_root)}")

            content = cpp_file.read_text(encoding='utf-8')

            # 检查是否使用std::cout
            cout_count = content.count("std::cout")
            if cout_count > 0:
                print(f"  [WARN] 使用std::cout {cout_count}次，应使用DHLOG")
                self.issues.append(f"{cpp_file.name}使用std::cout而非DHLOG")
                self.stats["issues"] += cout_count
                all_passed = False

            # 检查是否使用LogCapture
            logcapture_count = content.count("LogCapture")
            if logcapture_count > 0:
                print(f"  [OK] 使用LogCapture {logcapture_count}次")
                self.stats["logcapture_files"] += 1
            else:
                print(f"  [WARN] 未使用LogCapture")
                self.issues.append(f"{cpp_file.name}未使用LogCapture")
                self.stats["issues"] += 1
                all_passed = False

        if len(cpp_files) == 0:
            print("[INFO] 未找到集成测试文件")

        return all_passed

    def verify_logcapture_tool(self) -> bool:
        """验证LogCapture工具"""
        mock_dir = self.test_dir / "unit" / "mock"

        logcapture_h = mock_dir / "log_capture.h"
        logcapture_cpp = mock_dir / "log_capture.cpp"
        logcapture_test = mock_dir / "log_capture_test.cpp"

        all_exists = True

        if logcapture_h.exists():
            print("[OK] log_capture.h 存在")
        else:
            print("[ERROR] log_capture.h 不存在")
            self.issues.append("log_capture.h不存在")
            self.stats["issues"] += 1
            all_exists = False

        if logcapture_cpp.exists():
            print("[OK] log_capture.cpp 存在")
        else:
            print("[ERROR] log_capture.cpp 不存在")
            self.issues.append("log_capture.cpp不存在")
            self.stats["issues"] += 1
            all_exists = False

        if logcapture_test.exists():
            print("[OK] log_capture_test.cpp 存在")

            # 统计测试用例数
            content = logcapture_test.read_text(encoding='utf-8')
            test_count = content.count("bool Test")
            print(f"  LogCapture测试用例数: {test_count}")
        else:
            print("[WARN] log_capture_test.cpp 不存在")

        return all_exists

    def print_summary(self, results: List[Tuple[str, bool]]):
        """打印统计摘要"""
        print("=" * 60)
        print("验证结果汇总")
        print("=" * 60)
        print()

        for name, passed in results:
            status = "[通过]" if passed else "[失败]"
            print(f"{status} {name}")

        print()
        print("=" * 60)
        print("统计信息")
        print("=" * 60)
        print(f"  总检查文件数: {self.stats['total_files']}")
        print(f"  使用DHLOG的文件数: {self.stats['dhlog_files']}")
        print(f"  使用LogCapture的文件数: {self.stats['logcapture_files']}")
        print(f"  DHLOG验证点数量: {self.stats['verification_points']}")
        print(f"  发现的问题数: {self.stats['issues']}")
        print()

        if self.stats['issues'] > 0:
            print(f"[发现 {self.stats['issues']} 个问题]")
            print()
            print("问题清单:")
            for i, issue in enumerate(self.issues, 1):
                print(f"  {i}. {issue}")
        else:
            print("[所有检查通过]")

        print()
        print("=" * 60)

    def generate_report(self, output_file: str = None):
        """生成详细报告"""
        if output_file is None:
            timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
            output_file = self.test_dir / f"dhlog_verification_report_{timestamp}.txt"

        with open(output_file, 'w', encoding='utf-8') as f:
            f.write("DHLOG使用验证报告\n")
            f.write("=" * 60 + "\n")
            f.write(f"生成时间: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n")
            f.write("=" * 60 + "\n\n")

            f.write("统计信息\n")
            f.write("-" * 40 + "\n")
            f.write(f"总检查文件数: {self.stats['total_files']}\n")
            f.write(f"使用DHLOG的文件数: {self.stats['dhlog_files']}\n")
            f.write(f"使用LogCapture的文件数: {self.stats['logcapture_files']}\n")
            f.write(f"DHLOG验证点数量: {self.stats['verification_points']}\n")
            f.write(f"发现的问题数: {self.stats['issues']}\n\n")

            if self.issues:
                f.write("问题清单\n")
                f.write("-" * 40 + "\n")
                for i, issue in enumerate(self.issues, 1):
                    f.write(f"{i}. {issue}\n")

        print(f"\n详细报告已保存到: {output_file}")


def main():
    """主函数"""
    # 获取项目根目录
    script_dir = Path(__file__).parent
    project_root = script_dir.parent

    # 创建验证器
    verifier = DHLOGVerifier(str(project_root))

    # 执行验证
    passed = verifier.verify_all()

    # 生成报告
    verifier.generate_report()

    # 返回退出码
    sys.exit(0 if passed else 1)


if __name__ == "__main__":
    main()
