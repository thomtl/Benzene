#pragma once

#include "../base.hpp"

#include <vector>
#include <optional>
#include <cmath>

#include "../pipeline.hpp"
#include "mesh.hpp"

namespace benzene::opengl
{
    class Texture {
        public:
        Texture(): handle{0}, shader_name{} {}
        Texture(const benzene::Texture& tex): Texture{(size_t)tex.dimensions().first, (size_t)tex.dimensions().second, (size_t)tex.get_channels(), tex.bytes().data(), tex.get_shader_name(), tex.get_gamut()} {}
        Texture(size_t width, size_t height, size_t channels, const uint8_t* data, const std::string& shader_name, benzene::Texture::Gamut gamut);
        void clean();

        GLuint operator()(){
            return handle;
        }

        void bind(Program& program, size_t i) const;
        void set_parameter(GLenum key, GLint value);
        void set_parameter(GLenum key, GLfloat value);

        private:
        GLuint handle;
        std::string shader_name;

        static std::optional<float> max_anisotropy;
        static std::optional<size_t> max_texture_units;
    };

    class DrawMesh {
        public:
        DrawMesh(): mesh{}, textures{} {}
        DrawMesh(const benzene::Mesh& api_mesh, Program& program);
        void clean();

        void draw() const;

        private:
        void bind() const;

        Mesh<uint32_t, benzene::Mesh::Vertex> mesh;
        std::vector<opengl::Texture> textures;

        Program* program;
        const benzene::Mesh* api_mesh;
    };

    class Model {
        public:
        Model() {}
        Model(benzene::Model& model, Program& program);
        void clean();

        void draw() const;
        const benzene::Model& api_handle() const;

        private:
        benzene::Model* model;
        Program* program;
        std::vector<opengl::DrawMesh> meshes;
    };
} // namespace benzene::opengl
