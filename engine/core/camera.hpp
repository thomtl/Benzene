#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <GLFW/glfw3.h>

#include "../libs/imgui/imgui.h"

namespace benzene
{
    class Camera {
        public:
        Camera(): pos{0, 0, 3}, front{0, 0, -1}, up{0, 1, 0} {}

        glm::mat4 get_view_matrix() const {
            return glm::lookAt(pos, pos + front, up);
        }

        glm::vec3 get_position() const {
            return pos;
        }

        void process_input(float delta_time){
            float speed = delta_time * camera_speed;
            if(ImGui::IsAnyWindowHovered())
                return;
            if(ImGui::IsKeyDown(GLFW_KEY_W))
                pos += speed * front;
            if(ImGui::IsKeyDown(GLFW_KEY_S))
                pos -= speed * front;
            if(ImGui::IsKeyDown(GLFW_KEY_A))
                pos -= glm::normalize(glm::cross(front, up)) * speed;
            if(ImGui::IsKeyDown(GLFW_KEY_D))
                pos += glm::normalize(glm::cross(front, up)) * speed;
        }

        private:
        static constexpr float camera_speed = 5.0f;

        glm::vec3 pos, front, up;
    };
} // namespace benzene
