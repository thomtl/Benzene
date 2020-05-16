#pragma once

#include "base.hpp"

namespace benzene::opengl
{
    struct VertexAttribute {
        GLuint location;
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
            glCreateBuffers(1, &vbo);
            glNamedBufferStorage(vbo, sizeof(Vertex) * vertices.size(), vertices.data(), GL_DYNAMIC_STORAGE_BIT);

            glCreateBuffers(1, &ebo);
            glNamedBufferStorage(ebo, sizeof(Index) * indices.size(), indices.data(), GL_DYNAMIC_STORAGE_BIT);

            glCreateVertexArrays(1, &vao);

            glVertexArrayVertexBuffer(vao, 0, vbo, 0, sizeof(Vertex));
            glVertexArrayElementBuffer(vao, ebo);

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
