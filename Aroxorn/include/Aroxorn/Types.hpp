#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <string>

namespace Aroxorn {

    struct DeviceInfo {
        std::string name;
        uint32_t vendorID;
        uint32_t deviceID;
        // More capabilities can be added later (e.g., ray tracing support)
    };

}
