# Trabalho GB — Visualizador 3D com Cena Configurável

**Alunas:** Gabriela Bley e Luisa Becker  
**Disciplina:** Processamento Gráfico: Aplicações — Unisinos  
**Professora:** Rossana Queiroz

---

## Descrição

Visualizador de cena 3D navegável implementado em OpenGL. A cena é um **museu virtual** com 23 objetos distribuídos em um salão com paredes, colunas, pedestais e esculturas. Dois objetos se movem automaticamente: uma escultura (Suzanne) voa pelo salão em altura média, e um cubo orbita o centro do salão rente ao chão — ambos seguindo curvas paramétricas Catmull-Rom.

A cena é carregada inteiramente a partir do arquivo `scene.json`, que define os objetos, câmera, fonte de luz, frustum e animações.

---

## Como compilar e executar

### Pré-requisitos

- [CMake](https://cmake.org/download/) 3.10 ou superior
- Compilador C++17 (MSVC via Visual Studio, MinGW-w64 ou similar)
- Git (necessário para o CMake baixar as dependências automaticamente)

### Passo a passo (Windows)

Abra o terminal (PowerShell, Prompt de Comando ou terminal do VS Code) **na raiz do repositório** (`comp-grafica-aplicacoes/`):

```bat
mkdir build
cd build
cmake ..
cmake --build . --target TrabalhoGB_Gabi_Luisa
```

Na primeira vez o CMake baixa e compila GLFW, GLM, stb_image e Assimp — isso pode levar alguns minutos.

### Executar

Ainda dentro da pasta `build/`, rode:

```bat
# Debug (padrão do MSVC)
.\Debug\TrabalhoGB_Gabi_Luisa.exe

# Release
cmake --build . --target TrabalhoGB_Gabi_Luisa --config Release
.\Release\TrabalhoGB_Gabi_Luisa.exe
```

> O programa procura o `scene.json` automaticamente nos caminhos abaixo, nessa ordem:
> 1. `scene.json` (diretório atual)
> 2. `../src/TrabalhoGB_Gabi_Luisa/scene.json`
> 3. `src/TrabalhoGB_Gabi_Luisa/scene.json`

### Com Visual Studio (alternativa)

```bat
cmake .. -G "Visual Studio 17 2022"
```

Abra o `.sln` gerado em `build/`, defina `TrabalhoGB_Gabi_Luisa` como projeto de inicialização e pressione **F5**.

---

## Controles

| Tecla | Ação |
|---|---|
| `W` `A` `S` `D` | Mover câmera |
| Mouse | Girar câmera (modo FPS) |
| `TAB` | Selecionar próximo objeto (nome exibido no terminal) |
| `↑` `↓` `←` `→` | Transladar objeto selecionado (eixos Y e X) |
| `I` / `K` | Transladar objeto selecionado (eixo Z) |
| `R` + `X` / `Y` / `Z` | Rotacionar objeto selecionado no eixo escolhido |
| `+` / `-` | Escala uniforme do objeto selecionado |
| `P` | Alternar projeção perspectiva / ortográfica |
| `M` | Alternar modo wireframe |
| `ESC` | Sair |

> Objetos com animação ativa (`Suzanne_Voadora`, `Bloco_Orbital`) não respondem aos controles de translação.

---

## Requisitos implementados

- **Leitura de múltiplos OBJs** já triangularizados, com normais e UVs; suporte a grupos de malha (`g`) e materiais `.mtl` por grupo
- **Iluminação de Phong** com ka, kd, ks e shininess lidos do `.mtl`; intensidade da luz e ambient configuráveis no `scene.json`
- **Controle de câmera FPS** por teclado (WASD) e mouse
- **Seleção de objetos** via TAB com destaque visual dourado; translação (6 direções), rotação (3 eixos) e escala uniforme pelo teclado
- **Animação por curva Catmull-Rom** em dois objetos: `Suzanne_Voadora` (altura média) e `Bloco_Orbital` (rente ao chão)
- **Configuração completa por `scene.json`**: objetos, transformações iniciais, animações, fonte de luz, posição/orientação da câmera e definição do frustum (fov, near, far)

---

## Estrutura do projeto

```
TrabalhoGB_Gabi_Luisa/
├── main.cpp        # Loop principal, shaders embutidos, parser JSON, curva Catmull-Rom
├── Model.h / .cpp  # Carregamento de OBJ/MTL/texturas, renderização por mesh
├── Camera.h / .cpp # Câmera FPS com WASD, mouse, fov/near/far configuráveis
└── scene.json      # Configuração completa da cena
```

---

## Arquivo de cena (`scene.json`)

### `camera`
```json
"camera": {
  "position": [0.0, 3.0, 12.0],
  "yaw": -90.0,
  "pitch": -10.0,
  "speed": 5.0,
  "fov": 45.0,
  "near": 0.1,
  "far": 200.0
}
```

### `light`
```json
"light": {
  "position": [0.0, 10.0, 5.0],
  "color": [1.0, 0.97, 0.90],
  "intensity": 1.0,
  "ambient": 0.25
}
```

### `objects`
Cada objeto suporta:
- `name` — identificador exibido ao selecionar
- `file` — caminho do `.obj` (relativo à raiz do repositório)
- `position`, `rotation`, `scale` — transformação inicial
- `animation` *(opcional)* — curva Catmull-Rom com `controlPoints` e `speed`

```json
{
  "name": "Suzanne_Voadora",
  "file": "../assets/Modelos3D/SuzanneSubdiv1.obj",
  "position": [0.0, 4.0, 0.0],
  "rotation": [0.0, 0.0, 0.0],
  "scale":    [1.0, 1.0, 1.0],
  "animation": {
    "speed": 0.6,
    "controlPoints": [
      [ 4.0, 4.0,  4.0],
      [ 4.0, 4.5, -4.0],
      [-4.0, 3.5, -4.0],
      [-4.0, 4.0,  4.0]
    ]
  }
}
```

---

## Dependências

Baixadas e compiladas automaticamente pelo CMake via FetchContent — não é necessário instalar nada manualmente:

- [GLFW](https://www.glfw.org/) 3.4
- [GLM](https://github.com/g-truc/glm) 1.0.1
- [stb_image](https://github.com/nothings/stb)
- [Assimp](https://github.com/assimp/assimp) 5.3.1
- GLAD (incluído no repositório em `include/glad/` e `common/`)
