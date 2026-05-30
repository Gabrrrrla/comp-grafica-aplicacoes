// Alunas: Gabriela Bley e Luisa Becker
// Processamento Gráfico: Aplicações
// Prof. Rossana Queiroz

#include <iostream>
#include <string>
#include <vector>

// GLAD (Sempre inclua antes do GLFW)
#include <glad/glad.h>
#include <GLFW/glfw3.h>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Assimp
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

// Câmera
#include "Camera.h"

using namespace std;

// Protótipos das funções
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
GLuint setupShaders();
void drawGrid(GLuint shaderProgram);

// Dimensões da janela
const GLuint WIDTH = 1024, HEIGHT = 768;

// Shaders com iluminação de Phong
const GLchar* vertexShaderSource = R"glsl(#version 450
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texCoords; // Recuperado via Assimp, pronto para o futuro

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 FragPos;
out vec3 Normal;

void main() {
    FragPos = vec3(model * vec4(position, 1.0));
    Normal = mat3(transpose(inverse(model))) * normal;  
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
)glsl";

const GLchar* fragmentShaderSource = R"glsl(#version 450
in vec3 FragPos;
in vec3 Normal;
out vec4 color;

uniform vec3 lightPos; 
uniform vec3 viewPos; 

uniform vec3 material_ka;
uniform vec3 material_kd;
uniform vec3 material_ks;
uniform float material_shininess;

void main() {
    vec3 lightColor = vec3(1.0, 1.0, 1.0);
    vec3 ambient = material_ka * lightColor;
    
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = material_kd * diff * lightColor;
    
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material_shininess);
    vec3 specular = material_ks * spec * lightColor;  
        
    color = vec4(ambient + diffuse + specular, 1.0);
}
)glsl";

// Shader simples só para desenhar as linhas do grid
const GLchar* gridVertexShader = R"glsl(#version 450
layout (location = 0) in vec3 position;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
void main() {
    gl_Position = projection * view * model * vec4(position, 1.0);
}
)glsl";

const GLchar* gridFragmentShader = R"glsl(#version 450
out vec4 color;
void main() {
    color = vec4(0.5, 0.5, 0.5, 1.0); // Cinza
}
)glsl";

bool perspective = true; 
bool wireframe = false;
Camera camera(glm::vec3(0.0f, 2.0f, 6.0f)); 
float deltaTime = 0.0f;
float lastFrame = 0.0f; 

// Mouse FPS
float lastX = WIDTH / 2.0f;
float lastY = HEIGHT / 2.0f;
bool firstMouse = true;

struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
};

class Mesh {
public:
    // Transformações
    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 scale;

    // Materiais
    glm::vec3 ka, kd, ks;
    float shininess;

    Mesh(std::string path) : position(0.0f), rotation(0.0f), scale(1.0f),
                             ka(0.1f), kd(0.8f), ks(0.5f), shininess(32.0f) {
        loadModel(path);
    }

    void Draw(GLuint shaderID) {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, position);
        model = glm::rotate(model, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, scale);

        glUniformMatrix4fv(glGetUniformLocation(shaderID, "model"), 1, GL_FALSE, glm::value_ptr(model));
        
        glUniform3fv(glGetUniformLocation(shaderID, "material_ka"), 1, glm::value_ptr(ka));
        glUniform3fv(glGetUniformLocation(shaderID, "material_kd"), 1, glm::value_ptr(kd));
        glUniform3fv(glGetUniformLocation(shaderID, "material_ks"), 1, glm::value_ptr(ks));
        glUniform1f(glGetUniformLocation(shaderID, "material_shininess"), shininess);

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

private:
    GLuint VAO, VBO, EBO;
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    void loadModel(std::string path) {
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs);

        if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            std::cout << "ERRO ASSIMP: " << importer.GetErrorString() << std::endl;
            return;
        }
        processNode(scene->mRootNode, scene);
        setupMesh();
    }

    void processNode(aiNode *node, const aiScene *scene) {
        for(unsigned int i = 0; i < node->mNumMeshes; i++) {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            processMesh(mesh, scene);
        }
        for(unsigned int i = 0; i < node->mNumChildren; i++) {
            processNode(node->mChildren[i], scene);
        }
    }

    void processMesh(aiMesh *mesh, const aiScene *scene) {
        for(unsigned int i = 0; i < mesh->mNumVertices; i++) {
            Vertex vertex;
            vertex.Position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
            if (mesh->HasNormals()) {
                vertex.Normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
            }
            if(mesh->mTextureCoords[0]) {
                vertex.TexCoords = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
            } else {
                vertex.TexCoords = glm::vec2(0.0f, 0.0f);
            }
            vertices.push_back(vertex);
        }
        for(unsigned int i = 0; i < mesh->mNumFaces; i++) {
            aiFace face = mesh->mFaces[i];
            for(unsigned int j = 0; j < face.mNumIndices; j++)
                indices.push_back(face.mIndices[j]);
        }
    }

    void setupMesh() {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));

        glBindVertexArray(0);
    }
};

std::vector<Mesh> sceneObjects;
int selectedObjectIndex = 0; 
glm::vec3 pointLightPos(0.0f, 5.0f, 2.0f); 

