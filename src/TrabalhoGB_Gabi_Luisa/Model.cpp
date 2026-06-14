#include "Model.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <algorithm>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {
std::string trim(const std::string& text) {
    const std::string whitespace = " \t\r\n";
    const size_t begin = text.find_first_not_of(whitespace);
    if (begin == std::string::npos) {
        return "";
    }
    const size_t end = text.find_last_not_of(whitespace);
    return text.substr(begin, end - begin + 1);
}

std::string getDirectory(const std::string& path) {
    const size_t slash = path.find_last_of("/\\");
    if (slash == std::string::npos) {
        return ".";
    }
    return path.substr(0, slash);
}

std::string getFileName(const std::string& path) {
    const size_t slash = path.find_last_of("/\\");
    if (slash == std::string::npos) {
        return path;
    }
    return path.substr(slash + 1);
}

std::string joinPath(const std::string& base, const std::string& path) {
    if (path.empty()) {
        return base;
    }
    if (path.front() == '/' || (path.size() > 1 && path[1] == ':')) {
        return path;
    }
    if (base.empty() || base == ".") {
        return path;
    }
    const char last = base.back();
    if (last == '/' || last == '\\') {
        return base + path;
    }
    return base + "/" + path;
}

bool fileCanOpen(const std::string& path) {
    std::ifstream file(path);
    return file.good();
}

std::string resolveAssetPath(const std::string& path) {
    if (fileCanOpen(path)) {
        return path;
    }

    const std::string fileName = getFileName(path);
    const std::vector<std::string> candidates = {
        "../assets/Modelos3D/" + fileName,
        "assets/Modelos3D/" + fileName,
        "../../assets/Modelos3D/" + fileName,
        "../../../assets/Modelos3D/" + fileName,
        "src/TrabalhoGB_Gabi_Luisa/" + fileName,
        "../src/TrabalhoGB_Gabi_Luisa/" + fileName,
    };

    for (const std::string& candidate : candidates) {
        if (fileCanOpen(candidate)) {
            return candidate;
        }
    }

    return path;
}

int parseObjIndex(const std::string& token, int count) {
    if (token.empty()) {
        return -1;
    }

    const int value = std::stoi(token);
    if (value > 0) {
        return value - 1;
    }
    if (value < 0) {
        return count + value;
    }
    return -1;
}

struct FaceIndex {
    int position = -1;
    int texCoord = -1;
    int normal = -1;
};

FaceIndex parseFaceIndex(const std::string& token, int positionCount, int texCoordCount, int normalCount) {
    FaceIndex index;
    std::stringstream stream(token);
    std::string part;
    std::vector<std::string> parts;

    while (std::getline(stream, part, '/')) {
        parts.push_back(part);
    }

    if (!parts.empty()) {
        index.position = parseObjIndex(parts[0], positionCount);
    }
    if (parts.size() > 1 && !parts[1].empty()) {
        index.texCoord = parseObjIndex(parts[1], texCoordCount);
    }
    if (parts.size() > 2 && !parts[2].empty()) {
        index.normal = parseObjIndex(parts[2], normalCount);
    }

    return index;
}

bool isValidIndex(int index, size_t size) {
    return index >= 0 && static_cast<size_t>(index) < size;
}

std::string restOfLineAfterKeyword(const std::string& line, const std::string& keyword) {
    if (line.size() <= keyword.size()) {
        return "";
    }
    return trim(line.substr(keyword.size()));
}

std::string parseTextureName(const std::string& value) {
    std::stringstream stream(value);
    std::string token;
    std::vector<std::string> tokens;

    while (stream >> token) {
        tokens.push_back(token);
    }

    if (tokens.empty()) {
        return "";
    }
    if (tokens.size() == 1) {
        return tokens[0];
    }

    return tokens.back();
}

GLuint getFallbackTexture() {
    static GLuint textureID = 0;
    if (textureID != 0) {
        return textureID;
    }

    const unsigned char whitePixel[] = {255, 255, 255, 255};
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, whitePixel);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    return textureID;
}
}

