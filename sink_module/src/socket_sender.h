/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef SOCKET_SENDER_H
#define SOCKET_SENDER_H

#include "distributed_camera_source.h"
#include <string>
#include <vector>
#include <functional>
#include <memory>

// 前向声明
struct sockaddr_in;

/**
 * Socket发送端
 * 用于Sink端发送编码数据给Source端
 * 模拟 SoftBus 三通道通信
 */
class SocketSender {
public:
    SocketSender();
    ~SocketSender();

    // 初始化
    int32_t Initialize();

    // 连接到Source端
    int32_t ConnectToSource(const std::string& host, int port);

    // 发送编码数据 (Continuous通道)
    int32_t SendData(const std::vector<uint8_t>& data);

    // 发送控制消息 (Control通道)
    int32_t SendControlMessage(const std::string& message);

    // 关闭连接
    void Close();

    // 是否已连接
    bool IsConnected() const { return connected_; }

private:
    int32_t SendDataInternal(const void* data, int size);

    int sockfd_;
    bool connected_;
    bool initialized_;
};

#endif // SOCKET_SENDER_H
