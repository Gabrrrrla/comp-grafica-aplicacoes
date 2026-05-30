/* Hello Triangle - código adaptado de https://learnopengl.com/#!Getting-started/Hello-Triangle
 * e https://antongerdelan.net/opengl/
 * https://learnopengl.com/Advanced-OpenGL/Cubemaps
 * Adaptado por Rossana Baptista Queiroz
 * para as disciplinas de Processamento Gráfico/Computação Gráfica - Unisinos
 * Versão inicial: 7/4/2017
 * Última atualização em 12/05/2026
 */

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

// STB_IMAGE
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

// Camera
#include "Camera.h"

// Protótipo da função de callback de teclado
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode);

// Protótipos das funções
int setupShader(const GLchar *vshader, const GLchar *fshader);
int setupGeometry();
int loadSimpleOBJ(string filePATH, int &nVertices);
int loadTexture(string filePath, int &imgWidth, int &imgHeight);
unsigned int loadCubemap(vector<std::string> faces);
int setupCubemap(float scaleFactor);


// Dimensões da janela (pode ser alterado em tempo de execução)
const GLuint WIDTH = 600, HEIGHT = 600;

// Código fonte do Vertex Shader (em GLSL): ainda hardcoded
const GLchar *vertexShaderSource = R"glsl(#version 450
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;
layout (location = 2) in vec3 normal;
layout (location = 3) in vec2 texc;
uniform mat4 model;
uniform mat4 projection;
uniform mat4 view;
out vec4 finalColor;
out vec3 fragPos;
out vec3 scaledNormal;
out vec2 texcoord;
void main()
{
gl_Position = projection * view * model * vec4(position, 1.0);
finalColor = vec4(color, 1.0);
fragPos = vec3(model * vec4(position, 1.0)); 
scaledNormal = vec3(model * vec4(normal, 1.0));
texcoord = vec2(texc.s,1.0 - texc.t); //inicialmente assim -- propositalmente 
}
)glsl";

// Código fonte do Fragment Shader (em GLSL)
const GLchar *fragmentShaderSource = R"glsl(#version 450
in vec4 finalColor;
in vec3 fragPos;
in vec3 scaledNormal;
in vec2 texcoord;
uniform sampler2D texBuffer;
// Propriedades da superfície/material
uniform float ka;
uniform float kd;
uniform float ks, q;
// Propriedades da fonte de luz
uniform vec3 lightPos;
uniform vec3 lightColor;
// Posicao da camera
uniform vec3 cameraPos;
out vec4 color;
void main()
{
	// Parcela da luz ambiente
	vec3 ambient = ka * lightColor;
	// Parcela da reflexão difusa
	vec3 N = normalize(scaledNormal);
	vec3 L = normalize(lightPos - fragPos);
	float diff = max(dot(N,L),0.0);
	vec3 diffuse = kd * diff * lightColor;
	// Parcela da reflexão especular
	//vec3 specular = ....
	vec4 texColor = texture(texBuffer,texcoord);
    color = (vec4(ambient,1) + vec4(diffuse,1))*texColor; // + vec4(specular,1);
}
)glsl";

//--------------------
const GLchar *vShaderSkybox = R"glsl(
#version 450
layout (location = 0) in vec3 aPos;
out vec3 TexCoords;
uniform mat4 projection;
uniform mat4 view;
void main()
{
    TexCoords = aPos;
    gl_Position = projection * view * vec4(aPos, 1.0);
}  
)glsl";

const GLchar *fShaderSkybox = R"glsl(
#version 450
out vec4 FragColor;
in vec3 TexCoords;
uniform samplerCube skybox;
void main()
{    
    FragColor = texture(skybox, TexCoords);
}
)glsl";


bool rotateX = false, rotateY = false, rotateZ = false;
bool perspective = true; // começa com projeção perspectiva

// Instanciação da Camera
Camera camera(glm::vec3(0.0, 0.0, -3.0), glm::vec3(0.0, 1.0, 0.0), 90.0, 0.0);
float deltaTime = 0.0;
float lastFrame = 0.0;

struct Mesh
{
	GLuint VAO;
	int nVertices;
	GLuint texID;
};