Mesh::Mesh() = default;

Mesh::Mesh(const std::string& meshName, const std::vector<Vertex>& meshVertices, int meshMaterialIndex)
    : name(meshName), vertices(meshVertices), materialIndex(meshMaterialIndex) {
}

Mesh::~Mesh() {
    if (VBO != 0) {
        glDeleteBuffers(1, &VBO);
    }
    if (VAO != 0) {
        glDeleteVertexArrays(1, &VAO);
    }
}

Mesh::Mesh(Mesh&& other) noexcept {
    *this = std::move(other);
}

Mesh& Mesh::operator=(Mesh&& other) noexcept {
    if (this != &other) {
        if (VBO != 0) {
            glDeleteBuffers(1, &VBO);
        }
        if (VAO != 0) {
            glDeleteVertexArrays(1, &VAO);
        }

        name = std::move(other.name);
        vertices = std::move(other.vertices);
        materialIndex = other.materialIndex;
        VAO = other.VAO;
        VBO = other.VBO;

        other.materialIndex = 0;
        other.VAO = 0;
        other.VBO = 0;
    }
    return *this;
}

void Mesh::setup() {
    if (vertices.empty()) {
        return;
    }

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, position)));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, normal)));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, texCoords)));

    glBindVertexArray(0);
}

void Mesh::draw(GLuint shaderID, const std::vector<Material>& materials) const {
    if (vertices.empty() || VAO == 0) {
        return;
    }

    const Material fallback;
    const Material& material = isValidIndex(materialIndex, materials.size()) ? materials[materialIndex] : fallback;

    glUniform3fv(glGetUniformLocation(shaderID, "materialKa"), 1, glm::value_ptr(material.ka));
    glUniform3fv(glGetUniformLocation(shaderID, "materialKd"), 1, glm::value_ptr(material.kd));
    glUniform3fv(glGetUniformLocation(shaderID, "materialKs"), 1, glm::value_ptr(material.ks));
    glUniform1f(glGetUniformLocation(shaderID, "materialShininess"), material.shininess);
    glUniform1i(glGetUniformLocation(shaderID, "useDiffuseTexture"), material.hasDiffuseTexture ? 1 : 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, material.hasDiffuseTexture ? material.diffuseTexture : getFallbackTexture());
    glUniform1i(glGetUniformLocation(shaderID, "diffuseTexture"), 0);

    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size()));
    glBindVertexArray(0);

    glBindTexture(GL_TEXTURE_2D, 0);
}

bool Mesh::empty() const {
    return vertices.empty();
}

int Mesh::getMaterialIndex() const {
    return materialIndex;
}

Model::~Model() {
    clear();
}

Model::Model(Model&& other) noexcept
    : meshes(std::move(other.meshes)),
      materials(std::move(other.materials)),
      directory(std::move(other.directory)),
      loaded(other.loaded) {
    other.loaded = false;
}

Model& Model::operator=(Model&& other) noexcept {
    if (this != &other) {
        clear();
        meshes    = std::move(other.meshes);
        materials = std::move(other.materials);
        directory = std::move(other.directory);
        loaded    = other.loaded;
        other.loaded = false;
    }
    return *this;
}

