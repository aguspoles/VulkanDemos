#pragma once
#include "platform/Application.h"
#include "render/VulkanRenderer.h"

namespace prm
{
    class Platform;
    class InputEvent;
    class Mesh;

    class DemoApplication : public Application
    {
    public:
        DemoApplication();

        ~DemoApplication();

        bool Prepare(Platform& platform) override;

        void Update(float delta_time) override;

        bool Resize(const uint32_t width, const uint32_t height) override;

        void HandleInputEvent(const InputEvent& input_event) override;

    private:
        std::unique_ptr<VulkanRenderer> m_Renderer;
        std::unique_ptr<Mesh> m_Mesh;
    };
}
