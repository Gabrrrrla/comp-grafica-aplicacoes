// Alunas: Gabriela Bley e Luisa Becker
// Processamento Grafico: Aplicacoes
// Trabalho GB - Visualizador 3D

#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Camera.h"
#include "Model.h"

const unsigned int WIDTH = 1024;
const unsigned int HEIGHT = 768;

void framebufferSizeCallback(GLFWwindow* window, int width, int height);
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void mouseCallback(GLFWwindow* window, double xpos, double ypos);
void processInput(GLFWwindow* window);
GLuint setupShaders();
GLuint compileShader(GLenum type, const char* source);

const char* vertexShaderSource = R"glsl(
#version 410 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat3 normalMatrix;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;

void main() {
    vec4 worldPosition = model * vec4(position, 1.0);
    FragPos = worldPosition.xyz;
    Normal = normalize(normalMatrix * normal);
    TexCoords = texCoords;
    gl_Position = projection * view * worldPosition;
}
)glsl";

const char* fragmentShaderSource = R"glsl(
#version 410 core
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

out vec4 color;

uniform vec3 materialKa;
uniform vec3 materialKd;
uniform vec3 materialKs;
uniform float materialShininess;
uniform sampler2D diffuseTexture;
uniform bool useDiffuseTexture;

uniform vec3 lightPos;
uniform vec3 lightColor;
uniform float lightIntensity;
uniform float ambientIntensity;
uniform vec3 viewPos;
uniform bool selected;

void main() {
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);

    vec3 textureColor = useDiffuseTexture ? texture(diffuseTexture, TexCoords).rgb : vec3(1.0);
    vec3 diffuseMaterial = materialKd * textureColor;

    vec3 ambient = ambientIntensity * materialKa * lightColor;

    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * diffuseMaterial * lightColor * lightIntensity;

    float spec = pow(max(dot(viewDir, reflectDir), 0.0), materialShininess);
    vec3 specular = spec * materialKs * lightColor * lightIntensity;

    vec3 result = ambient + diffuse + specular;
    if (selected) {
        result = mix(result, vec3(1.0, 0.78, 0.25), 0.25);
    }

    color = vec4(result, 1.0);
}
)glsl";

bool perspectiveProjection = true;
bool wireframe = false;
int selectedObject = 0;

