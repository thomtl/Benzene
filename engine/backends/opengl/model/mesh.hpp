#pragma once

#include "../base.hpp"

namespace benzene::opengl
{
    struct VertexAttribute {
        GLint location;
        GLenum type;
        uintptr_t offset;
        uint32_t n = 1;
        bool normalized = false;
    };

    template<typename Index, typename Vertex>
    class Mesh {
        public:
        Mesh(): index_count{0}, vao{0}, ebo{0}, vbo{0} {}
        Mesh(const std::vector<Index>& indices, const std::vector<Vertex>& vertices, const std::vector<VertexAttribute>& attributes): index_count{indices.size()} {
            GLenum buffer_flags = GL_DYNAMIC_STORAGE_BIT | (debug) ? GL_MAP_READ_BIT : 0; // Allow buffer to be mappedas readonly in debug mode, for apitrace
            glCreateBuffers(1, &vbo);
            glNamedBufferStorage(vbo, sizeof(Vertex) * vertices.size(), vertices.data(), buffer_flags);

            glCreateBuffers(1, &ebo);
            glNamedBufferStorage(ebo, sizeof(Index) * indices.size(), indices.data(), buffer_flags);

            glCreateVertexArrays(1, &vao);

            glVertexArrayVertexBuffer(vao, 0, vbo, 0, sizeof(Vertex)); // Bind VBO0 to VAO, //TODO: Allow the user to specify N VBOs
            glVertexArrayElementBuffer(vao, ebo); // Bind index buffer to VAO

            auto vertex_attribute = [this](const VertexAttribute& attr){
                glEnableVertexArrayAttrib(vao, attr.location); // Enable the location, so it provides the dynamic data and not the static one
                glVertexArrayAttribFormat(vao, attr.location, attr.n, attr.type, attr.normalized, attr.offset); // Tell it how to find the data
                glVertexArrayAttribBinding(vao, attr.location, 0); // This attribute is in VBO 0, number must be thesame as the one in the glVertexArrayVertexBuffer call
            };

            for(const auto& attr : attributes)
                vertex_attribute(attr);
        }

        void clean(){
            glDeleteVertexArrays(1, &vao);

            glDeleteBuffers(1, &vbo);
            glDeleteBuffers(1, &ebo);
        }

        template<GLenum mode = GL_TRIANGLES>
        void draw() const {
            this->bind();
            glDrawElements(mode, index_count, gl::type_to_enum_v<Index>, nullptr);
        }

        private:
        void bind() const {
            glBindVertexArray(vao);
        }
        
        size_t index_count;
        GLuint vao, ebo, vbo;
    };
} // namespace benzene::opengl
