#include <iostream>
#include <string>
#include <assert.h>
#include <fstream>
#include <sstream>
#include <vector>

using namespace std;

// GLAD
#include <glad/glad.h>

// GLFW
#include <GLFW/glfw3.h>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Camera
#include "Camera.h"

// Protótipos das funções
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
int setupShader();
int loadSimpleOBJ(string filePATH, int &nVertices);

// Dimensões da janela
const GLuint WIDTH = 800, HEIGHT = 600;

// Shaders
const GLchar* vertexShaderSource = R"glsl(#version 450
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;
uniform mat4 model;
uniform mat4 projection;
uniform mat4 view;
out vec4 finalColor;
void main()
{
    gl_Position = projection * view * model * vec4(position, 1.0);
    finalColor = vec4(color, 1.0);
}
)glsl";

const GLchar* fragmentShaderSource = R"glsl(#version 450
in vec4 finalColor;
out vec4 color;
void main()
{
    color = finalColor;
}
)glsl";

// Variáveis Globais de Controle
bool perspective = true; 
Camera camera(glm::vec3(0.0, 0.0, -5.0), glm::vec3(0.0,1.0,0.0), 90.0, 0.0);
float deltaTime = 0.0;
float lastFrame = 0.0; 

// Estrutura do Objeto 3D (Substitui a antiga struct Mesh)
struct Object3D {
    GLuint VAO; 
    int nVertices;
    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 scale;

    Object3D() : position(0.0f), rotation(0.0f), scale(1.0f) {}
};

// Gerenciamento da Cena
std::vector<Object3D> sceneObjects;
int selectedObjectIndex = 0; // Índice do objeto atualmente selecionado

