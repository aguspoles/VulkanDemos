#include "pch.h"
#include "DemoApplication.h"

#include "platform/Platform.h"
#include "platform/InputEvents.h"
#include "render/Mesh.h"

namespace 
{
    bool firstMouse = true;
}

namespace prm
{
    DemoApplication::DemoApplication()
    {
        SetName("Demo");
    }

    DemoApplication::~DemoApplication()
    {
        m_Renderer->GetRenderContext().r_Device.waitIdle();
        m_GameObjects.clear();
        m_Mesh.reset();
        m_Mesh2.reset();
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

        m_Mesh = Mesh::CreateModelFromFile(context.r_GPU, context.r_Device, m_Renderer->GetCommandPool(), "assets/meshes/teapot.obj");
        //m_Mesh2 = Mesh::CreateModelFromFile(context.r_GPU, context.r_Device, m_Renderer->GetCommandPool(), "output/smooth_vase.obj");

        auto go = GameObject::CreateGameObject();
        m_GameObjects.push_back(std::move(go));
        m_GameObjects[0].model = m_Mesh;
        m_GameObjects[0].transform.translation = { 0.f, 0.1f, 0.7f };
        m_GameObjects[0].transform.scale = { 0.1f, 0.1f, 0.1f };
        m_GameObjects[0].transform.rotation = { glm::radians(180.f), 0.f, 0.f };
        /*go = GameObject::CreateGameObject();
        m_GameObjects.push_back(std::move(go));
        m_GameObjects[1].model = m_Mesh2;
        m_GameObjects[1].transform.translation = { -0.2f, 0.18f, 0.6f };*/

        m_LastMouseX = (float)(m_Platform->GetWindow().GetExtent().width) / 2;
        m_LastMouseY = (float)(m_Platform->GetWindow().GetExtent().height) / 2;

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
        if (input_event.GetSource() == EventSource::Keyboard)
        {
            const auto& key_event = static_cast<const KeyInputEvent&>(input_event);

            if (key_event.GetAction() == KeyAction::Down)
                m_ShouldMoveCamera = true;
            else if (key_event.GetAction() == KeyAction::Up)
                m_ShouldMoveCamera = false;

            if (key_event.GetCode() == KeyCode::W)
            {
                m_CurrentCameraMovement = CameraMovement::FORWARD;
            }
            if (key_event.GetCode() == KeyCode::S)
            {
                m_CurrentCameraMovement = CameraMovement::BACKWARD;
            }
            if (key_event.GetCode() == KeyCode::A)
            {
                m_CurrentCameraMovement = CameraMovement::LEFT;
            }
            if (key_event.GetCode() == KeyCode::D)
            {
                m_CurrentCameraMovement = CameraMovement::RIGHT;
            }
        }

        if (input_event.GetSource() == EventSource::Mouse)
        {
            const auto& mouse_event = static_cast<const MouseButtonInputEvent&>(input_event);
            if (mouse_event.GetAction() == MouseAction::Move)
            {
                const float xpos = mouse_event.GetPosX();
                const float ypos = mouse_event.GetPosY();

                if (firstMouse)
                {
                    m_LastMouseX = xpos;
                    m_LastMouseY = ypos;
                    firstMouse = false;
                }

                const float xoffset = m_LastMouseX - xpos;
                const float yoffset = m_LastMouseY - ypos;

                m_LastMouseX = xpos;
                m_LastMouseY = ypos;

                m_Camera.ProcessMouseMovement(xoffset, yoffset);
            }
        }
    }

    void DemoApplication::Update(float delta_time)
    {
        m_DeltaTime = delta_time;

        if (m_Platform->GetWindow().GetExtent().width == 0 || m_Platform->GetWindow().GetExtent().height == 0)
        {
            return;
        }

        const float aspect = m_Renderer->GetAspectRatio();
        m_Camera.SetPerspectiveProjection(glm::radians(50.f), aspect, 0.1f, 100.f);
        if (m_ShouldMoveCamera)
        {
            m_Camera.Move(m_CurrentCameraMovement, m_DeltaTime);
        }

        m_Renderer->Draw(m_GameObjects, m_Camera);

        Application::Update(delta_time);
    }

}
