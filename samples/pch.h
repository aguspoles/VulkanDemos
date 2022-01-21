#pragma once

#include <iostream>
#include <string>
#include <cstdint>
#include <memory>
#include <cassert>
#include <optional>
#include <functional>
#include <vector>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <chrono>
#include <ratio>
#include <map>
#include <unordered_map>
#include <typeindex>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <sys/stat.h>
#include <stdexcept>
#include <set>
#include <sstream>
#include <limits>
#include <numeric>
#include <mutex>
#include <stdexcept>

//Windows specific
#ifdef WIN32
#ifndef NOMINMAX
    #define NOMINMAX // See github.com/skypjack/entt/wiki/Frequently-Asked-Questions#warning-c4003-the-min-the-max-and-the-macro
#endif
#include <Windows.h>
#include <shellapi.h>
#endif

//vulkan
#include <vulkan/vulkan.hpp>



