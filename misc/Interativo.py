import numpy as np
import matplotlib.pyplot as plt
from matplotlib.widgets import Slider

# ==========================================
# 1. Configuração e Matemática Base
# ==========================================
P0 = np.array([0.0, 0.0])
P1 = np.array([1.5, 4.0])
P2 = np.array([3.5, 4.0])
P3 = np.array([5.0, 0.0])
pontos = [P0, P1, P2, P3]

cores = ['#20409A', '#BDDA45', '#FFAA49', '#8363A7']
nomes = ['$P_0$', '$P_1$', '$P_2$', '$P_3$']

# Função para calcular os pesos de Bernstein
def bernstein(t):
    b0 = (1 - t)**3
    b1 = 3 * t * (1 - t)**2
    b2 = 3 * (t**2) * (1 - t)
    b3 = t**3
    return [b0, b1, b2, b3]

# Gerando todos os pontos para desenhar a curva estática de fundo
t_array = np.linspace(0, 1, 100)
curva_x, curva_y = [], []
for t_val in t_array:
    pesos = bernstein(t_val)
    x = sum(pesos[i] * pontos[i][0] for i in range(4))
    y = sum(pesos[i] * pontos[i][1] for i in range(4))
    curva_x.append(x)
    curva_y.append(y)

# ==========================================
# 2. Configuração Visual da Janela
# ==========================================
fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(15, 6))
fig.patch.set_facecolor('#ffffff')

# Ajusta os espaços para evitar qualquer sobreposição e abrir espaço para o slider
plt.subplots_adjust(bottom=0.25, wspace=0.3) 

# --- GRÁFICO 1: Funções de Bernstein ---
ax1.set_title("1. Força dos Ímãs (Pesos de Bernstein)", fontsize=14, color='#072A50')
b_funcs = [bernstein(t) for t in t_array]
for i in range(4):
    ax1.plot(t_array, [b[i] for b in b_funcs], color=cores[i], linewidth=2, label=nomes[i])

# Linha vertical indicadora do 't' atual
linha_t = ax1.axvline(x=0.0, color='red', linestyle='--', linewidth=1.5, zorder=5)

# CORREÇÃO: Textos movidos para DENTRO do Gráfico 1 (canto superior direito)
textos_peso = []
for i in range(4):
    txt = ax1.text(0.75, 0.95 - i*0.08, '', transform=ax1.transAxes, color=cores[i], fontsize=12, fontweight='bold')
    textos_peso.append(txt)

ax1.set_xlabel("Valor de t (Tempo)", fontsize=10)
ax1.set_ylabel("Força de Atração", fontsize=10)
ax1.set_xlim(0, 1)
ax1.set_ylim(-0.05, 1.05)

# --- GRÁFICO 2: Curva e Atração Magnética ---
ax2.set_title("2. Trajetória da Curva", fontsize=14, color='#072A50')
ax2.plot([p[0] for p in pontos], [p[1] for p in pontos], color='#ADB8C4', linestyle='--', label='Polígono de Controle')
ax2.plot(curva_x, curva_y, color='#ADB8C4', linewidth=2, zorder=1)

for i in range(4):
    ax2.plot(pontos[i][0], pontos[i][1], marker='o', color=cores[i], markersize=8)
    ax2.text(pontos[i][0]+0.1, pontos[i][1]-0.2, nomes[i], color=cores[i], fontsize=12, fontweight='bold')

ponto_animado, = ax2.plot([], [], marker='o', color='red', markersize=10, zorder=10)

linhas_ima = []
for i in range(4):
    linha, = ax2.plot([], [], color=cores[i], linestyle='-', zorder=2)
    linhas_ima.append(linha)

ax2.axis('equal')

# ==========================================
# 3. Interatividade (O Slider)
# ==========================================
ax_slider = plt.axes([0.2, 0.1, 0.6, 0.05], facecolor='#eeeeee')
slider_t = Slider(ax=ax_slider, label='Controle do Tempo (t)', valmin=0.0, valmax=1.0, valinit=0.0, color='red')

def update(val):
    t_val = slider_t.val
    linha_t.set_xdata([t_val, t_val])
    pesos = bernstein(t_val)
    
    for i in range(4):
        textos_peso[i].set_text(f'{nomes[i]}: {pesos[i]*100:.1f}%')
        
    px = sum(pesos[i] * pontos[i][0] for i in range(4))
    py = sum(pesos[i] * pontos[i][1] for i in range(4))
    ponto_animado.set_data([px], [py])
    
    for i in range(4):
        linhas_ima[i].set_data([px, pontos[i][0]], [py, pontos[i][1]])
        linhas_ima[i].set_linewidth(1 + 8 * pesos[i]) 
        linhas_ima[i].set_alpha(max(0.1, pesos[i]))   

slider_t.on_changed(update)
update(0)

plt.show()