bool Model::loadFromFile(const std::string& path) {
    clear();

    materials.push_back(Material());

    const std::string resolvedPath = resolveAssetPath(path);
    std::ifstream file(resolvedPath);
    if (!file.is_open()) {
        std::cerr << "Erro ao abrir OBJ: " << path << std::endl;
        return false;
    }

    directory = getDirectory(resolvedPath);

    std::vector<glm::vec3> positions;
    std::vector<glm::vec2> texCoords;
    std::vector<glm::vec3> normals;
    std::vector<Vertex> currentVertices;
    std::string currentMeshName = getFileName(resolvedPath);
    int currentMaterialIndex = 0;

    auto flushMesh = [&]() {
        if (currentVertices.empty()) {
            return;
        }

        Mesh mesh(currentMeshName, currentVertices, currentMaterialIndex);
        mesh.setup();
        meshes.push_back(std::move(mesh));
        currentVertices.clear();
    };

    auto startMesh = [&](const std::string& meshName, int materialIndex) {
        flushMesh();
        if (!meshName.empty()) {
            currentMeshName = meshName;
        }
        currentMaterialIndex = materialIndex;
    };

    std::string line;
    while (std::getline(file, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') {
            continue;
        }

        std::stringstream stream(line);
        std::string keyword;
        stream >> keyword;

        if (keyword == "v") {
            glm::vec3 position;
            stream >> position.x >> position.y >> position.z;
            positions.push_back(position);
        } else if (keyword == "vt") {
            glm::vec2 texCoord;
            stream >> texCoord.x >> texCoord.y;
            texCoords.push_back(texCoord);
        } else if (keyword == "vn") {
            glm::vec3 normal;
            stream >> normal.x >> normal.y >> normal.z;
            normals.push_back(glm::normalize(normal));
        } else if (keyword == "f") {
            std::vector<FaceIndex> face;
            std::string token;

            while (stream >> token) {
                try {
                    face.push_back(parseFaceIndex(token, static_cast<int>(positions.size()),
                                                  static_cast<int>(texCoords.size()),
                                                  static_cast<int>(normals.size())));
                } catch (const std::exception&) {
                    std::cerr << "Aviso: face ignorada por indice invalido em " << resolvedPath << std::endl;
                    face.clear();
                    break;
                }
            }

            if (face.size() < 3) {
                continue;
            }

            for (size_t i = 1; i + 1 < face.size(); ++i) {
                FaceIndex triangle[3] = {face[0], face[i], face[i + 1]};
                Vertex vertices[3];
                bool needsNormal = false;
                bool validTriangle = true;

                for (int j = 0; j < 3; ++j) {
                    const FaceIndex& index = triangle[j];
                    if (!isValidIndex(index.position, positions.size())) {
                        validTriangle = false;
                        break;
                    }

                    vertices[j].position = positions[index.position];
                    if (isValidIndex(index.texCoord, texCoords.size())) {
                        vertices[j].texCoords = texCoords[index.texCoord];
                    }
                    if (isValidIndex(index.normal, normals.size())) {
                        vertices[j].normal = normals[index.normal];
                    } else {
                        needsNormal = true;
                    }
                }

                if (!validTriangle) {
                    std::cerr << "Aviso: triangulo ignorado por indice fora do intervalo em " << resolvedPath << std::endl;
                    continue;
                }

                if (needsNormal) {
                    const glm::vec3 edge1 = vertices[1].position - vertices[0].position;
                    const glm::vec3 edge2 = vertices[2].position - vertices[0].position;
                    const glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));
                    for (Vertex& vertex : vertices) {
                        vertex.normal = normal;
                    }
                }

                currentVertices.push_back(vertices[0]);
                currentVertices.push_back(vertices[1]);
                currentVertices.push_back(vertices[2]);
            }
        } else if (keyword == "o" || keyword == "g") {
            std::string meshName;
            stream >> meshName;
            if (meshName.empty()) {
                meshName = keyword == "o" ? "object" : "group";
            }
            startMesh(meshName, currentMaterialIndex);
        } else if (keyword == "mtllib") {
            const std::string mtlFileName = restOfLineAfterKeyword(line, "mtllib");
            const std::string mtlPath = joinPath(directory, mtlFileName);
            loadMaterials(mtlPath);
        } else if (keyword == "usemtl") {
            std::string materialName;
            stream >> materialName;
            currentMaterialIndex = findOrCreateMaterial(materialName);
            startMesh(currentMeshName, currentMaterialIndex);
        }
    }

    flushMesh();
    loaded = !meshes.empty();

    if (!loaded) {
        std::cerr << "Aviso: OBJ carregado sem meshes desenhaveis: " << resolvedPath << std::endl;
    }

    return loaded;
}

