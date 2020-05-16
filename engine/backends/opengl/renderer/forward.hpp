#pragma once

#include "../base.hpp"
#include "../pipeline.hpp"
#include "../model/model.hpp"

#include "../../../core/camera.hpp"

namespace benzene::opengl
{
    class ForwardRenderer : public IRenderer {
        public:
        ForwardRenderer();
        ~ForwardRenderer();

        void draw(std::unordered_map<benzene::ModelId, benzene::Model*>& models, benzene::FrameData& frame_data);

        Program& program(){
            return main_program;
        }

        private:
        Program main_program;
        std::unordered_map<ModelId, opengl::Model> internal_models;

        Camera camera;
    };
} // namespace benzene::opengl
