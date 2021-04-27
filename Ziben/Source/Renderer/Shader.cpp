#include "Shader.hpp"

#include <glm/gtc/type_ptr.hpp>

#include "Ziben/System/Log.hpp"

namespace Ziben {

    namespace Internal {

        static std::string ReadFile(const std::string& filepath) {
            namespace fs = std::filesystem;

            if (!fs::exists(filepath)) {
                ZIBEN_ERROR("Can't read the file by provided path: {0}", filepath);
                return {};
            }

            std::string result;
            std::ifstream infile(filepath, std::ios_base::in | std::ios_base::binary);

            if (!infile)
                ZIBEN_ERROR("Can't open the file by provided path: {0}", filepath);

            result.resize(fs::file_size(filepath));
            infile.read(result.data(), static_cast<std::streamsize>(result.size()));

            infile.close();

            return result;
        }

        ShaderType GetShaderTypeFromString(const std::string& type) {
            static const std::map<std::string, ShaderType> types = {
                { "vertex",         ShaderType::Vertex         },
                { "fragment",       ShaderType::Fragment       },
                { "pixel",          ShaderType::Fragment       },
                { "geometry",       ShaderType::Geometry       },
                { "tessControl",    ShaderType::TessControl    },
                { "tessEvaluation", ShaderType::TessEvaluation },
                { "compute",        ShaderType::Compute        }
            };

            return types.contains(type) ? types.at(type) : ShaderType::None;
        }

        static std::map<ShaderType, std::string> ParseShader(const std::string& shaderSource) {
            using namespace std::string_literals;

            std::map<ShaderType, std::string> result;

            auto token    = "#type"s;
            auto position = shaderSource.find(token, 0);

            while (position != std::string::npos) {
                auto eol   = shaderSource.find_first_of("\r\n", position);
                auto begin = position + token.size() + 1;

                assert(eol != std::string::npos);

                auto type       = shaderSource.substr(begin, eol - begin);
                auto shaderType = GetShaderTypeFromString(type);

                assert(shaderType != ShaderType::None);

                auto nextLinePosition = shaderSource.find_first_not_of("\r\n", eol);

                assert(nextLinePosition != std::string::npos);

                position = shaderSource.find(token, nextLinePosition);

                result[shaderType] = position == std::string::npos                                     ?
                                     shaderSource.substr(nextLinePosition)                             :
                                     shaderSource.substr(nextLinePosition, position - nextLinePosition);
            }

            return result;
        }

    } // namespace Internal

    Ref<Shader> Shader::Create(const std::string& filepath) {
        return CreateRef<Shader>(filepath);
    }

    void Shader::Bind(const Ref<Shader>& shader) {
        if (!shader->m_IsLinked)
            shader->Link();

        glUseProgram(shader->m_Handle);
    }

    void Shader::Unbind() {
        glUseProgram(0);
    }

    Shader::Shader(const std::string& filepath)
        : m_Handle(0)
        , m_IsLinked(false) {

        auto shaderSource  = Internal::ReadFile(filepath);
        auto shaderSources = Internal::ParseShader(shaderSource);

        Compile(shaderSources);
    }

    Shader::~Shader() {
        if (m_Handle == 0)
            return;

        glDeleteProgram(m_Handle);
    }

    void Shader::Compile(const std::map<ShaderType, std::string>& sources) {
        for (const auto& [type, source] : sources)
            Compile(type, source);
    }

