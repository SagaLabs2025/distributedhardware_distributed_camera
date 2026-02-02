# TCP Adapter Test Module

## 概述

此模块包含用于分布式相机通道的TCP适配器测试代码。这些文件专门为本地测试和开发环境设计，提供了基于TCP/IP的通信适配器实现，用于模拟软总线(SoftBus)通信。

## 文件结构

```
tcp_adapter/
├── include/
│   ├── i_communication_adapter.h      # 通信适配器接口定义
│   ├── local_tcp_adapter.h            # 本地TCP适配器头文件
│   ├── communication_adapter_factory.h # 通信适配器工厂类
│   └── dcamera_tcp_adapter.h          # 分布式相机TCP适配器头文件
├── src/
│   ├── communication_adapter_factory.cpp # 通信适配器工厂实现
│   ├── local_tcp_adapter.cpp          # 本地TCP适配器实现
│   └── dcamera_tcp_adapter.cpp        # 分布式相机TCP适配器实现
├── BUILD.gn                           # 测试模块构建配置
└── README.md                          # 本文件
```

## 功能说明

### ICommunicationAdapter 接口
定义了通信适配器的标准接口，包括：
- 创建Sink/Source套接字
- 发送字节流和数据流
- 会话管理
- 回调注册

### LocalTcpAdapter
实现了基于Windows Winsock的本地TCP通信适配器，用于：
- 本地开发测试
- 单机调试
- CI/CD自动化测试

### CommunicationAdapterFactory
工厂模式创建不同类型的通信适配器：
- 支持TCP模式
- 支持管道模式(预留)
- 通过环境变量DCAMERA_COMM_MODE配置

### DCameraTcpAdapter
将TCP适配器集成到分布式相机软总线适配器框架中，提供与生产代码兼容的接口。

## 使用方法

### 编译测试模块

1. 启用测试编译标志：
```bash
./build.sh --product-name {product_name} --build-target dcamera_tcp_adapter_test \
  --gn-args DCAMERA_TEST_ENABLE=true
```

2. 运行单元测试：
```bash
./build.sh --product-name {product_name} --build-target dcamera_tcp_adapter_unittest \
  --gn-args DCAMERA_TEST_ENABLE=true
```

### 环境变量配置

设置通信模式：
```bash
export DCAMERA_COMM_MODE=tcp  # 使用TCP模式（默认）
```

## 技术特性

- **宏保护**: 所有代码都使用 `DCAMERA_TEST_ENABLE` 宏保护，确保只在测试模式下编译
- **平台特定**: Windows/Winsock实现，用于本地开发环境
- **接口兼容**: 与生产环境的软总线适配器接口保持一致
- **独立构建**: 可作为独立测试库编译，不影响主模块

## 注意事项

1. **仅用于测试**: 这些文件仅用于开发和测试环境，不应在生产环境中使用
2. **编译隔离**: 需要显式启用 DCAMERA_TEST_ENABLE 宏才会被编译
3. **平台限制**: 当前实现仅支持Windows平台
4. **功能有限**: 实现了基本的通信功能，不支持完整的软总线特性

## 与主模块的关系

```
主模块 (distributed_camera_channel)
├── 生产代码 (始终编译)
│   ├── dcamera_softbus_adapter.cpp
│   ├── dcamera_softbus_session.cpp
│   └── ...
└── 测试代码 (DCAMERA_TEST_ENABLE时编译)
    └── 已移至 test/tcp_adapter/ 目录
```

## 未来改进

- [ ] 添加Linux平台支持
- [ ] 实现管道通信模式
- [ ] 添加更多单元测试用例
- [ ] 支持完整的软总线协议模拟
- [ ] 添加性能测试工具

## 维护者

如需更新或修改此测试模块，请确保：
1. 保持 `DCAMERA_TEST_ENABLE` 宏保护
2. 更新相关的BUILD.gn配置
3. 保持与生产代码接口的兼容性
4. 更新此README文档
