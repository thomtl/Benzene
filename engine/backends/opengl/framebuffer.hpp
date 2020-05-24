#pragma once

#include "base.hpp"

namespace benzene::opengl
{
    class Framebuffer {
        public:
        struct Attachment {
            enum class Container { Texture, Renderbuffer };
            enum class Type { Colour, Depth, Stencil, DepthStencil };
            Container container;
            Type type;
            GLenum format;
            int i = 0;
            bool multisampling = false;
            int samples = 1;
        };

        Framebuffer(size_t width, size_t height, const std::vector<Attachment>& attachments){
            glCreateFramebuffers(1, &handle);

            buffers.resize(attachments.size());
            for(size_t i = 0; i < buffers.size(); i++){
                GLenum attachment = 0;
                switch (attachments[i].type){
                    case Attachment::Type::Colour: attachment = GL_COLOR_ATTACHMENT0 + attachments[i].i; break;
                    case Attachment::Type::Depth: attachment = GL_DEPTH_ATTACHMENT; break;
                    case Attachment::Type::Stencil: attachment = GL_STENCIL_ATTACHMENT; break;
                    case Attachment::Type::DepthStencil: attachment = GL_DEPTH_STENCIL_ATTACHMENT; break;
                }

                GLuint buffer = 0;
                if(attachments[i].container == Attachment::Container::Texture){
                    buffer = this->create_texture_attachment(width, height, attachments[i]);
                    glNamedFramebufferTexture(handle, attachment, buffer, 0);
                } else if(attachments[i].container == Attachment::Container::Renderbuffer){
                    buffer = this->create_renderbuffer_attachment(width, height, attachments[i]);
                    glNamedFramebufferRenderbuffer(handle, attachment, GL_RENDERBUFFER, buffer);
                }
                
                buffers[i] = {attachments[i], buffer};
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
            for(auto& [attachment, buffer] : buffers){
                if(attachment.container == Attachment::Container::Texture)
                    glDeleteTextures(1, &buffer);
                else if(attachment.container == Attachment::Container::Renderbuffer)
                    glDeleteRenderbuffers(1, &buffer);
            }

            glDeleteFramebuffers(1, &handle);
        }

        GLuint operator()(){
            return handle;
        }

        private:
        GLuint create_texture_attachment(size_t width, size_t height, const Attachment& attachment){
            GLuint texture = 0;
            glCreateTextures(GL_TEXTURE_2D, 1, &texture);
            if(!attachment.multisampling)
                glTextureStorage2D(texture, 1, attachment.format, width, height);
            else
                glTextureStorage2DMultisample(texture, attachment.samples, attachment.format, width, height, false);
            
            glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            return texture;
        }

        GLuint create_renderbuffer_attachment(size_t width, size_t height, const Attachment& attachment){
            GLuint renderbuffer = 0;
            glCreateRenderbuffers(1, &renderbuffer);
            if(!attachment.multisampling)
                glNamedRenderbufferStorage(renderbuffer, attachment.format, width, height);
            else
                glNamedRenderbufferStorageMultisample(renderbuffer, attachment.samples, attachment.format, width, height);

            return renderbuffer;
        }

        std::vector<std::pair<Attachment, GLuint>> buffers;
        GLuint handle;
    };
} // namespace benzene::opengl
