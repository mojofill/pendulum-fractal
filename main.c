#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

void flip_image_vertical(unsigned char* data, int width, int height) {
    int rowSize = width * 4;
    unsigned char* temp = malloc(rowSize);
    for (int y = 0; y < height / 2; y++) {
        unsigned char* rowTop = data + y * rowSize;
        unsigned char* rowBottom = data + (height - 1 - y) * rowSize;
        memcpy(temp, rowTop, rowSize);
        memcpy(rowTop, rowBottom, rowSize);
        memcpy(rowBottom, temp, rowSize);
    }
    free(temp);
}

void check_shader(GLuint shader) {
    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[512];
        glGetShaderInfoLog(shader, 512, NULL, log);
        fprintf(stderr, "Shader compilation failed:\n%s\n", log);
    }
}

void checkProgram(GLuint program) {
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char log[512];
        glGetProgramInfoLog(program, 512, NULL, log);
        fprintf(stderr, "Program linking failed:\n%s\n", log);
    }
}

void load_shader(GLuint shader, const char *path) {
    FILE *f;
    char *text;
    long len;

    f = fopen(path, "rb");
    if (f == NULL) {
        fprintf(stderr, "shader file %s not found\n", path);
    }

    fseek(f, 0, SEEK_END);
    len = ftell(f);
    assert(len > 0);
    fseek(f, 0, SEEK_SET);

    text = calloc(1, len);
    assert(text != NULL);
    fread(text, 1, len, f);

    assert(strlen(text) > 0);
    fclose(f);

    glShaderSource(shader, 1, (const GLchar *const *) &text, (const GLint *) &len);
    glCompileShader(shader);
    check_shader(shader);
}

int main() {
    if (!glfwInit()) {
        fprintf(stderr, "shit went wrong with glfw reboot the entire system and stuff\n");
        return -1;
    }

    // glfw 4.3 for compute shaders
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // set up glfw with opengl ig
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE); // no window rendering, hide window

    GLFWwindow *window = glfwCreateWindow(640, 480, "Compute Shader", NULL, NULL);
    if (!window) {
        fprintf(stderr, "window couldnt be created\n");
        return -1;
    }
    glfwMakeContextCurrent(window);

    // load opengl functions
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        fprintf(stderr, "couldn't initialize glad\n");
        return -1;
    }

    // create shader
    GLuint shader = glCreateShader(GL_COMPUTE_SHADER);
    load_shader(shader, "./compute.glsl");

    // create program, attach shader, and link program
    GLuint program = glCreateProgram();
    glAttachShader(program, shader);
    glLinkProgram(program);
    checkProgram(program);
    glDeleteShader(shader); // delete to free up space, dont need after linking

    // create RGBA8 texture (RGBA8 is what stb_image_write uses)
    const unsigned int WIDTH = 2048, HEIGHT = 2048;
    GLuint texture;

    // Create RGBA8 texture
    glGenTextures(1, &texture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, WIDTH, HEIGHT);
    
    // Bind to compute shader for writing
    glBindImageTexture(0, texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);

    // dispatch compute
    glUseProgram(program);

    // uniform data
    GLuint width_loc = glGetUniformLocation(program, "WIDTH");
    GLuint height_loc = glGetUniformLocation(program, "HEIGHT");

    glUniform1ui(width_loc, WIDTH);
    glUniform1ui(height_loc, HEIGHT);

    glDispatchCompute(WIDTH, HEIGHT, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT); // stop opengl from doing anything with images before its safe to do so

    // Allocate image buffer (unsigned char)
    unsigned char* imageData = malloc(WIDTH * HEIGHT * 4);
    // Read texture into buffer
    glBindTexture(GL_TEXTURE_2D, texture);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData);

    // Flip vertically (optional)
    flip_image_vertical(imageData, WIDTH, HEIGHT);
    // Write to PNG
    stbi_write_png("output.png", WIDTH, HEIGHT, 4, imageData, WIDTH * 4);
    free(imageData);

    printf("finished!\n");

    // Cleanup
    // glDeleteBuffers(1, &ssbo);
    glDeleteTextures(1, &texture);
    glDeleteProgram(program);
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}