#pragma once

#include "../base.hpp"
#include "../pipeline.hpp"
#include "../model/batch.hpp"

#include "../../../core/camera.hpp"

namespace benzene::opengl
{
    class ForwardRenderer : public IRenderer {
        public:
        ForwardRenderer(int width, int height);
        ~ForwardRenderer();

        void draw(std::unordered_map<benzene::ModelId, benzene::Batch*>& batches, benzene::FrameData& frame_data);

        void framebuffer_resize_callback(size_t width, size_t height);

        private:
        Program main_program;
        std::unordered_map<ModelId, opengl::Batch> internal_batches;

        Camera camera;
    };
} // namespace benzene::opengl