int main() {
    glfwInit();
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Trabalho GA - Gabriela e Luisa", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Falha ao inicializar GLAD" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    // Compila os shaders principais
    GLuint shaderID = setupShaders();
    
    // Compila o Shader do Grid (simples)
    GLuint gridVShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(gridVShader, 1, &gridVertexShader, NULL);
    glCompileShader(gridVShader);
    GLuint gridFShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(gridFShader, 1, &gridFragmentShader, NULL);
    glCompileShader(gridFShader);
    GLuint gridShaderID = glCreateProgram();
    glAttachShader(gridShaderID, gridVShader);
    glAttachShader(gridShaderID, gridFShader);
    glLinkProgram(gridShaderID);

    Mesh suzanne("../assets/Modelos3D/SuzanneSubdiv1.obj");
    suzanne.position = glm::vec3(-2.0f, 1.0f, 0.0f); 
    suzanne.kd = glm::vec3(1.0f, 0.5f, 0.31f); // Laranja

    Mesh cube("../assets/Modelos3D/Cube.obj");
    cube.position = glm::vec3(2.0f, 1.0f, 0.0f); 
    cube.kd = glm::vec3(0.2f, 0.6f, 0.8f); // Azul

    sceneObjects.push_back(suzanne);
    sceneObjects.push_back(cube);

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        glfwPollEvents();

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if(wireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        else glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        // Matrizes base
        glm::mat4 projection = perspective ? 
            glm::perspective(glm::radians(45.0f), (float)WIDTH / HEIGHT, 0.1f, 100.0f) :
            glm::ortho(-6.0f, 6.0f, -4.5f, 4.5f, 0.1f, 100.0f);
        
        // Câmera WASD
        if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camera.processKeyboard("FORWARD", deltaTime);
        if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camera.processKeyboard("BACKWARD", deltaTime);
        if(glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camera.processKeyboard("LEFT", deltaTime);
        if(glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camera.processKeyboard("RIGHT", deltaTime);
        glm::mat4 view = camera.getViewMatrix();

        if (!sceneObjects.empty()) {
            Mesh& selectedObj = sceneObjects[selectedObjectIndex];
            float moveSpeed = 3.0f * deltaTime;
            float rotSpeed = 90.0f * deltaTime; 
            float scaleSpeed = 1.0f * deltaTime;

            // Translação (Setas + I/K)
            if(glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) selectedObj.position.y += moveSpeed;
            if(glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) selectedObj.position.y -= moveSpeed;
            if(glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) selectedObj.position.x += moveSpeed;
            if(glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) selectedObj.position.x -= moveSpeed;
            if(glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS) selectedObj.position.z -= moveSpeed;
            if(glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS) selectedObj.position.z += moveSpeed;

            // Rotação (R + Eixo)
            if(glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
                if(glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) selectedObj.rotation.x += rotSpeed;
                if(glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS) selectedObj.rotation.y += rotSpeed;
                if(glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) selectedObj.rotation.z += rotSpeed;
            }

            // Escala (+ e -)
            if(glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_PRESS) selectedObj.scale += glm::vec3(scaleSpeed);
            if(glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_PRESS) {
                selectedObj.scale -= glm::vec3(scaleSpeed);
                if (selectedObj.scale.x < 0.1f) selectedObj.scale = glm::vec3(0.1f);
            }
        }

        // Renderiza o grid
        glUseProgram(gridShaderID);
        glUniformMatrix4fv(glGetUniformLocation(gridShaderID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(gridShaderID, "view"), 1, GL_FALSE, glm::value_ptr(view));
        drawGrid(gridShaderID);

        // Renderiza os objetos (phong)
        glUseProgram(shaderID);
        glUniformMatrix4fv(glGetUniformLocation(shaderID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(shaderID, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniform3fv(glGetUniformLocation(shaderID, "lightPos"), 1, glm::value_ptr(pointLightPos));
        glUniform3fv(glGetUniformLocation(shaderID, "viewPos"), 1, glm::value_ptr(camera.position));

        for (int i = 0; i < sceneObjects.size(); i++) {
            sceneObjects[i].Draw(shaderID);
        }
        
        glfwSwapBuffers(window);
    }

    glfwTerminate();
    return 0;
}

// Funções auxiliares

void drawGrid(GLuint shaderProgram) {
    std::vector<glm::vec3> gridVertices;
    int size = 15;
    for (int i = -size; i <= size; i++) {
        gridVertices.push_back(glm::vec3(i, 0, -size));
        gridVertices.push_back(glm::vec3(i, 0, size));
        gridVertices.push_back(glm::vec3(-size, 0, i));
        gridVertices.push_back(glm::vec3(size, 0, i));
    }

    GLuint gridVAO, gridVBO;
    glGenVertexArrays(1, &gridVAO);
    glGenBuffers(1, &gridVBO);
    glBindVertexArray(gridVAO);
    glBindBuffer(GL_ARRAY_BUFFER, gridVBO);
    glBufferData(GL_ARRAY_BUFFER, gridVertices.size() * sizeof(glm::vec3), &gridVertices[0], GL_STATIC_DRAW);
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    
    glm::mat4 model = glm::mat4(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    
    glDrawArrays(GL_LINES, 0, gridVertices.size());
    
    glDeleteBuffers(1, &gridVBO);
    glDeleteVertexArrays(1, &gridVAO);
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; 
    lastX = xpos;
    lastY = ypos;

    camera.processMouseMovement(xoffset, yoffset);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    if (key == GLFW_KEY_P && action == GLFW_PRESS)
        perspective = !perspective;
        
    if (key == GLFW_KEY_M && action == GLFW_PRESS)
        wireframe = !wireframe;

    if (key == GLFW_KEY_TAB && action == GLFW_PRESS) {
        if (!sceneObjects.empty()) {
            selectedObjectIndex = (selectedObjectIndex + 1) % sceneObjects.size();
            std::cout << "Objeto selecionado: " << selectedObjectIndex << std::endl;
        }
    }
}

GLuint setupShaders() {
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