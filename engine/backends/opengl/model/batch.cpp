#include "batch.hpp"

using namespace benzene::opengl;

#pragma region Texture

std::optional<float> Texture::max_anisotropy;
std::optional<size_t> Texture::max_texture_units;

Texture::Texture(size_t width, size_t height, size_t channels, const uint8_t* data, const std::string& shader_name, benzene::Texture::Gamut gamut): shader_name{shader_name} {
    glCreateTextures(GL_TEXTURE_2D, 1, &handle);

    this->set_parameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    this->set_parameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    this->set_parameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    this->set_parameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    if((GL_ARB_texture_filter_anisotropic || GL_EXT_texture_filter_anisotropic) && !max_anisotropy.has_value()){
        static_assert(GL_MAX_TEXTURE_MAX_ANISOTROPY == GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, "Support both ARB and EXT anisotropy extensions");
        float max = 0.0f;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &max);
        this->max_anisotropy = max;
    }

    if(max_anisotropy.has_value()){
        static_assert(GL_TEXTURE_MAX_ANISOTROPY == GL_TEXTURE_MAX_ANISOTROPY_EXT, "Support both ARB and EXT anisotropy extensions");

        this->set_parameter(GL_TEXTURE_MAX_ANISOTROPY, *this->max_anisotropy);
    }

    auto internal_format = (gamut == benzene::Texture::Gamut::Srgb) ? GL_SRGB8 : GL_RGB8;
    auto mip_levels = (size_t)std::floor(std::log2(std::max(width, height))) + 1;
    glTextureStorage2D(handle, mip_levels, internal_format, width, height);

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

void Texture::clean(){
    glDeleteTextures(1, &handle);
}

void Texture::bind(Program& program, size_t i) const {
    if(!max_texture_units.has_value()){
        GLint max_units;
        glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &max_units);
        max_texture_units = max_units;
    }

    assert(i < (*max_texture_units - 1)); // Test if we're going over the limit        

    program.set_uniform("material." + shader_name, (int)i); // Tell it to bind the uniform with the name "material.{shader_name}" to texture unit i
    glBindTextureUnit(i, handle);
}

void Texture::set_parameter(GLenum key, GLint value){
    glTextureParameteri(handle, key, value);
}

void Texture::set_parameter(GLenum key, GLfloat value){
    glTextureParameterf(handle, key, value);
}

#pragma endregion

#pragma region DrawMesh

DrawMesh::DrawMesh(const benzene::Mesh& api_mesh, Program& program): program{&program}, api_mesh{&api_mesh} {
    mesh = Mesh{api_mesh.indices, api_mesh.vertices, {
        {.location = program.get_vertex_attrib_location("inPosition"), .type = gl::type_to_enum_v<float>, .offset = offsetof(benzene::Mesh::Vertex, pos), .n = 3},
        {.location = program.get_vertex_attrib_location("inNormal"), .type = gl::type_to_enum_v<float>, .offset = offsetof(benzene::Mesh::Vertex, normal), .n = 3},
        {.location = program.get_vertex_attrib_location("inTangent"), .type = gl::type_to_enum_v<float>, .offset = offsetof(benzene::Mesh::Vertex, tangent), .n = 3},
        {.location = program.get_vertex_attrib_location("inUv"), .type = gl::type_to_enum_v<float>, .offset = offsetof(benzene::Mesh::Vertex, uv), .n = 2}
    }};

    for(const auto& texture : api_mesh.textures)
        this->textures.emplace_back(texture);
}

void DrawMesh::clean() {
    mesh.clean();

    for(auto& texture : textures)
        texture.clean();
}

void DrawMesh::draw() const {
    this->bind();
    mesh.draw();
}

void DrawMesh::bind() const {
    for(size_t i = 0; i < textures.size(); i++)
        textures[i].bind(*program, i);

    program->bind();
    program->set_uniform("material.shininess", api_mesh->material.shininess);

    mesh.bind();
}

gl::DrawCommand DrawMesh::draw_command() const {
    return mesh.draw_command();
}

#pragma endregion

#pragma region Model

Batch::Batch(benzene::Batch& batch, Program& program): batch{&batch}, program{&program} {
    per_instance_buffer = Buffer<GL_SHADER_STORAGE_BUFFER>(batch.transforms.size() * sizeof(gl::InstanceData), nullptr, GL_DYNAMIC_STORAGE_BIT | GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
    for(auto& mesh : batch.meshes)
		meshes.emplace_back(mesh, program);
}

void Batch::clean(){
    for(auto& mesh : meshes)
        mesh.clean();

    per_instance_buffer.clean();
}

void Batch::draw() const {
    auto* instance_data = (gl::InstanceData*)per_instance_buffer.map();

    #pragma omp parallel for
    for(size_t i = 0; i < batch->transforms.size(); i++){
        auto& transform = batch->transforms[i];
        auto translate = glm::translate(glm::mat4{1.0f}, transform.pos);
	    auto scale = glm::scale(glm::mat4{1.0f}, transform.scale);
        auto rotate = glm::rotate(glm::mat4{1.0f}, glm::radians(transform.rotation.y), glm::vec3{0.0f, 1.0f, 0.0f});
	    rotate = glm::rotate(rotate, glm::radians(transform.rotation.z), glm::vec3{0.0f, 0.0f, 1.0f});
	    rotate = glm::rotate(rotate, glm::radians(transform.rotation.x), glm::vec3{1.0f, 0.0f, 0.0f});
            
        auto model_matrix = translate * rotate * scale;
        auto normal_matrix = glm::transpose(glm::inverse(model_matrix));

        instance_data[i].model_matrix = model_matrix;
        instance_data[i].normal_matrix = normal_matrix;
    }

    per_instance_buffer.bind_base(0);

    for(const auto& mesh : meshes){
        auto cmd = mesh.draw_command();
        cmd.instance_count = batch->transforms.size();
        
        mesh.bind();
        gl::draw<uint32_t>(cmd);
    }
}

const benzene::Batch& Batch::api_handle() const {
    return *batch;
}        

#pragma endregion