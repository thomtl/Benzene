#pragma once

#include "libs/glad/include/glad/glad.h"
#include <GLFW/glfw3.h>

#include "../../core/format.hpp"
#include <stdexcept>

#include <string>
#include <cstring>
#include <vector>

#include <glm/gtc/type_ptr.hpp>

#include <iostream>

namespace benzene::opengl
{
    class Shader {
        public:
        Shader(GLenum kind, const std::string& src){
            handle = glCreateShader(kind);
            auto* src_ptr = src.c_str();
            glShaderSource(handle, 1, &src_ptr, NULL);
            glCompileShader(handle);

            int success;
            glGetShaderiv(handle, GL_COMPILE_STATUS, &success);
            if(success == GL_FALSE){
                GLsizei size;
                glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &size);

                std::string str{};
                str.resize(size);

                glGetShaderInfoLog(handle, size, NULL, str.data());
                print("benzene/opengl: Failed to compile shader, shader type: {:#x}\n Compiler Error: {:s}\n", (uint32_t)kind, str);

                throw std::runtime_error("benzene/opengl: Failed to compile shader");
            }
        }

        void clean(){
            glDeleteShader(handle);
        }

        uint32_t operator()() const {
            return handle;
        }

        private:
        uint32_t handle;
    };
    
    class Program {
        public:
        void add_shader(GLenum kind, const std::string& src){
            shaders.emplace_back(kind, src);
        }

        void compile(){
            handle = glCreateProgram();

            for(const auto& shader : shaders)
                glAttachShader(handle, shader());

            glLinkProgram(handle);

            int success;
            glGetProgramiv(handle, GL_LINK_STATUS, &success);
            if(success == GL_FALSE){
                GLsizei size;
                glGetProgramiv(handle, GL_INFO_LOG_LENGTH, &size);

                std::string str{};
                str.resize(size);

                glGetProgramInfoLog(handle, size, NULL, str.data());
                print("benzene/opengl: Failed to compile shader program {:s}\n", str);

                throw std::runtime_error("benzene/opengl: Failed to compile shader program");
            }

            for(auto& shader : shaders)
                shader.clean(); // Not needed after this
        }

        void set_uniform(const std::string& name, glm::mat4 matrix){
            auto loc = glGetUniformLocation(handle, name.c_str());
            glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(matrix));
        }

        void set_uniform(const std::string& name, int i){
            auto loc = glGetUniformLocation(handle, name.c_str());
            glUniform1i(loc, i);
        }

        void set_uniform(const std::string& name, glm::vec2 vec){
            auto loc = glGetUniformLocation(handle, name.c_str());
            glUniform2f(loc, vec.x, vec.y);
        }

        uint32_t get_vector_attrib_location(const std::string& name){
            return glGetAttribLocation(handle, name.c_str());
        }

        void clean(){
            glDeleteProgram(handle);
        }

        void use(){
            glUseProgram(handle);
        }

        uint32_t operator()(){
            return handle;
        }

        private:
        uint32_t handle;
        std::vector<Shader> shaders;
    };
} // namespace benzene::opengl
