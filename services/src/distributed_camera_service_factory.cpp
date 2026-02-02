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

#include "distributed_camera_service.h"
#include "distributed_camera_source_impl.h"
// #include "distributed_camera_sink_impl.h"  // 暂时注释，简化测试

namespace OHOS {
namespace DistributedHardware {

std::shared_ptr<IDistributedCameraSource> DistributedCameraServiceFactory::CreateSourceService() {
    return std::make_shared<DistributedCameraSourceImpl>();
}

std::shared_ptr<IDistributedCameraSink> DistributedCameraServiceFactory::CreateSinkService() {
    // 暂时返回nullptr，简化测试
    return nullptr;
    // return std::make_shared<DistributedCameraSinkImpl>();
}

} // namespace DistributedHardware
} // namespace OHOS