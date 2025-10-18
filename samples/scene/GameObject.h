#pragma once
#include "core/glm_defs.h"
#include "render/Mesh.h"
#include "render/RenderableObject.h"

namespace prm {

    struct TransformComponent
    {
        glm::vec3 translation{};
        glm::vec3 scale{ 1.f, 1.f, 1.f };
        glm::vec3 rotation{};

        // 
        // Matrix corrsponds to Translate * Ry * Rx * Rz * Scale
        // Rotations correspond to Tait-bryan angles of Y(1), X(2), Z(3)
        // https://en.wikipedia.org/wiki/Euler_angles#Rotation_matrix
        glm::mat4 mat4() const
        {
            glm::mat4 translateMatrix(1.0);
            glm::mat4 rotateMatrix(1.0);
            glm::mat4 scaleMatrix(1.0);
            translateMatrix = glm::translate(translateMatrix, translation);
            rotateMatrix = glm::rotate(rotateMatrix, rotation.x, { 1,0,0 });
            rotateMatrix = glm::rotate(rotateMatrix, rotation.y, { 0,1,0 });
            rotateMatrix = glm::rotate(rotateMatrix, rotation.z, { 0,0,1 });
            scaleMatrix = glm::scale(scaleMatrix, scale);
            return translateMatrix*rotateMatrix*scaleMatrix;
            /*const float c3 = glm::cos(rotation.z);
            const float s3 = glm::sin(rotation.z);
            const float c2 = glm::cos(rotation.x);
            const float s2 = glm::sin(rotation.x);
            const float c1 = glm::cos(rotation.y);
            const float s1 = glm::sin(rotation.y);
            return glm::mat4{
                {
                    scale.x * (c1 * c3 + s1 * s2 * s3),
                    scale.x * (c2 * s3),
                    scale.x * (c1 * s2 * s3 - c3 * s1),
                    0.0f,
                },
                {
                    scale.y * (c3 * s1 * s2 - c1 * s3),
                    scale.y * (c2 * c3),
                    scale.y * (c1 * c3 * s2 + s1 * s3),
                    0.0f,
                },
                {
                    scale.z * (c2 * s1),
                    scale.z * (-s2),
                    scale.z * (c1 * c2),
                    0.0f,
                },
                {translation.x, translation.y, translation.z, 1.0f} };*/
        }
    };

    class GameObject : public IRenderableObject{
    public:
        using id_t = uint32_t;

        static GameObject CreateGameObject()
        {
            static id_t currentId = 0;
            return GameObject{ currentId++ };
        }

        GameObject(const GameObject&) = delete;
        GameObject& operator=(const GameObject&) = delete;
        GameObject(GameObject&&) = default;
        GameObject& operator=(GameObject&&) = default;

        void Render(vk::CommandBuffer commandBuffer, const Camera& camera, vk::PipelineLayout pipelineLayout) const override;

        id_t getId() { return m_Id; }

        std::shared_ptr<Mesh> model{};
        glm::vec3 color{};
        TransformComponent transform{};

    private:
        GameObject(id_t objId) : m_Id{ objId } {}

        id_t m_Id;
    };
}  