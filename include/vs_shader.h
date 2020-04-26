#pragma once

#include <glad/glad.h>

#include <glm/glm.hpp>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <filesystem>

#include "vs_log.h"

class VSShader
{
public:
    VSShader(const char* name)
    {
        const auto vertexDir = std::filesystem::path(shaderDirectory) / "vertex";
        const auto fragmentDir = std::filesystem::path(shaderDirectory) / "fragment";

        const auto vertexShaderPath = (vertexDir / name).replace_extension(".glsl");

        ID = glCreateProgram();

        GLuint vertexShaderID = compileShader(vertexShaderPath, GL_VERTEX_SHADER);
        glAttachShader(ID, vertexShaderID);

        const auto fragmentShaderPath = (fragmentDir / vertexShaderPath.filename());

        auto hasFragmentShader = false;
        GLuint fragmentShaderID = -1;
        if (!std::filesystem::exists(fragmentShaderPath))
        {
            VSLog::Log(
                VSLog::Category::Shader,
                VSLog::Level::warn,
                "Vertex shader: {} is present, but corresponding fragment shader: {} is missing",
                vertexShaderPath.c_str(),
                fragmentShaderPath.c_str());
        }
        else
        {
            fragmentShaderID = compileShader(fragmentShaderPath, GL_FRAGMENT_SHADER);
            glAttachShader(ID, fragmentShaderID);
        }
        glLinkProgram(ID);

        checkProgramLinkErrors(ID);

        glDetachShader(ID, vertexShaderID);
        glDetachShader(ID, fragmentShaderID);

        glDeleteShader(vertexShaderID);
        glDeleteShader(fragmentShaderID);
    }

    GLuint getID() const
    {
        return ID;
    }

    void use() const
    {
        glUseProgram(ID);
    }

    void setBool(const std::string& name, bool value) const
    {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
    }

    void setInt(const std::string& name, GLint value) const
    {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
    }

    void setFloat(const std::string& name, float value) const
    {
        glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
    }

    void setVec3(const std::string& name, glm::vec3 value) const
    {
        glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
    }

    void setMat4(const std::string& name, glm::mat4 value) const
    {
        glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &value[0][0]);
    }
    
private:
    GLuint ID;

    inline static const auto shaderDirectory = std::filesystem::path("shaders");
    inline static const auto vertexDir = shaderDirectory / "vertex";
    inline static const auto fragmentDir = shaderDirectory / "fragment";

    static bool checkShaderCompileErrors(unsigned int shaderID)
    {
        int success = 1;
        int infoLogLength = 0;

        glGetShaderiv(shaderID, GL_COMPILE_STATUS, &success);
        glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &infoLogLength);

        std::vector<char> infoLog(infoLogLength + 1);

        if (success == 0)
        {
            glGetShaderInfoLog(shaderID, 1024, nullptr, &infoLog[0]);
            VSLog::Log(
                VSLog::Category::Shader,
                VSLog::Level::critical,
                "Shader compilation failed:\n{}",
                &infoLog[0]);
        }

        return success == 0;
    }

    static bool checkProgramLinkErrors(unsigned int programID)
    {
        int success = 1;
        int infoLogLength = 0;

        glGetProgramiv(programID, GL_LINK_STATUS, &success);
        glGetProgramiv(programID, GL_INFO_LOG_LENGTH, &infoLogLength);

        std::vector<char> infoLog(infoLogLength + 1);

        if (success == 0)
        {
            glGetProgramInfoLog(programID, 1024, nullptr, &infoLog[0]);
            VSLog::Log(
                VSLog::Category::Shader,
                VSLog::Level::critical,
                "Progam linking failed:\n{}",
                &infoLog[0]);
        }

        return success == 0;
    }

    static GLuint compileShader(const std::filesystem::path& shaderPath, GLenum shaderType)
    {
        std::ifstream shaderStream(shaderPath);
        std::string shaderString(
            (std::istreambuf_iterator<char>(shaderStream)), std::istreambuf_iterator<char>());

        VSLog::Log(
            VSLog::Category::Shader,
            VSLog::Level::info,
            "Compiling shader: {}",
            shaderPath.c_str());

        GLuint shaderID = glCreateShader(shaderType);

        // Compile Vertex Shader
        char const* shaderSourcePointer = shaderString.c_str();
        glShaderSource(shaderID, 1, &shaderSourcePointer, nullptr);
        glCompileShader(shaderID);

        const auto hadCompileError = checkShaderCompileErrors(shaderID);
        if (!hadCompileError)
        {
            VSLog::Log(
                VSLog::Category::Shader,
                VSLog::Level::info,
                "Succesfully compiled shader: {}",
                shaderPath.c_str());
        }

        return shaderID;
    }
};