    void Shader::Compile(ShaderType type, const std::string& source) {
        if (m_Handle == 0) {
            m_Handle = glCreateProgram();

            if (m_Handle == 0)
                throw std::runtime_error("Unable to create shader program!");
        }

        HandleType shaderHandle = glCreateShader(static_cast<GLenum>(type));

        if (shaderHandle == 0)
            throw std::runtime_error("Error creating shader!");

        const char* cSource = source.c_str();

        glShaderSource(shaderHandle, 1, &cSource, nullptr);
        glCompileShader(shaderHandle);

        int status;
        glGetShaderiv(shaderHandle, GL_COMPILE_STATUS, &status);

        if (status == GL_FALSE) {
            int logLength;
            glGetShaderiv(shaderHandle, GL_INFO_LOG_LENGTH, &logLength);

            std::string log(logLength, '\0');

            if (logLength > 0) {
                int written;
                glGetShaderInfoLog(shaderHandle, logLength, &written, log.data());
            }

            if (!log.empty())
                log += ": shader compilation failed!";
            else
                log = "Shader compilation failed!";

            glDeleteShader(shaderHandle);

            throw std::runtime_error(log);
        }

        glAttachShader(m_Handle, shaderHandle);
    }

    void Shader::Link() const {
        glLinkProgram(m_Handle);

        int shaderCount = 0;
        glGetProgramiv(m_Handle, GL_ATTACHED_SHADERS, &shaderCount);

        std::vector<HandleType> shaderHandles(shaderCount);
        glGetAttachedShaders(m_Handle, shaderCount, nullptr, shaderHandles.data());

        int status;
        glGetProgramiv(m_Handle, GL_LINK_STATUS, &status);

        if (status == GL_TRUE) {
            glValidateProgram(m_Handle);
            m_IsLinked = true;

            for (HandleType shaderHandle : shaderHandles) {
                glDetachShader(m_Handle, shaderHandle);
                glDeleteShader(shaderHandle);
            }

            return;
        }

        int logLength;
        glGetProgramiv(m_Handle, GL_INFO_LOG_LENGTH, &logLength);

        if (logLength > 0) {
            std::string log(logLength, '\0');

            int written;
            glGetProgramInfoLog(m_Handle, logLength, &written, log.data());

            glDeleteProgram(m_Handle);

            for (HandleType shaderHandle : shaderHandles)
                glDeleteShader(shaderHandle);

            throw std::runtime_error(log);
        }
    }

    void Shader::BindAttribLocation(uint32_t location, const std::string& name) const {
        if (!m_IsLinked)
            glBindAttribLocation(m_Handle, location, name.c_str());
    }

    void Shader::BindFragDataLocation(uint32_t location, const std::string& name) const {
        if (!m_IsLinked)
            glBindFragDataLocation(m_Handle, location, name.c_str());
    }

    void Shader::SetUniform(const std::string& name, bool value) {
        glUniform1i(GetUniformLocation(name), value);
    }

    void Shader::SetUniform(const std::string& name, int value) {
        glUniform1i(GetUniformLocation(name), value);
    }

    void Shader::SetUniform(const std::string& name, float value) {
        glUniform1f(GetUniformLocation(name), value);
    }

    void Shader::SetUniform(const std::string& name, float x, float y, float z) {
        glUniform3f(GetUniformLocation(name), x, y, z);
    }

    void Shader::SetUniform(const std::string& name, const glm::vec3& vec3) {
        glUniform3fv(GetUniformLocation(name), 1, glm::value_ptr(vec3));
    }

    void Shader::SetUniform(const std::string& name, const glm::vec4& vec4) {
        glUniform4fv(GetUniformLocation(name), 1, glm::value_ptr(vec4));
    }

    void Shader::SetUniform(const std::string& name, const glm::mat3& mat3) {
        glUniformMatrix3fv(GetUniformLocation(name), 1, GL_FALSE, glm::value_ptr(mat3));
    }

    void Shader::SetUniform(const std::string& name, const glm::mat4& mat4) {
        glUniformMatrix4fv(GetUniformLocation(name), 1, GL_FALSE, glm::value_ptr(mat4));
    }
    
    int Shader::GetUniformLocation(const std::string& name) {
        if (!m_UniformLocations.contains(name)) {
            int location = glGetUniformLocation(m_Handle, name.c_str());

            if (location >= 0)
                return m_UniformLocations[name] = location;
        }

        return m_UniformLocations[name];
    }

} // namespace Ziben