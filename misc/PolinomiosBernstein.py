import numpy as np
import matplotlib.pyplot as plt

# 1. Definindo os mesmos Pontos de Controle da curva do De Casteljau
P0 = np.array([0.0, 0.0])
P1 = np.array([1.5, 4.0])
P2 = np.array([3.5, 4.0])
P3 = np.array([5.0, 0.0])

# 2. Definindo o intervalo de 't' (a "porcentagem" do caminho, de 0 a 1)
t = np.linspace(0, 1, 100)

# 3. Equações dos Polinômios de Bernstein (Pesos)
B0 = (1 - t)**3               # Peso do Ponto P0
B1 = 3 * t * (1 - t)**2       # Peso do Ponto P1
B2 = 3 * (t**2) * (1 - t)     # Peso do Ponto P2
B3 = t**3                     # Peso do Ponto P3

# 4. A LIGAÇÃO: Calculando a Curva usando a Forma de Bernstein
# Multiplicamos cada ponto de controle pelo seu respectivo peso (Bernstein) e somamos tudo!
curva_x = P0[0]*B0 + P1[0]*B1 + P2[0]*B2 + P3[0]*B3
curva_y = P0[1]*B0 + P1[1]*B1 + P2[1]*B2 + P3[1]*B3

# 5. Configuração Visual: Criando dois gráficos lado a lado (1 linha, 2 colunas)
fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(15, 6))
fig.patch.set_facecolor('#ffffff')

# ==========================================
# GRÁFICO 1: As Funções de Peso (Seu script original)
# ==========================================
ax1.set_facecolor('#ffffff')
ax1.plot(t, B0, color='#20409A', linewidth=3, label='$B_{3,0}(t)$ (Peso de $P_0$)')
ax1.plot(t, B1, color='#BDDA45', linewidth=3, label='$B_{3,1}(t)$ (Peso de $P_1$)')
ax1.plot(t, B2, color='#FFAA49', linewidth=3, label='$B_{3,2}(t)$ (Peso de $P_2$)')
ax1.plot(t, B3, color='#8363A7', linewidth=3, label='$B_{3,3}(t)$ (Peso de $P_3$)')
ax1.plot(t, B0 + B1 + B2 + B3, color='#ADB8C4', linestyle='--', linewidth=2, label='Soma = 1')

ax1.set_title("1. Influência (Pesos) de Bernstein", fontsize=14, color='#072A50', pad=15)
ax1.set_xlabel("Valor de t", fontsize=12, color='#072A50')
ax1.set_ylabel("Peso", fontsize=12, color='#072A50')
ax1.spines['top'].set_visible(False)
ax1.spines['right'].set_visible(False)
ax1.spines['left'].set_color('#ADB8C4')
ax1.spines['bottom'].set_color('#ADB8C4')
ax1.legend(loc='upper center', bbox_to_anchor=(0.5, -0.15), frameon=False, ncol=2)

# ==========================================
# GRÁFICO 2: O Resultado no Espaço 2D (A Curva)
# ==========================================
ax2.set_facecolor('#ffffff')
# Plotando o polígono de controle (linhas retas entre os pontos)
ax2.plot([P0[0], P1[0], P2[0], P3[0]], [P0[1], P1[1], P2[1], P3[1]], 
         color='#ADB8C4', linestyle='--', linewidth=1.5, marker='o', markersize=8, label='Polígono de Controle')

# Plotando a curva calculada pela soma algébrica
ax2.plot(curva_x, curva_y, color='#072A50', linewidth=3, label='Curva de Bézier (Bernstein)')

# Anotações dos pontos com as mesmas cores dos pesos para relacionar!
ax2.text(P0[0]-0.3, P0[1]-0.2, '$P_0$', fontsize=14, color='#20409A', fontweight='bold')
ax2.text(P1[0]-0.3, P1[1]+0.2, '$P_1$', fontsize=14, color='#BDDA45', fontweight='bold')
ax2.text(P2[0]+0.1, P2[1]+0.2, '$P_2$', fontsize=14, color='#FFAA49', fontweight='bold')
ax2.text(P3[0]+0.1, P3[1]-0.2, '$P_3$', fontsize=14, color='#8363A7', fontweight='bold')

ax2.set_title("2. Curva Resultante no Espaço", fontsize=14, color='#072A50', pad=15)
ax2.spines['top'].set_visible(False)
ax2.spines['right'].set_visible(False)
ax2.spines['left'].set_color('#ADB8C4')
ax2.spines['bottom'].set_color('#ADB8C4')
ax2.axis('equal')
ax2.legend(loc='upper center', bbox_to_anchor=(0.5, -0.15), frameon=False)

plt.tight_layout()
plt.show()