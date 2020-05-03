#pragma once

#include "base.hpp"

#include <vector>

#include "pipeline.hpp"

#include <glm/gtx/string_cast.hpp>

namespace benzene::opengl
{
    class Texture {
        public:
        Texture(): handle{0}, shader_name{} {}
        Texture(const benzene::Texture& tex): Texture{(size_t)tex.dimensions().first, (size_t)tex.dimensions().second, (size_t)tex.get_channels(), tex.bytes().data(), tex.get_shader_name(), tex.get_gamut()} {}
        Texture(size_t width, size_t height, size_t channels, const uint8_t* data, const std::string& shader_name, benzene::Texture::Gamut gamut): shader_name{shader_name} {
            glCreateTextures(GL_TEXTURE_2D, 1, &handle);

            glTextureParameteri(handle, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
            glTextureParameteri(handle, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
            glTextureParameteri(handle, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTextureParameteri(handle, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            auto internal_format = (gamut == benzene::Texture::Gamut::Srgb) ? GL_SRGB8 : GL_RGB8;
            glTextureStorage2D(handle, 1, internal_format, width, height);

            auto format = GL_RGBA;
            switch (channels){
            case 3: format = GL_RGB; break;
            case 4: format = GL_RGBA; break;
            default:
                print("opengl/Texture: Unknown channel count {:d}\n", channels);
                throw std::runtime_error("opengl/Texture: Unknown channel count");
                break;
            }
            glTextureSubImage2D(handle, 0, 0, 0, width, height, format, GL_UNSIGNED_BYTE, data);

            glGenerateTextureMipmap(handle);
        }

        uint32_t operator()(){
            return handle;
        }

        void bind(Program& program, int i) const {
            if(debug){
                GLint max_units;
                glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &max_units);
                assert(i < (max_units - 1)); // Test if we're going over the limit        
            }

            program.set_uniform("material." + shader_name, i); // Tell it to bind the uniform with the name "material.{shader_name}" to texture unit i
            glBindTextureUnit(i, handle);
        }

        void clean(){
            glDeleteTextures(1, &handle);
        }

        private:
        uint32_t handle;
        std::string shader_name;
    };

    class Mesh {
        public:
        Mesh(): vao{0}, vbo{0}, ebo{0} {}
        Mesh(const benzene::Mesh& api_mesh, Program& program): n_indicies{api_mesh.indices.size()}, program{&program}, api_mesh{&api_mesh} {
            glCreateBuffers(1, &vbo);
            glNamedBufferStorage(vbo, sizeof(benzene::Mesh::Vertex) * api_mesh.vertices.size(), api_mesh.vertices.data(), GL_DYNAMIC_STORAGE_BIT);

            glCreateBuffers(1, &ebo);
            glNamedBufferStorage(ebo, sizeof(uint32_t) * api_mesh.indices.size(), api_mesh.indices.data(), GL_DYNAMIC_STORAGE_BIT);

            glCreateVertexArrays(1, &vao);

            glVertexArrayVertexBuffer(vao, 0, vbo, 0, sizeof(benzene::Mesh::Vertex));
            glVertexArrayElementBuffer(vao, ebo);

            auto vertex_attribute = [&program, this](const std::string& name, size_t n, uintptr_t offset){
                auto location = program.get_vector_attrib_location(name);
                assert(location != -1);

                glEnableVertexArrayAttrib(vao, location); // Enable the location, so it provides the dynamic data and not the static one
                glVertexArrayAttribFormat(vao, location, n, GL_FLOAT, GL_FALSE, offset); // Tell it how to find the data
                glVertexArrayAttribBinding(vao, location, 0); // This attribute is in VBO 0, number must be thesame as the one in the glVertexArrayVertexBuffer call
            };

            vertex_attribute("inPosition", 3, offsetof(benzene::Mesh::Vertex, pos));
            vertex_attribute("inNormal", 3, offsetof(benzene::Mesh::Vertex, normal));
            vertex_attribute("inTangent", 3, offsetof(benzene::Mesh::Vertex, tangent));
            vertex_attribute("inUv", 2, offsetof(benzene::Mesh::Vertex, uv));

            for(const auto& texture : api_mesh.textures)
                this->textures.emplace_back(texture);
        }

        void draw() const {
            this->bind();
            glDrawElements(GL_TRIANGLES, n_indicies, GL_UNSIGNED_INT, 0);
        }

        void clean(){
            glDeleteVertexArrays(1, &vao);
            glDeleteBuffers(1, &vbo);
            glDeleteBuffers(1, &ebo);

            for(auto& texture : textures)
                texture.clean();
        }

        private:
        void bind() const {
            for(size_t i = 0; i < textures.size(); i++)
                textures[i].bind(*program, i);
            
            glBindVertexArray(vao);
            program->bind();
            program->set_uniform("material.shininess", api_mesh->material.shininess);
        }

        uint32_t vao, vbo, ebo;
        std::vector<opengl::Texture> textures;

        size_t n_indicies;
        Program* program;
        const benzene::Mesh* api_mesh;
    };

    class Model {
        public:
        Model() {}
        Model(benzene::Model& model, Program& program): model{&model}, program{&program} {
            for(auto& mesh : model.meshes)
		        meshes.emplace_back(mesh, program);
        }

        void draw() const {
            auto translate = glm::translate(glm::mat4{1.0f}, model->pos);
		    auto scale = glm::scale(glm::mat4{1.0f}, model->scale);
            auto rotate = glm::rotate(glm::mat4{1.0f}, glm::radians(model->rotation.y), glm::vec3{0.0f, 1.0f, 0.0f});
		    rotate = glm::rotate(rotate, glm::radians(model->rotation.z), glm::vec3{0.0f, 0.0f, 1.0f});
		    rotate = glm::rotate(rotate, glm::radians(model->rotation.x), glm::vec3{1.0f, 0.0f, 0.0f});
            
            auto model_matrix = scale * translate * rotate;
            auto normal_matrix = glm::mat3{glm::transpose(glm::inverse(model_matrix))};

            program->bind();

            program->set_uniform("modelMatrix", model_matrix);
            program->set_uniform("normalMatrix", normal_matrix);

            for(const auto& mesh : meshes)
                mesh.draw();
        }

        benzene::Model& api_handle() const {
            return *model;
        }

        void clean(){
            for(auto& mesh : meshes)
                mesh.clean();
        }

        private:
        benzene::Model* model;
        Program* program;
        std::vector<opengl::Mesh> meshes;
    };
} // namespace benzene::opengl
