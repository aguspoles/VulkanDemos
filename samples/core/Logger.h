#pragma once
#include <spdlog/spdlog.h>

namespace prm {

    class Log
    {
    public:
        static void Init();

        static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }

    private:
        static std::shared_ptr<spdlog::logger> s_CoreLogger;
    };

}

// Core log macros
#define LOGT(...)              ::prm::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define LOGI(...)              ::prm::Log::GetCoreLogger()->info(__VA_ARGS__)
#define LOGW(...)              ::prm::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define LOGE(...)              ::prm::Log::GetCoreLogger()->error(__VA_ARGS__)
#define LOGD(...)              ::prm::Log::GetCoreLogger()->debug(__VA_ARGS__)
#define LOGC(...)              ::prm::Log::GetCoreLogger()->critical(__VA_ARGS__)
