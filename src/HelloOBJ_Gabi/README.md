# Transformações 3D em OpenGL (Exercício de GA)

Este projeto é uma aplicação gráfica interativa desenvolvida em **C++** utilizando **OpenGL moderno**. O programa permite carregar múltiplos modelos 3D a partir de arquivos `.obj`, navegar pelo cenário com uma câmera sintética e aplicar transformações geométricas (translação, rotação e escala) individualmente no objeto selecionado.


## 📁 Estrutura de pastas 

```plaintext
📂 CG-20261/
├── 📂 include/              
│   ├── 📂 glad/              
│   │   ├── glad.h
│   │   ├── 📂 KHR/           
│   │       ├── khrplatform.h
├── 📂 common/               
│   ├── glad.c                
├── 📂 src/                   
│   ├── 📂 HelloOBJ_Gabi/
│   │   ├── main.cpp    
│   └── ...                   
├── 📂 build/
│   │   ├── HelloOBJ_Gabi.exe                  
├── 📄 CMakeLists.txt         
├── 📄 README.md              
├── 📄 GettingStarted.md      
```

## Controles da aplicação
Todas as interações na cena são feitas via teclado.

- **Câmera e visualização:**
W A S D: Movimenta a câmera pelo cenário.

P: Alterna entre projeção Perspectiva e Ortográfica.

- **Seleção de objetos:**
TAB: Alterna a seleção entre os objetos da cena (seleção cíclica).

Dica: Fique de olho no terminal, ele imprimirá o índice do objeto atualmente selecionado.

- **Transformações (aplicadas apenas ao objeto selecionado) - Mover:**

Seta para Cima / Seta para Baixo: Move no eixo Y.

Seta para Esquerda / Seta para Direita: Move no eixo X.

I / K: Move no eixo Z (profundidade).

- **Rotacionar:**

Segure R + X: Rotaciona no eixo X.

Segure R + Y: Rotaciona no eixo Y.

Segure R + Z: Rotaciona no eixo Z.

- **Escala:**

+ (Sinal de igual/mais): Aumenta a escala uniforme do objeto.

- (Sinal de menos): Diminui a escala uniforme do objeto.