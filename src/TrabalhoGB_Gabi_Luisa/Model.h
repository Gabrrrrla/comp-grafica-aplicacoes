#ifndef MODEL_H
#define MODEL_H

#include <glad/glad.h>

#include <glm/glm.hpp>

#include <string>
#include <vector>

struct Transform3D {
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f);
    glm::vec3 scale = glm::vec3(1.0f);
};

struct Vertex {
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 normal = glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec2 texCoords = glm::vec2(0.0f);
};

struct Material {
    std::string name = "default";
    glm::vec3 ka = glm::vec3(0.15f);
    glm::vec3 kd = glm::vec3(0.80f);
    glm::vec3 ks = glm::vec3(0.30f);
    float shininess = 32.0f;
    GLuint diffuseTexture = 0;
    bool hasDiffuseTexture = false;
};

class Mesh {
public:
    Mesh();
    Mesh(const std::string& name, const std::vector<Vertex>& vertices, int materialIndex);
    ~Mesh();

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    Mesh(Mesh&& other) noexcept;
    Mesh& operator=(Mesh&& other) noexcept;

    void setup();
    void draw(GLuint shaderID, const std::vector<Material>& materials) const;

    bool empty() const;
    int getMaterialIndex() const;

private:
    std::string name;
    std::vector<Vertex> vertices;
    int materialIndex = 0;
    GLuint VAO = 0;
    GLuint VBO = 0;
};

class Model {
public:
    Model() = default;
    ~Model();

    Model(const Model&) = delete;
    Model& operator=(const Model&) = delete;

    Model(Model&& other) noexcept;
    Model& operator=(Model&& other) noexcept;

    bool loadFromFile(const std::string& path);
    void draw(GLuint shaderID, const Transform3D& transform, bool selected = false) const;
    bool isLoaded() const;

private:
    std::vector<Mesh> meshes;
    std::vector<Material> materials;
    std::string directory;
    bool loaded = false;

    int findOrCreateMaterial(const std::string& name);
    bool loadMaterials(const std::string& mtlPath);
    GLuint loadTexture(const std::string& texturePath, bool& success) const;
    void clear();
};

#endif
