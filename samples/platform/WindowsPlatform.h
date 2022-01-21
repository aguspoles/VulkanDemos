#pragma once
#include <Windows.h>
#include "platform/Platform.h"

namespace prm
{
    class WindowsPlatform : public Platform
    {
    public:
        WindowsPlatform(HINSTANCE hInstance, HINSTANCE hPrevInstance,
            PSTR lpCmdLine, INT nCmdShow);

        virtual ~WindowsPlatform() = default;

        const char* GetSurfaceExtension() override;

    protected:
        void ICreateWindow(const Window::Properties& properties) override;
    };
}
