import numpy as np
import matplotlib.pyplot as plt

# 1. Função base: Interpolação Linear (Lerp)
# Implementa a fórmula: Lerp(t) = (1 - t)*P0 + t*P1
def lerp(p0, p1, t):
    return (1 - t) * p0 + t * p1

# 2. Definindo os 4 Pontos de Controle (Curva Cúbica)
P = np.array([
    [0.0, 0.0],  # P0
    [1.5, 4.0],  # P1
    [3.5, 4.0],  # P2
    [5.0, 0.0]   # P3
])

# Valor de 't' (a "porcentagem" do caminho, de 0 a 1)
t = 0.4 

# 3. Executando o Algoritmo de De Casteljau (Nested Lerps)
L0 = P

# Nível 1: Interpola 3 pontos sobre os segmentos originais
# CORREÇÃO: Usando L0[0] no lugar de L0
L1 = np.array([lerp(L0[0], L0[1], t), 
               lerp(L0[1], L0[2], t), 
               lerp(L0[2], L0[3], t)])

# Nível 2: Interpola 2 pontos sobre os segmentos do Nível 1
# CORREÇÃO: Usando L1[0] no lugar de L1
L2 = np.array([lerp(L1[0], L1[1], t), 
               lerp(L1[1], L1[2], t)])

# Nível 3: Interpola 1 ponto final (que formará a curva)
# CORREÇÃO: Usando L2[0] no lugar de L2
L3 = np.array([lerp(L2[0], L2[1], t)])

# 4. Gerando a Curva de Bézier completa (para o desenho do fundo)
curva_x, curva_y = [], []
for step_t in np.linspace(0, 1, 100):
    step_L1 = [lerp(L0[i], L0[i+1], step_t) for i in range(3)]
    step_L2 = [lerp(step_L1[i], step_L1[i+1], step_t) for i in range(2)]
    
    # CORREÇÃO: Faltava indicar o índice 0 em step_L2
    step_L3 = lerp(step_L2[0], step_L2[1], step_t) 
    
    # CORREÇÃO: step_L3 retorna um array [x, y], então precisamos separar em [0] e [1]
    curva_x.append(step_L3[0])
    curva_y.append(step_L3[1])

# 5. Configuração Visual (Matplotlib) usando a sua paleta de cores
fig, ax = plt.subplots(figsize=(10, 6))
fig.patch.set_facecolor('#ffffff') # Fundo branco sólido
ax.set_facecolor('#ffffff')

# Plotando a Curva de Bézier completa
ax.plot(curva_x, curva_y, color='#072A50', linewidth=3, label='Curva de Bézier')

# Plotando as linhas de construção de De Casteljau
ax.plot(L0[:, 0], L0[:, 1], color='#ADB8C4', linestyle='--', linewidth=1.5, marker='o', markersize=8, label='Polígono de Controle')
ax.plot(L1[:, 0], L1[:, 1], color='#20409A', linestyle='-', linewidth=2, marker='o', markersize=7, label='Nível 1 (3 pontos)')
ax.plot(L2[:, 0], L2[:, 1], color='#BDDA45', linestyle='-', linewidth=2.5, marker='o', markersize=7, label='Nível 2 (2 pontos)')

# Plotando o ponto final (Nível 3) em t = 0.4
ax.plot(L3[:, 0], L3[:, 1], color='#FFAA49', marker='o', markersize=12, zorder=5, label=f'Ponto Final (t = {t})')

# Adicionando rótulos aos pontos de controle originais
# CORREÇÃO: Para acessar a coordenada X do ponto 0, usa-se L0[0, 0] e para a Y, L0[0, 1]
ax.text(L0[0, 0]-0.2, L0[0, 1]-0.2, '$P_0$', fontsize=14, color='#072A50')
ax.text(L0[1, 0]-0.1, L0[1, 1]+0.2, '$P_1$', fontsize=14, color='#072A50')
ax.text(L0[2, 0]-0.1, L0[2, 1]+0.2, '$P_2$', fontsize=14, color='#072A50')
ax.text(L0[3, 0]+0.1, L0[3, 1]-0.2, '$P_3$', fontsize=14, color='#072A50')

# Ajustes estéticos finais (Estilo Flat/Limpo)
ax.set_title("Construção de De Casteljau (Interpolações Aninhadas)", fontsize=16, color='#072A50', pad=20)
ax.spines['top'].set_visible(False)
ax.spines['right'].set_visible(False)
ax.spines['left'].set_color('#ADB8C4')
ax.spines['bottom'].set_color('#ADB8C4')
ax.legend(loc='upper right', frameon=False, labelcolor='#20409A')
plt.axis('equal') # Mantém a proporção geométrica 1:1

plt.show()