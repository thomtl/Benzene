#pragma once

#include "base.hpp"

namespace benzene::opengl
{
    // TODO: Support RBOs (Render Buffer Object)
    class Framebuffer {
        public:
        struct Attachment {
            enum class Type { Colour, Depth, Stencil, DepthStencil };
            Type type;
            GLenum format;
            int i = 0;
        };

        Framebuffer(size_t width, size_t height, const std::vector<Attachment>& attachments){
            glCreateFramebuffers(1, &handle);

            images.resize(attachments.size(), 0);
            glCreateTextures(GL_TEXTURE_2D, images.size(), images.data());

            for(size_t i = 0; i < images.size(); i++){
                glTextureStorage2D(images[i], 1, attachments[i].format, width, height);

                glTextureParameteri(images[i], GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTextureParameteri(images[i], GL_TEXTURE_MAG_FILTER, GL_LINEAR);

                GLenum attachment = 0;
                switch (attachments[i].type){
                    case Attachment::Type::Colour: attachment = GL_COLOR_ATTACHMENT0 + attachments[i].i; break;
                    case Attachment::Type::Depth: attachment = GL_DEPTH_ATTACHMENT; break;
                    case Attachment::Type::Stencil: attachment = GL_STENCIL_ATTACHMENT; break;
                    case Attachment::Type::DepthStencil: attachment = GL_DEPTH_STENCIL_ATTACHMENT; break;
                }

                glNamedFramebufferTexture(handle, attachment, images[i], 0);
            }

            if constexpr (debug)
                assert(glCheckNamedFramebufferStatus(handle, GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
        }

        template<GLenum mode = GL_FRAMEBUFFER>
        void bind(){
            static_assert(mode == GL_FRAMEBUFFER || mode == GL_READ_FRAMEBUFFER || mode == GL_DRAW_FRAMEBUFFER);

            glBindFramebuffer(mode, handle);
        }

        void clean(){
            glDeleteTextures(images.size(), images.data());
            glDeleteFramebuffers(1, &handle);
        }

        uint32_t operator()(){
            return handle;
        }

        private:
        std::vector<uint32_t> images;
        uint32_t handle;
    };
} // namespace benzene::opengl
