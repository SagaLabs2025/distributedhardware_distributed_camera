/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "platform_interface.h"
#include <vector>

namespace OHOS {
namespace DistributedHardware {

class DataBufferImpl : public IDataBuffer {
public:
    explicit DataBufferImpl(size_t size = 0) {
        if (size > 0) {
            buffer_.resize(size);
            isValid_ = true;
        } else {
            isValid_ = false;
        }
    }

    DataBufferImpl(const uint8_t* data, size_t size) {
        if (data != nullptr && size > 0) {
            buffer_.assign(data, data + size);
            isValid_ = true;
        } else {
            isValid_ = false;
        }
    }

    ~DataBufferImpl() override = default;

    uint8_t* Data() override {
        return isValid_ ? buffer_.data() : nullptr;
    }

    const uint8_t* ConstData() const override {
        return isValid_ ? buffer_.data() : nullptr;
    }

    size_t Size() const override {
        return isValid_ ? buffer_.size() : 0;
    }

    void Resize(size_t newSize) override {
        if (newSize == 0) {
            buffer_.clear();
            isValid_ = false;
        } else {
            buffer_.resize(newSize);
            isValid_ = true;
        }
    }

    bool IsValid() const override {
        return isValid_;
    }

private:
    std::vector<uint8_t> buffer_;
    bool isValid_ = true;
};

// Factory function to create data buffer
std::shared_ptr<IDataBuffer> CreateDataBuffer(size_t initialSize) {
    return std::make_shared<DataBufferImpl>(initialSize);
}

} // namespace DistributedHardware
} // namespace OHOS