int main()
{
    glfwInit();
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Transformacoes 3D - GA", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    GLuint shaderID = setupShader();
    glUseProgram(shaderID);

    // Carregando os modelos (Ajuste os caminhos conforme a sua pasta)
    Object3D suzanne;
    suzanne.VAO = loadSimpleOBJ("../assets/Modelos3D/SuzanneSubdiv1.obj", suzanne.nVertices);
    suzanne.position = glm::vec3(-1.5f, 0.0f, 0.0f); // Posiciona na esquerda
    
    Object3D cube;
    // Se o nome do arquivo do cubo for diferente, altere aqui
    cube.VAO = loadSimpleOBJ("../assets/Modelos3D/Cube.obj", cube.nVertices);
    cube.position = glm::vec3(1.5f, 0.0f, 0.0f); // Posiciona na direita

    // Adiciona na "tabela" da cena
    sceneObjects.push_back(suzanne);
    sceneObjects.push_back(cube);

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        glfwPollEvents();

        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Atualização da Projeção
        glm::mat4 projection;
        if (perspective) {
            projection = glm::perspective(glm::radians(45.0f), (float)WIDTH / (float)HEIGHT, 0.1f, 100.0f);
        } else {
            projection = glm::ortho(-3.0, 3.0, -3.0, 3.0, 0.1, 100.0);
        }
        glUniformMatrix4fv(glGetUniformLocation(shaderID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

        // Controles de Câmera (WASD)
        if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camera.processKeyboard("FORWARD", deltaTime);
        if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camera.processKeyboard("BACKWARD", deltaTime);
        if(glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camera.processKeyboard("LEFT", deltaTime);
        if(glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camera.processKeyboard("RIGHT", deltaTime);
        
        glm::mat4 view = camera.getViewMatrix();
        glUniformMatrix4fv(glGetUniformLocation(shaderID, "view"), 1, GL_FALSE, glm::value_ptr(view));

        // ---------------------------------------------------------
        // CONTROLES DO OBJETO SELECIONADO
        // ---------------------------------------------------------
        if (sceneObjects.size() > 0) {
            Object3D& selectedObj = sceneObjects[selectedObjectIndex];
            float moveSpeed = 2.5f * deltaTime;
            float rotSpeed = 90.0f * deltaTime; // Graus por segundo
            float scaleSpeed = 1.0f * deltaTime;

            // Translação (Setas + I/K)
            if(glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) selectedObj.position.y += moveSpeed;
            if(glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) selectedObj.position.y -= moveSpeed;
            if(glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) selectedObj.position.x += moveSpeed;
            if(glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) selectedObj.position.x -= moveSpeed;
            if(glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS) selectedObj.position.z -= moveSpeed;
            if(glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS) selectedObj.position.z += moveSpeed;

            // Rotação (Segura R + X, Y ou Z)
            if(glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
                if(glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) selectedObj.rotation.x += rotSpeed;
                if(glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS) selectedObj.rotation.y += rotSpeed;
                if(glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) selectedObj.rotation.z += rotSpeed;
            }

            // Escala (Teclas + e -)
            if(glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_PRESS) selectedObj.scale += glm::vec3(scaleSpeed);
            if(glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_PRESS) {
                selectedObj.scale -= glm::vec3(scaleSpeed);
                // Evita escala negativa ou zero que inverte/deleta o modelo
                if (selectedObj.scale.x < 0.1f) selectedObj.scale = glm::vec3(0.1f);
            }
        }

        // ---------------------------------------------------------
        // RENDERIZAÇÃO DA CENA
        // ---------------------------------------------------------
        for (int i = 0; i < sceneObjects.size(); i++) {
            glm::mat4 model = glm::mat4(1.0f);
            
            // Ordem das transformações: T * R * S
            model = glm::translate(model, sceneObjects[i].position);
            model = glm::rotate(model, glm::radians(sceneObjects[i].rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
            model = glm::rotate(model, glm::radians(sceneObjects[i].rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::rotate(model, glm::radians(sceneObjects[i].rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
            model = glm::scale(model, sceneObjects[i].scale);

            glUniformMatrix4fv(glGetUniformLocation(shaderID, "model"), 1, GL_FALSE, glm::value_ptr(model));
            
            glBindVertexArray(sceneObjects[i].VAO);
            glDrawArrays(GL_TRIANGLES, 0, sceneObjects[i].nVertices);
        }
        
        glBindVertexArray(0);
        glfwSwapBuffers(window);
    }

    glfwTerminate();
    return 0;
}

// Callback de Teclado
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    if (key == GLFW_KEY_P && action == GLFW_PRESS)
        perspective = !perspective;

    // Seleção Cíclica
    if (key == GLFW_KEY_TAB && action == GLFW_PRESS) {
        if (sceneObjects.size() > 0) {
            selectedObjectIndex = (selectedObjectIndex + 1) % sceneObjects.size();
            std::cout << "Objeto selecionado: " << selectedObjectIndex << std::endl;
        }
    }
}

int setupShader()
{
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

int loadSimpleOBJ(string filePATH, int &nVertices)
{
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec2> texCoords;
    std::vector<glm::vec3> normals;
    std::vector<GLfloat> vBuffer;

    std::ifstream arqEntrada(filePATH.c_str());
    if (!arqEntrada.is_open()) 
    {
        std::cerr << "Erro ao tentar ler o arquivo " << filePATH << std::endl;
        return -1;
    }

    std::string line;
    while (std::getline(arqEntrada, line)) 
    {
        std::istringstream ssline(line);
        std::string word;
        ssline >> word;

        if (word == "v") 
        {
            glm::vec3 vertice;
            ssline >> vertice.x >> vertice.y >> vertice.z;
            vertices.push_back(vertice);
        } 
        else if (word == "f")
         {
            while (ssline >> word) 
            {
                int vi = 0;
                std::istringstream ss(word);
                std::string index;

                if (std::getline(ss, index, '/')) vi = !index.empty() ? std::stoi(index) - 1 : 0;

                vBuffer.push_back(vertices[vi].x);
                vBuffer.push_back(vertices[vi].y);
                vBuffer.push_back(vertices[vi].z);
                // Cor aleatória por vértice
                vBuffer.push_back(rand() % 256/255.0);
                vBuffer.push_back(rand() % 256/255.0);
                vBuffer.push_back(rand() % 256/255.0);
            }
        }
    }

    arqEntrada.close();

    GLuint VBO, VAO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vBuffer.size() * sizeof(GLfloat), vBuffer.data(), GL_STATIC_DRAW);
    
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    nVertices = vBuffer.size() / 6; 

    return VAO;
}