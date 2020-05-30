#pragma once

#include "base.hpp"

namespace benzene::opengl {
    template<GLenum target>
    class Buffer {
        public:
        static_assert(target == GL_ARRAY_BUFFER || target == GL_ATOMIC_COUNTER_BUFFER || target == GL_COPY_READ_BUFFER || target == GL_COPY_WRITE_BUFFER || 
                      target == GL_DISPATCH_INDIRECT_BUFFER || target == GL_DRAW_INDIRECT_BUFFER || target == GL_ELEMENT_ARRAY_BUFFER || target == GL_PIXEL_PACK_BUFFER || 
                      target == GL_PIXEL_UNPACK_BUFFER || target == GL_QUERY_BUFFER || target == GL_SHADER_STORAGE_BUFFER || target == GL_TEXTURE_BUFFER || 
                      target == GL_TRANSFORM_FEEDBACK_BUFFER || target == GL_UNIFORM_BUFFER);

        Buffer(): size{0}, handle{0}, storage_flags{0}, mapped_addr{} {}
        Buffer(size_t size, const void* data = nullptr, GLbitfield flags = GL_DYNAMIC_STORAGE_BIT | GL_MAP_READ_BIT | GL_MAP_WRITE_BIT): size{size}, storage_flags{flags} {
            if(flags & GL_MAP_PERSISTENT_BIT)
                assert(flags & GL_MAP_READ_BIT || flags & GL_MAP_WRITE_BIT);

            if(flags & GL_MAP_WRITE_BIT)
                assert(flags & GL_MAP_PERSISTENT_BIT);
            
            glCreateBuffers(1, &handle);
            glNamedBufferStorage(handle, size, data, storage_flags);
        }

        void clean(){
            glDeleteBuffers(1, &handle);
        }

        void bind(){
            glBindBuffer(target, handle);
        }

        void bind_base(GLint binding){
            static_assert(target == GL_ATOMIC_COUNTER_BUFFER || target == GL_TRANSFORM_FEEDBACK_BUFFER || target == GL_UNIFORM_BUFFER || target == GL_SHADER_STORAGE_BUFFER);
            glBindBufferBase(target, binding, handle);
        }

        GLuint operator()(){
            return handle;
        }

        void* map(){
            if(!mapped_addr)
                mapped_addr = glMapNamedBufferRange(handle, 0, size, storage_flags &= ~GL_DYNAMIC_STORAGE_BIT);
                
            return *mapped_addr;
        }

        void unmap(){
            glUnmapNamedBuffer(handle);
            mapped_addr.reset();
        }

        void flush(size_t offset = 0){ // Since we can't use a member variable as default argument
            flush(offset, this->size);
        }

        void flush(size_t offset, size_t size){
            glFlushMappedNamedBufferRange(handle, 0, size);
        }

        void write(void* data){ // Since we can't use a member variable as default argument
            write(data, 0, size);
        }

        void write(void* data, size_t offset, size_t size){
            if(!storage_flags & GL_DYNAMIC_STORAGE_BIT)
                throw std::runtime_error("opengl/Buffer: Can't write to non-GL_DYNAMIC_STORAGE_BIT after creation");
            glNamedBufferSubData(handle, offset, size, data);
        }

        private:
        size_t size;
        GLuint handle;
        GLbitfield storage_flags;

        std::optional<void*> mapped_addr;
    };
}