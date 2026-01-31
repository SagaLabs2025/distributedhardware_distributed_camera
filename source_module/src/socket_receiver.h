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

#ifndef SOCKET_RECEIVER_H
#define SOCKET_RECEIVER_H

#include "distributed_camera_source.h"
#include <string>
#include <vector>
#include <thread>
#include <mutex>

/**
 * Socket接收端
 * 用于Source端接收Sink端发送的编码数据
 */
class SocketReceiver {
public:
    SocketReceiver();
    ~SocketReceiver();

    // 初始化
    int32_t Initialize(ISourceCallback* callback);

    // 启动服务器并开始接收
    int32_t StartReceiving();

    // 停止接收
    void StopReceiving();

    // 是否正在接收
    bool IsReceiving() const { return receiving_; }

private:
    void ReceiveThreadProc();
    int32_t StartServer();
    void StopServer();
    int32_t ReceiveData();

    int listenSockfd_;
    int connSockfd_;
    bool receiving_;
    bool initialized_;
    bool serverStarted_;

    std::thread receiveThread_;
    std::mutex mutex_;

    ISourceCallback* callback_;

    static const int DEFAULT_PORT = 8888;
};

#endif // SOCKET_RECEIVER_H
