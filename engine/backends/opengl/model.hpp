#pragma once

#include "libs/glad/include/glad/glad.h"
#include <GLFW/glfw3.h>

#include <benzene/benzene.hpp>

#include <vector>

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

        void bind(){
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, handle);
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
        Model(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indicies, benzene::Texture& tex): tex{tex}, n_indicies{indicies.size()} {
            glGenVertexArrays(1, &vao);

            glGenBuffers(1, &vbo);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * vertices.size(), vertices.data(), GL_STATIC_DRAW);

            glGenBuffers(1, &ebo);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32_t) * indicies.size(), indicies.data(), GL_STATIC_DRAW);
            //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

            
            glBindVertexArray(vao);

            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
            glEnableVertexAttribArray(0);

            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, colour));
            glEnableVertexAttribArray(1);

            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
            glEnableVertexAttribArray(2);

            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(0);
        }

        void bind(){
            tex.bind();
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBindVertexArray(vao);
        }

        void draw(){
            this->bind();

            glDrawElements(GL_TRIANGLES, n_indicies, GL_UNSIGNED_INT, 0);
        }

        benzene::Model* model;

        private:
        uint32_t vao, vbo, ebo;
        Texture tex;

        size_t n_indicies;
    };
} // namespace benzene::opengl
