// Alunas: Gabriela Bley e Luisa Becker
// Trabalho GB - Visualizador 3D com cena configurável

#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// JSON simples (header-only nlohmann/json nao disponivel — lemos manualmente)
// Usamos um parser minimo proprio para o scene.json

#include "Camera.h"
#include "Model.h"

// ---------- Dimensoes da janela ----------
const unsigned int WIDTH  = 1024;
const unsigned int HEIGHT = 768;

// ---------- Shaders ----------
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

    vec3 ambient  = ambientIntensity * materialKa * lightColor;
    float diff    = max(dot(norm, lightDir), 0.0);
    vec3 diffuse  = diff * diffuseMaterial * lightColor * lightIntensity;
    float spec    = pow(max(dot(viewDir, reflectDir), 0.0), materialShininess);
    vec3 specular = spec * materialKs * lightColor * lightIntensity;

    vec3 result = ambient + diffuse + specular;
    if (selected) {
        result = mix(result, vec3(1.0, 0.78, 0.25), 0.3);
    }

    color = vec4(result, 1.0);
}
)glsl";

// ---------- Estruturas da cena ----------

struct AnimationCurve {
    std::vector<glm::vec3> controlPoints;
    float speed = 1.0f;
    float t = 0.0f;          // parametro atual [0, N-1)
    bool active = false;

