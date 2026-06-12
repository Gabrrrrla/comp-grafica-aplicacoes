# Trabalho GB — Visualizador 3D com Cena Configurável

**Alunas:** Gabriela Bley e Luisa Becker  
**Disciplina:** Processamento Gráfico: Aplicações — Unisinos  
**Professor:** Rossana Queiroz

---

## Descrição

Visualizador de cena 3D navegável implementado em OpenGL. A cena é um **museu virtual** com 22 objetos distribuídos em um salão com paredes, colunas, pedestais e esculturas. Uma das esculturas (Suzanne) voa pelo salão seguindo uma curva paramétrica Catmull-Rom.

A cena é carregada a partir de um arquivo `scene.json`, que define os objetos, a câmera, a fonte de luz e as animações.

---

## Requisitos implementados

- **Leitura de múltiplos OBJs** com grupos de malha (mesh), materiais `.mtl` e texturas
- **Iluminação de Phong** com ka, kd, ks e shininess lidos do `.mtl`; intensidades configuráveis no `scene.json`
- **Controle de câmera** por teclado (WASD) e mouse (FPS)
- **Seleção de objetos** via TAB com destaque visual; translação, rotação e escala uniformes pelo teclado
- **Animação por curva Catmull-Rom** fechada — o objeto `Suzanne_Voadora` percorre pontos de controle em loop contínuo
- **Arquivo de configuração de cena** (`scene.json`) com objetos, transformações iniciais, animações, luz e câmera

---

## Controles

| Tecla | Ação |
|---|---|
| `W` `A` `S` `D` | Mover câmera |
| Mouse | Girar câmera (modo FPS) |
| `TAB` | Selecionar próximo objeto |
| `↑` `↓` `←` `→` | Transladar objeto (Y e X) |
| `I` / `K` | Transladar objeto (Z) |
| `R` + `X` / `Y` / `Z` | Rotacionar objeto no eixo escolhido |
| `+` / `-` | Escala uniforme do objeto |
| `P` | Alternar projeção perspectiva / ortográfica |
| `M` | Alternar wireframe |
| `ESC` | Sair |

> Objetos com animação ativa não respondem aos controles de translação.

---

## Estrutura do projeto

```
TrabalhoGB_Gabi_Luisa/
├── main.cpp        # Loop principal, shaders, parser de cena, curva Catmull-Rom
├── Model.h / .cpp  # Carregamento de OBJ/MTL/texturas, renderização por mesh
├── Camera.h / .cpp # Câmera FPS com WASD e mouse
└── scene.json      # Configuração da cena (objetos, luz, câmera, animações)
```

---

## Arquivo de cena (`scene.json`)

O arquivo define três seções principais:

### `camera`
```json
"camera": {
  "position": [0.0, 3.0, 12.0],
  "yaw": -90.0,
  "pitch": -10.0,
  "speed": 5.0
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
Cada objeto pode ter:
- `file` — caminho do `.obj`
- `position`, `rotation`, `scale` — transformação inicial
- `animation` *(opcional)* — curva Catmull-Rom com `controlPoints` e `speed`

```json
{
  "name": "Suzanne_Voadora",
  "file": "../assets/Modelos3D/SuzanneSubdiv1.obj",
  "position": [0.0, 4.0, 0.0],
  "rotation": [0.0, 0.0, 0.0],
  "scale": [1.0, 1.0, 1.0],
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

## Como compilar e executar

O projeto usa CMake e está integrado ao workspace da disciplina.

```bash
# Na raiz do repositório (comp-grafica-aplicacoes/)
mkdir build && cd build
cmake ..
cmake --build . --target TrabalhoGB_Gabi_Luisa

# Executar (de dentro de build/)
./TrabalhoGB_Gabi_Luisa
```

O `scene.json` é procurado automaticamente nos seguintes caminhos (relativo ao executável):
1. `scene.json` (diretório atual)
2. `../src/TrabalhoGB_Gabi_Luisa/scene.json`
3. `src/TrabalhoGB_Gabi_Luisa/scene.json`

---

## Dependências

Gerenciadas automaticamente pelo CMake via FetchContent:

- [GLFW](https://www.glfw.org/) 3.4
- [GLM](https://github.com/g-truc/glm) 1.0.1
- [stb_image](https://github.com/nothings/stb)
- GLAD (incluído no repositório em `include/glad/` e `common/`)
