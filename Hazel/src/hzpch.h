#pragma once

#include <iostream>
#include <filesystem>
#include <limits>
#include <memory>
#include <utility>
#include <algorithm>
#include <functional>

#include <string>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <unordered_set>

#include "Hazel/Log.h"

#ifdef HZ_PLATFORM_WINDOWS
	#include <Windows.h>

#elif defined(HZ_PLATFORM_LINUX)
	#include <unistd.h>
#endif