    // Catmull-Rom: interpola entre p1 e p2, usando p0 e p3 como tangentes
    glm::vec3 catmullRom(const glm::vec3& p0, const glm::vec3& p1,
                         const glm::vec3& p2, const glm::vec3& p3, float t) const {
        float t2 = t * t;
        float t3 = t2 * t;
        return 0.5f * (
            (2.0f * p1) +
            (-p0 + p2) * t +
            (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2 +
            (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3
        );
    }

    glm::vec3 evaluate() const {
        if (controlPoints.size() < 4) return controlPoints.empty() ? glm::vec3(0.0f) : controlPoints[0];

        int n = static_cast<int>(controlPoints.size());
        // segmento e parametro local
        float seg = std::fmod(t, static_cast<float>(n));
        int i = static_cast<int>(seg);
        float localT = seg - static_cast<float>(i);

        int i0 = (i - 1 + n) % n;
        int i1 = i % n;
        int i2 = (i + 1) % n;
        int i3 = (i + 2) % n;

        return catmullRom(controlPoints[i0], controlPoints[i1], controlPoints[i2], controlPoints[i3], localT);
    }

    void advance(float deltaTime) {
        if (!active || controlPoints.size() < 4) return;
        t += speed * deltaTime;
        if (t >= static_cast<float>(controlPoints.size()))
            t -= static_cast<float>(controlPoints.size());
    }
};

struct SceneObject {
    Model model;
    Transform3D transform;
    AnimationCurve curve;
    std::string name;
};

// ---------- Globals ----------
bool perspectiveProjection = true;
bool wireframe = false;
int selectedObject = 0;

Camera camera(glm::vec3(0.0f, 3.0f, 10.0f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, -10.0f);
float deltaTime = 0.0f;
float lastFrame = 0.0f;
float lastX = WIDTH * 0.5f;
float lastY = HEIGHT * 0.5f;
bool firstMouse = true;

std::vector<SceneObject> sceneObjects;
glm::vec3 lightPos(5.0f, 8.0f, 5.0f);
glm::vec3 lightColor(1.0f, 0.97f, 0.90f);
float lightIntensity  = 1.0f;
float ambientIntensity = 0.25f;

// ---------- Prototipos ----------
void framebufferSizeCallback(GLFWwindow* window, int w, int h);
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void mouseCallback(GLFWwindow* window, double xpos, double ypos);
void processInput(GLFWwindow* window);
GLuint setupShaders();
GLuint compileShader(GLenum type, const char* source);
bool loadScene(const std::string& path);

// ---------- Parser JSON minimo ----------
// Le valores simples de um JSON sem dependencias externas.
namespace json {

static std::string readFile(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) return "";
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

static std::string trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

// Retorna o valor de uma chave string simples: "key": "value"
static std::string getString(const std::string& json, const std::string& key) {
    std::string pattern = "\"" + key + "\"";
    size_t p = json.find(pattern);
    if (p == std::string::npos) return "";
    size_t colon = json.find(':', p + pattern.size());
    if (colon == std::string::npos) return "";
    size_t q1 = json.find('"', colon + 1);
    if (q1 == std::string::npos) return "";
    size_t q2 = json.find('"', q1 + 1);
    if (q2 == std::string::npos) return "";
    return json.substr(q1 + 1, q2 - q1 - 1);
}

// Retorna o valor numerico de uma chave: "key": 1.23
static float getFloat(const std::string& js, const std::string& key, float def = 0.0f) {
    std::string pattern = "\"" + key + "\"";
    size_t p = js.find(pattern);
    if (p == std::string::npos) return def;
    size_t colon = js.find(':', p + pattern.size());
    if (colon == std::string::npos) return def;
    size_t numStart = js.find_first_not_of(" \t\r\n", colon + 1);
    if (numStart == std::string::npos) return def;
    try { return std::stof(js.substr(numStart)); } catch (...) { return def; }
}

// Retorna o conteudo de um array de 3 floats: "key": [x, y, z]
static glm::vec3 getVec3(const std::string& js, const std::string& key, glm::vec3 def = glm::vec3(0.0f)) {
    std::string pattern = "\"" + key + "\"";
    size_t p = js.find(pattern);
    if (p == std::string::npos) return def;
    size_t bracket = js.find('[', p);
    if (bracket == std::string::npos) return def;
    size_t end = js.find(']', bracket);
    if (end == std::string::npos) return def;
    std::string inner = js.substr(bracket + 1, end - bracket - 1);
    std::replace(inner.begin(), inner.end(), ',', ' ');
    std::istringstream ss(inner);
    glm::vec3 v;
    ss >> v.x >> v.y >> v.z;
    return v;
}

// Retorna todos os blocos de objetos dentro de um array de nome dado
static std::vector<std::string> getArray(const std::string& js, const std::string& key) {
    std::vector<std::string> result;
    std::string pattern = "\"" + key + "\"";
    size_t p = js.find(pattern);
    if (p == std::string::npos) return result;
    size_t bracket = js.find('[', p);
    if (bracket == std::string::npos) return result;

    int depth = 0;
    size_t start = std::string::npos;
    for (size_t i = bracket; i < js.size(); ++i) {
        if (js[i] == '{') {
            if (depth == 0) start = i;
            ++depth;
        } else if (js[i] == '}') {
            --depth;
            if (depth == 0 && start != std::string::npos) {
                result.push_back(js.substr(start, i - start + 1));
                start = std::string::npos;
            }
        } else if (js[i] == ']' && depth == 0) {
            break;
        }
    }
    return result;
}

// Retorna o bloco de um objeto filho: "key": { ... }
static std::string getObject(const std::string& js, const std::string& key) {
    std::string pattern = "\"" + key + "\"";
    size_t p = js.find(pattern);
    if (p == std::string::npos) return "";
    size_t brace = js.find('{', p);
    if (brace == std::string::npos) return "";
    int depth = 0;
    for (size_t i = brace; i < js.size(); ++i) {
        if (js[i] == '{') ++depth;
        else if (js[i] == '}') {
            --depth;
            if (depth == 0) return js.substr(brace, i - brace + 1);
        }
    }
    return "";
}

// Retorna array de vec3 dentro de um array: [[x,y,z],[x,y,z],...]
static std::vector<glm::vec3> getVec3Array(const std::string& js, const std::string& key) {
    std::vector<glm::vec3> result;
    std::string pattern = "\"" + key + "\"";
    size_t p = js.find(pattern);
    if (p == std::string::npos) return result;
    size_t outerBracket = js.find('[', p);
    if (outerBracket == std::string::npos) return result;

    size_t i = outerBracket + 1;
    while (i < js.size()) {
        size_t inner = js.find('[', i);
        size_t outerClose = js.find(']', i);
        if (outerClose == std::string::npos) break;
        if (inner == std::string::npos || inner > outerClose) break;

        size_t innerClose = js.find(']', inner + 1);
        if (innerClose == std::string::npos) break;
        std::string innerStr = js.substr(inner + 1, innerClose - inner - 1);
        std::replace(innerStr.begin(), innerStr.end(), ',', ' ');
        std::istringstream ss(innerStr);
        glm::vec3 v(0.0f);
        ss >> v.x >> v.y >> v.z;
        result.push_back(v);
        i = innerClose + 1;
    }
    return result;
}

} // namespace json

// ---------- Carregamento de cena ----------
bool loadScene(const std::string& path) {
    std::string js = json::readFile(path);
    if (js.empty()) {
        std::cerr << "Erro: nao foi possivel ler " << path << std::endl;
        return false;
    }

    // Luz
    std::string lightObj = json::getObject(js, "light");
    if (!lightObj.empty()) {
        lightPos       = json::getVec3(lightObj, "position", lightPos);
        lightColor     = json::getVec3(lightObj, "color", lightColor);
        lightIntensity  = json::getFloat(lightObj, "intensity", lightIntensity);
        ambientIntensity = json::getFloat(lightObj, "ambient", ambientIntensity);
    }

    // Camera
    std::string camObj = json::getObject(js, "camera");
    if (!camObj.empty()) {
        glm::vec3 pos   = json::getVec3(camObj, "position", camera.position);
        float yaw       = json::getFloat(camObj, "yaw",   camera.yaw);
        float pitch     = json::getFloat(camObj, "pitch", camera.pitch);
        float speed     = json::getFloat(camObj, "speed", camera.movementSpeed);
        float fov       = json::getFloat(camObj, "fov",   camera.fov);
        float nearP     = json::getFloat(camObj, "near",  camera.nearPlane);
        float farP      = json::getFloat(camObj, "far",   camera.farPlane);
        camera = Camera(pos, glm::vec3(0.0f, 1.0f, 0.0f), yaw, pitch);
        camera.movementSpeed = speed;
        camera.fov       = fov;
        camera.nearPlane = nearP;
        camera.farPlane  = farP;
    }

    // Objetos
    auto objects = json::getArray(js, "objects");
    for (auto& objStr : objects) {
        SceneObject so;
        so.name = json::getString(objStr, "name");
        std::string file = json::getString(objStr, "file");

        so.transform.position = json::getVec3(objStr, "position");
        so.transform.rotation = json::getVec3(objStr, "rotation");
        so.transform.scale    = json::getVec3(objStr, "scale");
        if (so.transform.scale == glm::vec3(0.0f)) so.transform.scale = glm::vec3(1.0f);

        // Animacao
        std::string animObj = json::getObject(objStr, "animation");
        if (!animObj.empty()) {
            so.curve.controlPoints = json::getVec3Array(animObj, "controlPoints");
            so.curve.speed = json::getFloat(animObj, "speed", 1.0f);
            so.curve.active = (so.curve.controlPoints.size() >= 4);
        }

        if (!so.model.loadFromFile(file)) {
            std::cerr << "Aviso: modelo nao carregado: " << file << std::endl;
        }

        sceneObjects.push_back(std::move(so));
    }

    std::cout << "Cena carregada: " << sceneObjects.size() << " objetos." << std::endl;
    return !sceneObjects.empty();
}

// ---------- MAIN ----------
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

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Trabalho GB - Gabriela e Luisa", nullptr, nullptr);
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

    // Tenta carregar de varias localizacoes
    bool loaded = false;
    for (const std::string& p : {"scene.json", "../src/TrabalhoGB_Gabi_Luisa/scene.json", "src/TrabalhoGB_Gabi_Luisa/scene.json"}) {
        if (loadScene(p)) { loaded = true; break; }
    }
    if (!loaded) {
        std::cerr << "Nenhum scene.json encontrado. Coloque o arquivo no diretorio de trabalho." << std::endl;
        glfwTerminate();
        return -1;
    }

    std::cout << "\n=== Controles ===" << std::endl;
    std::cout << "WASD        : mover camera" << std::endl;
    std::cout << "Mouse       : girar camera" << std::endl;
    std::cout << "TAB         : selecionar proximo objeto" << std::endl;
    std::cout << "Setas + I/K : transladar objeto selecionado" << std::endl;
    std::cout << "R + X/Y/Z   : rotacionar objeto selecionado" << std::endl;
    std::cout << "+ / -       : escalar objeto selecionado" << std::endl;
    std::cout << "P           : alternar perspectiva/ortografica" << std::endl;
    std::cout << "M           : wireframe" << std::endl;
    std::cout << "ESC         : sair" << std::endl;
    std::cout << "================\n" << std::endl;

    while (!glfwWindowShouldClose(window)) {
        const float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        // Atualiza animacoes
        for (auto& so : sceneObjects) {
            if (so.curve.active) {
                so.curve.advance(deltaTime);
                so.transform.position = so.curve.evaluate();
            }
        }

        glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
        glClearColor(0.08f, 0.09f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        int fbWidth = WIDTH, fbHeight = HEIGHT;
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
        const float aspect = static_cast<float>(fbWidth) / static_cast<float>(fbHeight);

        glm::mat4 view = camera.getViewMatrix();
        glm::mat4 projection;
        if (perspectiveProjection) {
            projection = glm::perspective(glm::radians(camera.fov), aspect, camera.nearPlane, camera.farPlane);
        } else {
            projection = glm::ortho(-8.0f * aspect, 8.0f * aspect, -8.0f, 8.0f, camera.nearPlane, camera.farPlane);
        }

        glUseProgram(shaderID);
        glUniformMatrix4fv(glGetUniformLocation(shaderID, "view"),       1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shaderID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniform3fv(glGetUniformLocation(shaderID, "lightPos"),    1, glm::value_ptr(lightPos));
        glUniform3fv(glGetUniformLocation(shaderID, "lightColor"),  1, glm::value_ptr(lightColor));
        glUniform1f(glGetUniformLocation(shaderID, "lightIntensity"),  lightIntensity);
        glUniform1f(glGetUniformLocation(shaderID, "ambientIntensity"), ambientIntensity);
        glUniform3fv(glGetUniformLocation(shaderID, "viewPos"), 1, glm::value_ptr(camera.position));

        for (int i = 0; i < static_cast<int>(sceneObjects.size()); ++i) {
            sceneObjects[i].model.draw(shaderID, sceneObjects[i].transform, i == selectedObject);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteProgram(shaderID);
    glfwTerminate();
    return 0;
}

// ---------- Callbacks ----------

void framebufferSizeCallback(GLFWwindow*, int w, int h) {
    glViewport(0, 0, w, h);
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

void keyCallback(GLFWwindow* window, int key, int, int action, int) {
    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_ESCAPE) {
            glfwSetWindowShouldClose(window, true);
        } else if (key == GLFW_KEY_P) {
            perspectiveProjection = !perspectiveProjection;
        } else if (key == GLFW_KEY_M) {
            wireframe = !wireframe;
        } else if (key == GLFW_KEY_TAB) {
            if (!sceneObjects.empty()) {
                selectedObject = (selectedObject + 1) % static_cast<int>(sceneObjects.size());
                std::cout << "Objeto selecionado: [" << selectedObject << "] " << sceneObjects[selectedObject].name << std::endl;
            }
        }
    }
}

void processInput(GLFWwindow* window) {
    // Camera
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camera.processKeyboard("FORWARD",  deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camera.processKeyboard("BACKWARD", deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camera.processKeyboard("LEFT",     deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camera.processKeyboard("RIGHT",    deltaTime);

    if (sceneObjects.empty()) return;
    SceneObject& sel = sceneObjects[selectedObject];

    // Nao move objeto animado
    if (sel.curve.active) return;

    const float moveSpeed  = 3.0f * deltaTime;
    const float rotSpeed   = 90.0f * deltaTime;
    const float scaleSpeed = 1.0f * deltaTime;

    // Translacao
    if (glfwGetKey(window, GLFW_KEY_UP)    == GLFW_PRESS) sel.transform.position.y += moveSpeed;
    if (glfwGetKey(window, GLFW_KEY_DOWN)  == GLFW_PRESS) sel.transform.position.y -= moveSpeed;
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) sel.transform.position.x += moveSpeed;
    if (glfwGetKey(window, GLFW_KEY_LEFT)  == GLFW_PRESS) sel.transform.position.x -= moveSpeed;
    if (glfwGetKey(window, GLFW_KEY_I)     == GLFW_PRESS) sel.transform.position.z -= moveSpeed;
    if (glfwGetKey(window, GLFW_KEY_K)     == GLFW_PRESS) sel.transform.position.z += moveSpeed;

    // Rotacao
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
        if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) sel.transform.rotation.x += rotSpeed;
        if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS) sel.transform.rotation.y += rotSpeed;
        if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) sel.transform.rotation.z += rotSpeed;
    }

    // Escala uniforme
    if (glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_PRESS) {
        sel.transform.scale += glm::vec3(scaleSpeed);
    }
    if (glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_PRESS) {
        sel.transform.scale -= glm::vec3(scaleSpeed);
        if (sel.transform.scale.x < 0.05f) sel.transform.scale = glm::vec3(0.05f);
    }
}

// ---------- Shaders ----------

GLuint setupShaders() {
    GLuint vs = compileShader(GL_VERTEX_SHADER,   vertexShaderSource);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
    if (vs == 0 || fs == 0) { glDeleteShader(vs); glDeleteShader(fs); return 0; }

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);

    GLint ok = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetProgramInfoLog(prog, 1024, nullptr, log);
        std::cerr << "Erro ao linkar shaders:\n" << log << std::endl;
        glDeleteShader(vs); glDeleteShader(fs); glDeleteProgram(prog);
        return 0;
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

GLuint compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetShaderInfoLog(shader, 1024, nullptr, log);
        std::cerr << "Erro ao compilar shader:\n" << log << std::endl;
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}
