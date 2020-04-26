#pragma once

#include "libs/glad/include/glad/glad.h"
#include <GLFW/glfw3.h>

#include <benzene/benzene.hpp>

#include <vector>

#include "pipeline.hpp"

#include <glm/gtx/string_cast.hpp>

namespace benzene::opengl
{
    class Texture {
        public:
        Texture(): handle{0}, shader_name{} {}
        Texture(const benzene::Texture& tex): Texture{(size_t)tex.dimensions().first, (size_t)tex.dimensions().second, tex.bytes().data(), tex.get_shader_name(), tex.get_gamut()} {}
        Texture(size_t width, size_t height, const uint8_t* data, const std::string& shader_name, benzene::Texture::Gamut gamut): shader_name{shader_name} {
            glGenTextures(1, &handle);

            glBindTexture(GL_TEXTURE_2D, handle);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            glTexImage2D(GL_TEXTURE_2D, 0, (gamut == benzene::Texture::Gamut::Srgb) ? GL_SRGB : GL_RGB, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);
        }

        uint32_t operator()(){
            return handle;
        }

        void bind(Program& program, int i) const {
            GLint max_units;
            glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &max_units);
            assert(i < (max_units - 1)); // GL Current bound texture limit

            program.set_uniform("material." + shader_name, i); // Tell it to bind the uniform with the name "material.{shader_name}" to texture unit i

            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, handle);
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
            glGenVertexArrays(1, &vao);
            glBindVertexArray(vao);

            glGenBuffers(1, &vbo);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(benzene::Mesh::Vertex) * api_mesh.vertices.size(), api_mesh.vertices.data(), GL_STATIC_DRAW);

            glGenBuffers(1, &ebo);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32_t) * api_mesh.indices.size(), api_mesh.indices.data(), GL_STATIC_DRAW);
            
            auto enable_vertex_attribute = [&program](const std::string& name, size_t n, uintptr_t offset){
                auto location = program.get_vector_attrib_location(name);
                assert(location != -1);

                glEnableVertexAttribArray(location);
                glVertexAttribPointer(location, n, GL_FLOAT, GL_FALSE, sizeof(benzene::Mesh::Vertex), (void*)offset);
            };

            enable_vertex_attribute("inPosition", 3, offsetof(benzene::Mesh::Vertex, pos));
            enable_vertex_attribute("inNormal", 3, offsetof(benzene::Mesh::Vertex, normal));
            enable_vertex_attribute("inUv", 2, offsetof(benzene::Mesh::Vertex, uv));

            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(0);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

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

            program->use();
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
