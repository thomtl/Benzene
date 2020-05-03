#pragma once

#include "libs/glad/include/glad/glad.h"
#include <GLFW/glfw3.h>

#include "../../core/format.hpp"
#include <stdexcept>

#include <string>
#include <cstring>
#include <vector>

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
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

            glValidateProgram(handle);
            glGetProgramiv(handle, GL_VALIDATE_STATUS, &success);
            if(success == GL_FALSE){
                GLsizei size;
                glGetProgramiv(handle, GL_INFO_LOG_LENGTH, &size);

                std::string str{};
                str.resize(size);

                glGetProgramInfoLog(handle, size, NULL, str.data());
                print("benzene/opengl: Failed to validate shader program {:s}\n", str);

                throw std::runtime_error("benzene/opengl: Failed to validate shader program");
            }

            for(auto& shader : shaders){
                glDetachShader(handle, shader());
                shader.clean(); // Not needed after this
            }
        }

        void set_uniform(const std::string& name, glm::mat4 matrix){
            auto loc = this->get_uniform_location(name);
            glProgramUniformMatrix4fv(handle, loc, 1, GL_FALSE, glm::value_ptr(matrix));
        }

        void set_uniform(const std::string& name, glm::mat3 matrix){
            /*
            For some reason when using `GLM_FORCE_DEFAULT_ALIGNED_GENTYPES` `glm::value_ptr` doesn't work on `glm::mat3`s and outputs corrupted data
            so we create our own temporary array
            see: https://stackoverflow.com/questions/61447393/glmvalue-ptr-broken-for-glmmat3 for future reference
            */
            float tmp[3][3] = {};
            for(int i = 0; i < 3; i++)
                for(int j = 0; j < 3; j++)
                    tmp[i][j] = matrix[i][j];
            
            auto loc = this->get_uniform_location(name);
            glProgramUniformMatrix3fv(handle, loc, 1, GL_FALSE, (const GLfloat*)tmp);
        }

        void set_uniform(const std::string& name, int i){
            auto loc = this->get_uniform_location(name);
            glProgramUniform1i(handle, loc, i);
        }

        void set_uniform(const std::string& name, float f){
            auto loc = this->get_uniform_location(name);
            glProgramUniform1f(handle, loc, f);
        }

        void set_uniform(const std::string& name, glm::vec2 vec){
            auto loc = this->get_uniform_location(name);
            glProgramUniform2f(handle, loc, vec.x, vec.y);
        }

        void set_uniform(const std::string& name, glm::vec3 vec){
            auto loc = this->get_uniform_location(name);
            glProgramUniform3f(handle, loc, vec.x, vec.y, vec.z);
        }

        void set_uniform(const std::string& name, glm::vec4 vec){
            auto loc = this->get_uniform_location(name);
            glProgramUniform4f(handle, loc, vec.x, vec.y, vec.z, vec.w);
        }

        int32_t get_vector_attrib_location(const std::string& name){
            auto ret = (int32_t)glGetAttribLocation(handle, name.c_str());
            if(ret == -1){
                print("opengl: \"{:s}\" is not a valid vertex attribute\n", name);
                throw std::runtime_error("opengl: Couldn't find vertex attribute");
            }
            return ret;
        }

        void clean(){
            glDeleteProgram(handle);
        }

        void bind(){
            glUseProgram(handle);
        }

        uint32_t operator()(){
            return handle;
        }

        private:
        uint32_t get_uniform_location(const std::string& name){
            auto ret = glGetUniformLocation(handle, name.c_str());
            if(ret == -1){
                print("opengl: \"{:s}\" is not a valid uniform\n", name);
                throw std::runtime_error("opengl: Couldn't find uniform");
            }

            return ret;
        }
        
        uint32_t handle;
        std::vector<Shader> shaders;
    };
} // namespace benzene::opengl