Camera camera(glm::vec3(0.0f, 1.6f, 6.0f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, -10.0f);
float deltaTime = 0.0f;
float lastFrame = 0.0f;

float lastX = WIDTH * 0.5f;
float lastY = HEIGHT * 0.5f;
bool firstMouse = true;

int main() {
    if (!glfwInit()) {
        std::cerr << "Falha ao inicializar GLFW" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Trabalho GB - Visualizador", nullptr, nullptr);
    if (!window) {
        std::cerr << "Falha ao criar janela GLFW" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        std::cerr << "Falha ao inicializar GLAD" << std::endl;
        glfwTerminate();
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    GLuint shaderID = setupShaders();
    if (shaderID == 0) {
        glfwTerminate();
        return -1;
    }

    {
        Model suzanne;
        Model cube;

        suzanne.loadFromFile("../assets/Modelos3D/SuzanneSubdiv1.obj");
        cube.loadFromFile("../assets/Modelos3D/Cube.obj");

        Transform3D suzanneTransform;
        suzanneTransform.position = glm::vec3(-1.35f, 0.0f, 0.0f);
        suzanneTransform.rotation = glm::vec3(0.0f, 25.0f, 0.0f);
        suzanneTransform.scale = glm::vec3(1.0f);

        Transform3D cubeTransform;
        cubeTransform.position = glm::vec3(1.55f, 0.0f, 0.0f);
        cubeTransform.rotation = glm::vec3(0.0f, -20.0f, 0.0f);
        cubeTransform.scale = glm::vec3(1.0f);

        glm::vec3 lightPos(2.5f, 4.0f, 3.0f);
        glm::vec3 lightColor(1.0f, 0.96f, 0.88f);

        while (!glfwWindowShouldClose(window)) {
            const float currentFrame = static_cast<float>(glfwGetTime());
            deltaTime = currentFrame - lastFrame;
            lastFrame = currentFrame;

            processInput(window);

            glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
            glClearColor(0.08f, 0.09f, 0.11f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            int framebufferWidth = WIDTH;
            int framebufferHeight = HEIGHT;
            glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);
            const float aspect = static_cast<float>(framebufferWidth) / static_cast<float>(framebufferHeight);

            glm::mat4 view = camera.getViewMatrix();
            glm::mat4 projection;
            if (perspectiveProjection) {
                projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
            } else {
                projection = glm::ortho(-4.0f * aspect, 4.0f * aspect, -4.0f, 4.0f, 0.1f, 100.0f);
            }

            glUseProgram(shaderID);
            glUniformMatrix4fv(glGetUniformLocation(shaderID, "view"), 1, GL_FALSE, glm::value_ptr(view));
            glUniformMatrix4fv(glGetUniformLocation(shaderID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
            glUniform3fv(glGetUniformLocation(shaderID, "lightPos"), 1, glm::value_ptr(lightPos));
            glUniform3fv(glGetUniformLocation(shaderID, "lightColor"), 1, glm::value_ptr(lightColor));
            glUniform1f(glGetUniformLocation(shaderID, "lightIntensity"), 1.0f);
            glUniform1f(glGetUniformLocation(shaderID, "ambientIntensity"), 0.25f);
            glUniform3fv(glGetUniformLocation(shaderID, "viewPos"), 1, glm::value_ptr(camera.position));

            suzanne.draw(shaderID, suzanneTransform, selectedObject == 0);
            cube.draw(shaderID, cubeTransform, selectedObject == 1);

            glfwSwapBuffers(window);
            glfwPollEvents();
        }
    }

    glDeleteProgram(shaderID);
    glfwTerminate();
    return 0;
}

void framebufferSizeCallback(GLFWwindow*, int width, int height) {
    glViewport(0, 0, width, height);
}

void keyCallback(GLFWwindow* window, int key, int, int action, int) {
    if (action != GLFW_PRESS) {
        return;
    }

    if (key == GLFW_KEY_ESCAPE) {
        glfwSetWindowShouldClose(window, true);
    } else if (key == GLFW_KEY_P) {
        perspectiveProjection = !perspectiveProjection;
    } else if (key == GLFW_KEY_M) {
        wireframe = !wireframe;
    } else if (key == GLFW_KEY_1) {
        selectedObject = 0;
    } else if (key == GLFW_KEY_2) {
        selectedObject = 1;
    }
}

void mouseCallback(GLFWwindow*, double xpos, double ypos) {
    if (firstMouse) {
        lastX = static_cast<float>(xpos);
        lastY = static_cast<float>(ypos);
        firstMouse = false;
    }

    const float xoffset = static_cast<float>(xpos) - lastX;
    const float yoffset = lastY - static_cast<float>(ypos);

    lastX = static_cast<float>(xpos);
    lastY = static_cast<float>(ypos);

    camera.processMouseMovement(xoffset, yoffset);
}

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        camera.processKeyboard("FORWARD", deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        camera.processKeyboard("BACKWARD", deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        camera.processKeyboard("LEFT", deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        camera.processKeyboard("RIGHT", deltaTime);
    }
}

GLuint setupShaders() {
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    if (vertexShader == 0 || fragmentShader == 0) {
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return 0;
    }

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    GLint success = 0;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[1024];
        glGetProgramInfoLog(shaderProgram, 1024, nullptr, infoLog);
        std::cerr << "Erro ao linkar shader program:\n" << infoLog << std::endl;
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        glDeleteProgram(shaderProgram);
        return 0;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

GLuint compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[1024];
        glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
        std::cerr << "Erro ao compilar shader:\n" << infoLog << std::endl;
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}
