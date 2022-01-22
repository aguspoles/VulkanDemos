#include "pch.h"
#include "platform/WindowsPlatform.h"

#include "platform/FileSystem.h"
#include "platform/GlfwWindow.h"

namespace prm
{
    namespace
    {
        std::string get_temp_path_from_environment()
        {
            std::string temp_path = "temp/";

            TCHAR temp_buffer[MAX_PATH];
            DWORD temp_path_ret = GetTempPath(MAX_PATH, temp_buffer);
            if (temp_path_ret > MAX_PATH || temp_path_ret == 0)
            {
                temp_path = "temp/";
            }
            else
            {
                temp_path = std::string(temp_buffer) + "/";
            }

            return temp_path;
        }

        /// @brief Converts wstring to string using Windows specific function
        /// @param wstr Wide string to convert
        /// @return A converted utf8 string
        std::string wstr_to_str(const std::wstring& wstr)
        {
            if (wstr.empty())
            {
                return {};
            }

            auto wstr_len = static_cast<int>(wstr.size());
            auto str_len = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], wstr_len, NULL, 0, NULL, NULL);

            std::string str(str_len, 0);
            WideCharToMultiByte(CP_UTF8, 0, &wstr[0], wstr_len, &str[0], str_len, NULL, NULL);

            return str;
        }
    }        // namespace

    namespace fs
    {
        void create_directory(const std::string& path)
        {
            if (!is_directory(path))
            {
                CreateDirectory(path.c_str(), NULL);
            }
        }
    }        // namespace fs

    WindowsPlatform::WindowsPlatform(HINSTANCE hInstance, HINSTANCE hPrevInstance,
        PSTR lpCmdLine, INT nCmdShow)
    {
        if (!AllocConsole())
        {
            throw std::runtime_error{ "AllocConsole error" };
        }

        FILE* fp;
        freopen_s(&fp, "conin$", "r", stdin);
        freopen_s(&fp, "conout$", "w", stdout);
        freopen_s(&fp, "conout$", "w", stderr);

        Platform::SetTempDirectory(get_temp_path_from_environment());
    }

    const char* WindowsPlatform::GetSurfaceExtension()
    {
        return VK_KHR_WIN32_SURFACE_EXTENSION_NAME;
    }

    void WindowsPlatform::ICreateWindow(const Window::Properties& properties)
    {
        if (properties.mode == prm::Window::Mode::Headless)
        {
            //window = std::make_unique<HeadlessWindow>(properties);
        }
        else
        {
            m_Window = std::make_unique<GlfwWindow>(this, properties);
        }
    }
}  