// Função MAIN
int main()
{
	// Inicialização da GLFW
	glfwInit();

	// Muita atenção aqui: alguns ambientes não aceitam essas configurações
	// Você deve adaptar para a versão do OpenGL suportada por sua placa
	// Sugestão: comente essas linhas de código para desobrir a versão e
	// depois atualize (por exemplo: 4.5 com 4 e 5)
	// glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	// glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	// glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Essencial para computadores da Apple
	// #ifdef __APPLE__
	//	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	// #endif

	// Criação da janela GLFW
	GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "Olá Camera Sintética!", nullptr, nullptr);
	glfwMakeContextCurrent(window);

	// Fazendo o registro da função de callback para a janela GLFW
	glfwSetKeyCallback(window, key_callback);

	// GLAD: carrega todos os ponteiros d funções da OpenGL
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
	}

	// Obtendo as informações de versão
	const GLubyte *renderer = glGetString(GL_RENDERER); /* get renderer string */
	const GLubyte *version = glGetString(GL_VERSION);	/* version as a string */
	cout << "Renderer: " << renderer << endl;
	cout << "OpenGL version supported " << version << endl;

	// Definindo as dimensões da viewport com as mesmas dimensões da janela da aplicação
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	glViewport(0, 0, width, height);

	// Compilando e buildando o programa de shader
	GLuint shaderID = setupShader(vertexShaderSource,fragmentShaderSource);
	GLuint skyboxShaderID = setupShader(vShaderSkybox,fShaderSkybox);
	
	glUseProgram(shaderID);

	// Matriz de modelo - Transformações nos objetos
	glm::mat4 model = glm::mat4(1); // matriz identidade;
	model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	glUniformMatrix4fv(glGetUniformLocation(shaderID, "model"), 1, GL_FALSE, glm::value_ptr(model));
	//-----------------
	// Matriz de projeção
	// glm::mat4 projection = glm::ortho(-3.0, 3.0, -3.0, 3.0, 0.1, 100.0);
	// glUniformMatrix4fv(glGetUniformLocation(shaderID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

	glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)WIDTH / (float)HEIGHT, 0.1f, 100.0f);
	glUniformMatrix4fv(glGetUniformLocation(shaderID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

	// Matriz de view
	glm::mat4 view = glm::lookAt(glm::vec3(0, 0, -3), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
	glUniformMatrix4fv(glGetUniformLocation(shaderID, "view"), 1, GL_FALSE, glm::value_ptr(view));

	glEnable(GL_DEPTH_TEST);

	Mesh m;

	m.VAO = loadSimpleOBJ("../assets/Modelos3D/SuzanneSubdiv1.obj", m.nVertices);

	// Mandando as infos de iluminação para o shader
	float ka = 0.2, kd = 0.5, ks = 0.5, q = 10.0;
	glUniform1f(glGetUniformLocation(shaderID, "ka"), ka);
	glUniform1f(glGetUniformLocation(shaderID, "kd"), kd);
	glUniform1f(glGetUniformLocation(shaderID, "ks"), ks);
	glUniform1f(glGetUniformLocation(shaderID, "q"), q);

	glm::vec3 lightPos = glm::vec3(-0.5, 5.0, 0.0);
	glUniform3f(glGetUniformLocation(shaderID, "lightPos"), lightPos.x, lightPos.y, lightPos.z);
	glm::vec3 lightColor = glm::vec3(1.0, 1.0, 1.0);
	glUniform3f(glGetUniformLocation(shaderID, "lightColor"), lightColor.x, lightColor.y, lightColor.z);
	
	int imgWidth,imgHeight;
	m.texID = loadTexture("../src/HelloCubemap/Suzanne.png",imgWidth,imgHeight);
	
	glActiveTexture(GL_TEXTURE0);
	glUniform1i(glGetUniformLocation(shaderID, "texBuff"), 0);

	//---
	vector <string> faces = {
		"../src/HelloCubemap/skybox/right.jpg",
    	"../src/HelloCubemap/skybox/left.jpg",
    	"../src/HelloCubemap/skybox/top.jpg",
    	"../src/HelloCubemap/skybox/bottom.jpg",
    	"../src/HelloCubemap/skybox/front.jpg",
    	"../src/HelloCubemap/skybox/back.jpg"
	};

	GLfloat skyboxTexID = loadCubemap(faces);
	GLfloat skyboxVAO = setupCubemap(5.0);

	glUseProgram(skyboxShaderID);
	glUniformMatrix4fv(glGetUniformLocation(skyboxShaderID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
	glUniformMatrix4fv(glGetUniformLocation(skyboxShaderID, "view"), 1, GL_FALSE, glm::value_ptr(view));
	glUniform1i(glGetUniformLocation(skyboxShaderID, "skybox"), 0);

	// ---

	// Loop da aplicação - "game loop"
	while (!glfwWindowShouldClose(window))
	{
		// Controle do tempo entre frames
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		// Checa se houveram eventos de input (key pressed, mouse moved etc.) e chama as funções de callback correspondentes
		glfwPollEvents();

		// Limpa o buffer de cor
		glClearColor(0.2f, 0.2f, 0.2f, 1.0f); // cor de fundo
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Essas funções estão depreciadas, só vão funcionar se usarmos o
		// OpenGL em modo "Compability" -- eu as deixo pra facilitar a visualização
		glLineWidth(10);
		glPointSize(10);

		// ----------

		if (perspective)
		{
			projection = glm::perspective(glm::radians(45.0f), (float)WIDTH / (float)HEIGHT, 0.1f, 100.0f);
			glUniformMatrix4fv(glGetUniformLocation(shaderID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
		}
		else // Troca para projeção paralela ortográfica
		{
			projection = glm::ortho(-3.0, 3.0, -3.0, 3.0, 0.1, 100.0);
			glUniformMatrix4fv(glGetUniformLocation(shaderID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
		}

		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
			camera.processKeyboard("FORWARD", deltaTime);
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
			camera.processKeyboard("BACKWARD", deltaTime);
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
			camera.processKeyboard("LEFT", deltaTime);
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
			camera.processKeyboard("RIGHT", deltaTime);

		if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
		{
			// Visualização de frente
			view = glm::lookAt(glm::vec3(0, 0, -3), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
		}
		else if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
		{
			// Visualização de costas
			view = glm::lookAt(glm::vec3(0, 0, 3), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
		}
		else if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS)
		{
			// Visualização da esquerda
			view = glm::lookAt(glm::vec3(-3, 0, 0), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
		}
		else if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS)
		{
			// Visualização da direita
			view = glm::lookAt(glm::vec3(3, 0, 0), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
		}
		else if (glfwGetKey(window, GLFW_KEY_5) == GLFW_PRESS)
		{
			// Visualização de cima
			view = glm::lookAt(glm::vec3(0, 3, 0), glm::vec3(0, 0, 0), glm::vec3(0, 0, 1));
		}
		else if (glfwGetKey(window, GLFW_KEY_6) == GLFW_PRESS)
		{
			// Visualização debaixo
			view = glm::lookAt(glm::vec3(0, -3, 0), glm::vec3(0, 0, 0), glm::vec3(0, 0, 1));
		}

		glUniformMatrix4fv(glGetUniformLocation(shaderID, "view"), 1, GL_FALSE, glm::value_ptr(view));

		// ----------

		float angle = (GLfloat)glfwGetTime();
		model = glm::mat4(1);
		model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		if (rotateX)
		{
			model = glm::rotate(model, angle, glm::vec3(1.0f, 0.0f, 0.0f));
		}
		else if (rotateY)
		{
			model = glm::rotate(model, angle, glm::vec3(0.0f, 1.0f, 0.0f));
		}
		else if (rotateZ)
		{
			model = glm::rotate(model, angle, glm::vec3(0.0f, 0.0f, 1.0f));
		}

		glUniformMatrix4fv(glGetUniformLocation(shaderID, "model"), 1, GL_FALSE, glm::value_ptr(model));

		// Atualização da matriz de view de acordo com as mudanças que ela sofreu via input
		// de mouse e/ou teclado
		view = camera.getViewMatrix();
		
		glUniformMatrix4fv(glGetUniformLocation(skyboxShaderID, "view"), 1, GL_FALSE, glm::value_ptr(view));

		//-------------------------------------------- Chamada de desenho do Skybox
		glDepthMask(GL_FALSE);
		glUseProgram(skyboxShaderID);
		glBindVertexArray(skyboxVAO);
		glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexID);
		glDrawArrays(GL_TRIANGLES, 0, 36);
		glDepthMask(GL_TRUE);

		//-------------------------------------------- Chamada de desenho do objeto
		glUseProgram(shaderID);
		glUniformMatrix4fv(glGetUniformLocation(shaderID, "view"), 1, GL_FALSE, glm::value_ptr(view));
		// Passa a posicao da camera para o shader
		glUniform3f(glGetUniformLocation(shaderID, "cameraPos"), camera.position.x, camera.position.y, camera.position.z);
		// Chamada de Desenho - DRAWCALL
		// Poligono Preenchido - GL_TRIANGLES
		// glBindVertexArray(VAO);
		// glDrawArrays(GL_TRIANGLES, 0, 18);
		glBindVertexArray(m.VAO);
		glBindTexture(GL_TEXTURE_2D,m.texID);
		glDrawArrays(GL_TRIANGLES, 0, m.nVertices);
		glBindVertexArray(0);
		// Troca os buffers da tela
		glfwSwapBuffers(window);
	}
	// Pede pra OpenGL desalocar os buffers
	glDeleteVertexArrays(1, &m.VAO);
	// Finaliza a execução da GLFW, limpando os recursos alocados por ela
	glfwTerminate();
	return 0;
}

// Função de callback de teclado - só pode ter uma instância (deve ser estática se
// estiver dentro de uma classe) - É chamada sempre que uma tecla for pressionada
// ou solta via GLFW
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);

	if (key == GLFW_KEY_X && action == GLFW_PRESS)
	{
		rotateX = true;
		rotateY = false;
		rotateZ = false;
	}

	if (key == GLFW_KEY_Y && action == GLFW_PRESS)
	{
		rotateX = false;
		rotateY = true;
		rotateZ = false;
	}

	if (key == GLFW_KEY_Z && action == GLFW_PRESS)
	{
		rotateX = false;
		rotateY = false;
		rotateZ = true;
	}

	if (key == GLFW_KEY_P && action == GLFW_PRESS)
	{
		perspective = !perspective;
	}
}

// Esta função está basntante hardcoded - objetivo é compilar e "buildar" um programa de
//  shader simples e único neste exemplo de código
//  O código fonte do vertex e fragment shader está nos arrays vertexShaderSource e
//  fragmentShader source no iniçio deste arquivo
//  A função retorna o identificador do programa de shader
int setupShader(const GLchar *vshader, const GLchar *fshader)
{
	// Vertex shader
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vshader, NULL);
	glCompileShader(vertexShader);
	// Checando erros de compilação (exibição via log no terminal)
	GLint success;
	GLchar infoLog[512];
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n"
				  << infoLog << std::endl;
	}
	// Fragment shader
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fshader, NULL);
	glCompileShader(fragmentShader);
	// Checando erros de compilação (exibição via log no terminal)
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n"
				  << infoLog << std::endl;
	}
	// Linkando os shaders e criando o identificador do programa de shader
	GLuint shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);
	// Checando por erros de linkagem
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n"
				  << infoLog << std::endl;
	}
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
	glm::vec3 color = glm::vec3(1.0, 0.0, 0.0);

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
		else if (word == "vt")
		{
			glm::vec2 vt;
			ssline >> vt.s >> vt.t;
			texCoords.push_back(vt);
		}
		else if (word == "vn")
		{
			glm::vec3 normal;
			ssline >> normal.x >> normal.y >> normal.z;
			normals.push_back(normal);
		}
		else if (word == "f")
		{
			while (ssline >> word)
			{
				int vi = 0, ti = 0, ni = 0;
				std::istringstream ss(word);
				std::string index;

				if (std::getline(ss, index, '/'))
					vi = !index.empty() ? std::stoi(index) - 1 : 0;
				if (std::getline(ss, index, '/'))
					ti = !index.empty() ? std::stoi(index) - 1 : 0;
				if (std::getline(ss, index))
					ni = !index.empty() ? std::stoi(index) - 1 : 0;

				vBuffer.push_back(vertices[vi].x);
				vBuffer.push_back(vertices[vi].y);
				vBuffer.push_back(vertices[vi].z);
				vBuffer.push_back(color.r);
				vBuffer.push_back(color.g);
				vBuffer.push_back(color.b);
				vBuffer.push_back(normals[ni].x);
				vBuffer.push_back(normals[ni].y);
				vBuffer.push_back(normals[ni].z);
				vBuffer.push_back(texCoords[ti].s);
				vBuffer.push_back(texCoords[ti].t);
			}
		}
	}

	arqEntrada.close();

	std::cout << "Gerando o buffer de geometria..." << std::endl;
	GLuint VBO, VAO;
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, vBuffer.size() * sizeof(GLfloat), vBuffer.data(), GL_STATIC_DRAW);

	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	// Atributo: coordenada do vértice (x,y,z) - 3 valores
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid *)0);
	glEnableVertexAttribArray(0);

	// Atributo: cor do vértice (r, g, b) - 3 valores
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid *)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);

	// Atributo: vetor normal no vértice (x, y, z) - 3 valores
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid *)(6 * sizeof(GLfloat)));
	glEnableVertexAttribArray(2);

	// Atributo: coordenada de textura s, t - 2 valores
	glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid *)(9 * sizeof(GLfloat)));
	glEnableVertexAttribArray(3);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	nVertices = vBuffer.size() / 11; // x, y, z, r, g, b,nx,ny, nz (valores atualmente armazenados por vértice)

	return VAO;
}

int loadTexture(string filePath, int &imgWidth, int &imgHeight)
{
	GLuint texID;
	// Gera o identificador da textura na memória
	glGenTextures(1, &texID);
	glBindTexture(GL_TEXTURE_2D, texID);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	int width, height, nrChannels;

	unsigned char *data = stbi_load(filePath.c_str(), &width, &height, &nrChannels, 0);

	if (data)
	{
		if (nrChannels == 3) // jpg, bmp
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		}
		else // png
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		}
		glGenerateMipmap(GL_TEXTURE_2D);
		stbi_image_free(data);
		glBindTexture(GL_TEXTURE_2D, 0);
		return texID;
	}
	else
	{
		std::cout << "Failed to load texture" << std::endl;
		return -1;
	}

}


unsigned int loadCubemap(vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 
                         0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
            );
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap tex failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}  

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
