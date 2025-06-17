#ifndef SHADER_H
#define SHADER_H

#include <glad/glad.h>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>  // For transformations like translate, rotate, scale
#include <glm/gtc/type_ptr.hpp>

class shader
{
    public:
    unsigned int progID; //program ID of the shader program

    void loadShaders(const char* vertexPath, const char* fragmentPath)
    {
        // 1. Retrieve the vertex/fragment source code from filePath
        std::string vertexCode;
        std::string fragmentCode;
        std::ifstream vShaderFile;
        std::ifstream fShaderFile;
        //exception handling
        vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

        try
        {
            vShaderFile.open(vertexPath);
            fShaderFile.open(fragmentPath);
            std::stringstream vShaderStream, fShaderStream;

            vShaderStream << vShaderFile.rdbuf();
            fShaderStream << fShaderFile.rdbuf();
            
            vShaderFile.close();
            fShaderFile.close();

            vertexCode = vShaderStream.str();
            fragmentCode = fShaderStream.str();

        }
        catch(std::ifstream::failure e)
        {
            std::cout << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ\n";
        }
        
        const char* vShaderCode = vertexCode.c_str();
        const char* fShaderCode = fragmentCode.c_str();

        // 2. Compile shaders
        unsigned int vertex, fragment;
        int success;
        char infoLog[512];

        vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vShaderCode, NULL);
        glCompileShader(vertex);

        glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
        if(!success)
        {
            glGetShaderInfoLog(vertex, 512, NULL, infoLog);
            std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
        }

        fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fShaderCode, NULL);
        glCompileShader(fragment);

        glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
        if(!success)
        {
            glGetShaderInfoLog(fragment, 512, NULL, infoLog);
            std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
        }
    
        // shaderProgam
        progID = glCreateProgram();
        glAttachShader(progID, vertex);
        glAttachShader(progID, fragment);
        glLinkProgram(progID);

        glGetProgramiv(progID, GL_LINK_STATUS, &success);
        if(!success)
        {
            glGetProgramInfoLog(progID, 512, NULL, infoLog);
            std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
        }

        glDeleteShader(vertex);
        glDeleteShader(fragment);
    }

    void use()
    {
        glUseProgram(progID);
    }


    void setFloat(const std::string &name, float value) const
    {
        int uniformLoc;
        uniformLoc = glGetUniformLocation(progID, name.c_str()); 
        glUniform1f(uniformLoc, value); 
    }

    void setVec2(const std::string &name, float val1, float val2) const
    {
        int uniformLoc;
        uniformLoc = glGetUniformLocation(progID, name.c_str()); 
        glUniform2f(uniformLoc, val1, val2); 
    }

    void setMat4(const std::string &name, glm::mat4 matrix) const
    {
        int uniformLoc;
        uniformLoc = glGetUniformLocation(progID, name.c_str()); 
        glUniformMatrix4fv(uniformLoc, 1, GL_FALSE, glm::value_ptr(matrix));
    }

    void setVec4(const std::string &name, glm::vec4 vec4D) const
    {
        int uniformLoc;
        uniformLoc = glGetUniformLocation(progID, name.c_str()); 
        glUniform4f(uniformLoc, vec4D.x, vec4D.y, vec4D.z, vec4D.w); 
    }
};

#endif