void Model::draw(GLuint shaderID, const Transform3D& transform, bool selected) const {
    if (!loaded) {
        return;
    }

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, transform.position);
    model = glm::rotate(model, glm::radians(transform.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, glm::radians(transform.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, glm::radians(transform.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::scale(model, transform.scale);

    const glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));

    glUniformMatrix4fv(glGetUniformLocation(shaderID, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix3fv(glGetUniformLocation(shaderID, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(normalMatrix));
    glUniform1i(glGetUniformLocation(shaderID, "selected"), selected ? 1 : 0);

    for (const Mesh& mesh : meshes) {
        mesh.draw(shaderID, materials);
    }
}

bool Model::isLoaded() const {
    return loaded;
}

int Model::findOrCreateMaterial(const std::string& name) {
    for (size_t i = 0; i < materials.size(); ++i) {
        if (materials[i].name == name) {
            return static_cast<int>(i);
        }
    }

    Material material;
    material.name = name.empty() ? "default" : name;
    materials.push_back(material);
    return static_cast<int>(materials.size() - 1);
}

bool Model::loadMaterials(const std::string& mtlPath) {
    const std::string resolvedPath = resolveAssetPath(mtlPath);
    std::ifstream file(resolvedPath);
    if (!file.is_open()) {
        std::cerr << "Aviso: MTL nao encontrado: " << mtlPath << std::endl;
        return false;
    }

    const std::string mtlDirectory = getDirectory(resolvedPath);
    int currentMaterialIndex = -1;
    std::string line;

    while (std::getline(file, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') {
            continue;
        }

        std::stringstream stream(line);
        std::string keyword;
        stream >> keyword;

        if (keyword == "newmtl") {
            std::string materialName;
            stream >> materialName;
            currentMaterialIndex = findOrCreateMaterial(materialName);
        } else if (currentMaterialIndex >= 0) {
            Material& material = materials[currentMaterialIndex];

            if (keyword == "Ka") {
                stream >> material.ka.r >> material.ka.g >> material.ka.b;
            } else if (keyword == "Kd") {
                stream >> material.kd.r >> material.kd.g >> material.kd.b;
            } else if (keyword == "Ks") {
                stream >> material.ks.r >> material.ks.g >> material.ks.b;
            } else if (keyword == "Ns") {
                stream >> material.shininess;
            } else if (keyword == "map_Kd") {
                const std::string textureName = parseTextureName(restOfLineAfterKeyword(line, "map_Kd"));
                if (!textureName.empty()) {
                    bool success = false;
                    material.diffuseTexture = loadTexture(joinPath(mtlDirectory, textureName), success);
                    material.hasDiffuseTexture = success;
                }
            }
        }
    }

    return true;
}

GLuint Model::loadTexture(const std::string& texturePath, bool& success) const {
    success = false;

    const std::string resolvedPath = resolveAssetPath(texturePath);
    int width = 0;
    int height = 0;
    int channels = 0;

    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(resolvedPath.c_str(), &width, &height, &channels, 0);
    if (!data) {
        std::cerr << "Aviso: textura nao carregada: " << texturePath << std::endl;
        return 0;
    }

    GLenum format = GL_RGB;
    if (channels == 1) {
        format = GL_RED;
    } else if (channels == 4) {
        format = GL_RGBA;
    }

    GLuint textureID = 0;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(data);

    success = true;
    return textureID;
}

void Model::clear() {
    for (Material& material : materials) {
        if (material.diffuseTexture != 0) {
            glDeleteTextures(1, &material.diffuseTexture);
            material.diffuseTexture = 0;
        }
        material.hasDiffuseTexture = false;
    }

    meshes.clear();
    materials.clear();
    directory.clear();
    loaded = false;
}
