# Exercício de Cubemap - Algumas instruções e snippets de cõdigo

## Código da geometria do Skybox

```cpp
int setupCubemap(float scaleFactor)
{
    float skyboxVertices[] = {
        // positions          
        scaleFactor * -0.5f,  scaleFactor *  0.5f,  scaleFactor * -0.5f,
        scaleFactor * -0.5f,  scaleFactor * -0.5f,  scaleFactor * -0.5f,
        scaleFactor *  0.5f,  scaleFactor * -0.5f,  scaleFactor * -0.5f,
        scaleFactor *  0.5f,  scaleFactor * -0.5f,  scaleFactor * -0.5f,
        scaleFactor *  0.5f,  scaleFactor *  0.5f,  scaleFactor * -0.5f,
        scaleFactor * -0.5f,  scaleFactor *  0.5f,  scaleFactor * -0.5f,

        scaleFactor * -0.5f,  scaleFactor * -0.5f,  scaleFactor *  0.5f,
        scaleFactor * -0.5f,  scaleFactor * -0.5f,  scaleFactor * -0.5f,
        scaleFactor * -0.5f,  scaleFactor *  0.5f,  scaleFactor * -0.5f,
        scaleFactor * -0.5f,  scaleFactor *  0.5f,  scaleFactor * -0.5f,
        scaleFactor * -0.5f,  scaleFactor *  0.5f,  scaleFactor *  0.5f,
        scaleFactor * -0.5f,  scaleFactor * -0.5f,  scaleFactor *  0.5f,

        scaleFactor *  0.5f,  scaleFactor * -0.5f,  scaleFactor * -0.5f,
        scaleFactor *  0.5f,  scaleFactor * -0.5f,  scaleFactor *  0.5f,
        scaleFactor *  0.5f,  scaleFactor *  0.5f,  scaleFactor *  0.5f,
        scaleFactor *  0.5f,  scaleFactor *  0.5f,  scaleFactor *  0.5f,
        scaleFactor *  0.5f,  scaleFactor *  0.5f,  scaleFactor * -0.5f,
        scaleFactor *  0.5f,  scaleFactor * -0.5f,  scaleFactor * -0.5f,

        scaleFactor * -0.5f,  scaleFactor * -0.5f,  scaleFactor *  0.5f,
        scaleFactor * -0.5f,  scaleFactor *  0.5f,  scaleFactor *  0.5f,
        scaleFactor *  0.5f,  scaleFactor *  0.5f,  scaleFactor *  0.5f,
        scaleFactor *  0.5f,  scaleFactor *  0.5f,  scaleFactor *  0.5f,
        scaleFactor *  0.5f,  scaleFactor * -0.5f,  scaleFactor *  0.5f,
        scaleFactor * -0.5f,  scaleFactor * -0.5f,  scaleFactor *  0.5f,

        scaleFactor * -0.5f,  scaleFactor *  0.5f,  scaleFactor * -0.5f,
        scaleFactor *  0.5f,  scaleFactor *  0.5f,  scaleFactor * -0.5f,
        scaleFactor *  0.5f,  scaleFactor *  0.5f,  scaleFactor *  0.5f,
        scaleFactor *  0.5f,  scaleFactor *  0.5f,  scaleFactor *  0.5f,
        scaleFactor * -0.5f,  scaleFactor *  0.5f,  scaleFactor *  0.5f,
        scaleFactor * -0.5f,  scaleFactor *  0.5f,  scaleFactor * -0.5f,

        scaleFactor * -0.5f,  scaleFactor * -0.5f,  scaleFactor * -0.5f,
        scaleFactor * -0.5f,  scaleFactor * -0.5f,  scaleFactor *  0.5f,
        scaleFactor *  0.5f,  scaleFactor * -0.5f,  scaleFactor * -0.5f,
        scaleFactor *  0.5f,  scaleFactor * -0.5f,  scaleFactor * -0.5f,
        scaleFactor * -0.5f,  scaleFactor * -0.5f,  scaleFactor *  0.5f,
        scaleFactor *  0.5f,  scaleFactor * -0.5f,  scaleFactor *  0.5f
    };   
    // skybox VAO
    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    return skyboxVAO;
}
```


## Referências
* **LearnOpenGL:** Advanced OpenGL - Cubemaps. Disponível em: https://learnopengl.com/Advanced-OpenGL/Cubemaps
* MÖLLER, Tomas et al. **Real-time rendering**. 4th ed. Boca Raton: CRC Press, 2018. Capítulo 10.

