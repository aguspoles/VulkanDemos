#pragma once
#include "platform/Application.h"
#include "render/VulkanRenderer.h"
#include "scene/GameObject.h"
#include "scene/Camera.h"

namespace prm
{
    class Platform;
    class InputEvent;
    class Mesh;
    class Texture;

    class DemoApplication : public Application
    {
    public:
        DemoApplication();

        ~DemoApplication() override;

        bool Prepare(Platform& platform) override;

        void Update(float delta_time) override;

        bool Resize(const uint32_t width, const uint32_t height) override;

        void HandleInputEvent(const InputEvent& input_event) override;

    private:
        std::unique_ptr<VulkanRenderer> m_Renderer;
        std::shared_ptr<Mesh> m_Mesh;
        std::shared_ptr<Texture> m_Texture;
        std::vector<GameObject> m_GameObjects;
        std::vector<IRenderableObject*> m_RenderableObjects;
        Camera m_Camera;
        CameraMovement m_CurrentCameraMovement;
        bool m_ShouldMoveCamera;

        float m_DeltaTime{};

        float m_LastMouseX{};
        float m_LastMouseY{};
    };
}
