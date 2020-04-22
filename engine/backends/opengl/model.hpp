#pragma once

#include "libs/glad/include/glad/glad.h"
#include <GLFW/glfw3.h>

#include <benzene/benzene.hpp>

#include <vector>

#include "pipeline.hpp"

namespace benzene::opengl
{
    class Texture {
        public:
        Texture() {}

        Texture(benzene::Texture& tex): Texture{(size_t)tex.dimensions().first, (size_t)tex.dimensions().second, tex.bytes().data()} {}

        Texture(size_t width, size_t height, const uint8_t* data) {
            glGenTextures(1, &handle);

            glBindTexture(GL_TEXTURE_2D, handle);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
            //glGenerateMipmap(GL_TEXTURE_2D);
        }

        uint32_t operator()(){
            return handle;
        }

        void bind(int i){
            assert(i <= 31); // GL Current bound texture limit
            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, handle);
        }

        void clean(){
            glDeleteTextures(1, &handle);
        }

        private:
        uint32_t handle;
    };

    struct Vertex {
        glm::vec3 position, colour;
        glm::vec2 uv;
    };

    class Model {
        public:
        Model(): vao{0}, vbo{}, ebo{} {}
        Model(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indicies, benzene::Texture& tex, Program& program): tex{tex}, program{&program}, n_indicies{indicies.size()} {
            glGenVertexArrays(1, &vao);

            glGenBuffers(1, &vbo);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * vertices.size(), vertices.data(), GL_STATIC_DRAW);

            glGenBuffers(1, &ebo);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32_t) * indicies.size(), indicies.data(), GL_STATIC_DRAW);
            
            glBindVertexArray(vao);

            auto position_loc = program.get_vector_attrib_location("inPosition");
            glVertexAttribPointer(position_loc, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
            glEnableVertexAttribArray(position_loc);

            auto colour_loc = program.get_vector_attrib_location("inColour");
            glVertexAttribPointer(colour_loc, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, colour));
            glEnableVertexAttribArray(colour_loc);

            auto uv_loc = program.get_vector_attrib_location("inUv");
            glVertexAttribPointer(uv_loc, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
            glEnableVertexAttribArray(uv_loc);

            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(0);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        }

        void bind(){
            tex.bind(0);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
            glBindVertexArray(vao);
        }

        void draw(){
            program->use();
            this->bind();

            glDrawElements(GL_TRIANGLES, n_indicies, GL_UNSIGNED_INT, 0);
        }

        void clean(){
            glDeleteVertexArrays(1, &vao);
            glDeleteBuffers(1, &vbo);
            glDeleteBuffers(1, &ebo);

            tex.clean();
        }

        benzene::Model* model;

        private:
        uint32_t vao, vbo, ebo, shader_program;
        Texture tex;
        Program* program;

        size_t n_indicies;
    };
} // namespace benzene::opengl
