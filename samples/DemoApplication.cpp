#include "pch.h"
#include "DemoApplication.h"

#include "platform/Platform.h"
#include "platform/InputEvents.h"
#include "render/Mesh.h"

namespace prm
{
    DemoApplication::DemoApplication()
    {
        SetName("Demo");
    }

    DemoApplication::~DemoApplication()
    {
        m_Renderer->GetRenderContext().r_Device.waitIdle();
        m_Mesh.reset();
        m_Renderer.reset();
    }

    bool DemoApplication::Prepare(Platform& _platform)
    {
        Application::Prepare(_platform);

        m_Renderer = std::make_unique<VulkanRenderer>(*m_Platform);

        m_Renderer->SetVertexShader("output/triangle_vert.spv");
        m_Renderer->SetFragmentShader("output/triangle_frag.spv");

        m_Renderer->Init();

        const auto& context = m_Renderer->GetRenderContext();
        const std::vector<Vertex> vertices = {
            {{0.f, -0.4f, 0.f}},
            {{0.4f, 0.4f, 0.f} },
            {{-0.4f, 0.4f, 0.f} }
        };
        m_Mesh = std::make_unique<Mesh>(context.r_GPU, context.r_Device, vertices);

        return true;
    }

    bool DemoApplication::Resize(const uint32_t width, const uint32_t height)
    {
        Application::Resize(width, height);
        if(width == 0 || height == 0)
        {
            return false;
        }
        m_Renderer->RecreateSwapchain();
        return true;
    }

    void DemoApplication::HandleInputEvent(const InputEvent& input_event)
    {
    }

    void DemoApplication::Update(float delta_time)
    {
        if (m_Platform->GetWindow().GetExtent().width == 0 || m_Platform->GetWindow().GetExtent().height == 0)
        {
            return;
        }

        m_Renderer->Draw(*m_Mesh);

        Application::Update(delta_time);
    }

}
