# Grupo 6

| Nome | Matrícula |
|------|-----------|
| Júlia Marques Boaventura | 8105 |
| Moisés Moreira | 8108 |
| Myrella Meireles | 9080 |
| Paulo Lemos | 9353 |

---

# Projeto 6 — Jantar dos Filósofos
### Gerenciamento de Deadlock — Sistemas Operacionais

---

## Descrição

O problema do Jantar dos Filósofos é um clássico problema de sincronização e concorrência na ciência da computação, proposto por Edsger Dijkstra. Nele, 5 filósofos sentam-se em uma mesa redonda para alternar entre duas ações: pensar e comer. Cada filósofo precisa de dois garfos para comer (um à esquerda e um à direita), mas só existem 5 garfos na mesa, compartilhados entre eles.

O desafio é criar um algoritmo que permita que todos os filósofos comam sem que ocorra:
- **Deadlock** — quando todos pegam o garfo de um lado e ficam esperando infinitamente pelo outro, travando o sistema.
- **Starvation** — quando um filósofo nunca consegue pegar os dois garfos e morre de fome.

---

## Arquitetura do Sistema

Cada filósofo é uma **thread POSIX** e cada garfo é um **mutex**. O ciclo de vida de cada filósofo é:

```
pensar → pegar garfos → comer → soltar garfos → pensar...
```

```
        (F0)----fork0----(F1)
          |                |
        fork4            fork1
          |                |
        (F4)----fork3----(F2)
                  |
                fork2
                  |
                (F3)
```

---

## Estratégias Implementadas

O simulador demonstra 5 estratégias, alternáveis em tempo real durante a execução:

| Tecla | Estratégia | Descrição |
|-------|-----------|-----------|
| `1` | **Ordem Total** | Cada filósofo pega sempre o garfo de menor ID primeiro, quebrando a espera circular |
| `2` | **Deadlock intencional** | Sem solução — todos pegam o garfo esquerdo simultaneamente e travam |
| `3` | **Filósofo especial** | O filósofo 0 inverte a ordem de aquisição, um único "rebelde" quebra o ciclo |
| `4` | **Semáforo global** | Limita a no máximo 4 filósofos tentando comer ao mesmo tempo (N-1) |
| `5` | **Monitor de Hoare** | Estados PENSANDO → FAMINTO → COMENDO com variáveis de condição |

### Solução principal — Ordem Total dos Recursos

Cada garfo recebe um ID numérico de 0 a 4. A regra é: **cada filósofo deve sempre pegar o garfo de menor ID primeiro**.

- Filósofos 0 a 3: pegam primeiro o garfo da esquerda (ID menor) e depois o da direita.
- Filósofo 4: precisa dos garfos 4 e 0. Pela regra, pega o garfo 0 primeiro e depois o 4.

Essa quebra de simetria impede a espera circular, eliminando completamente o deadlock.

```c
int first  = (left < right) ? left  : right;
int second = (left < right) ? right : left;

pthread_mutex_lock(&forks[first]);
pthread_mutex_lock(&forks[second]);
```

---

## Simulador Visual

O projeto inclui um simulador gráfico em tempo real desenvolvido com **SDL2**. A janela exibe:

- **Mesa interativa** — filósofos mudam de cor conforme o estado:
  - 🟢 Verde claro → pensando
  - 🟡 Âmbar → faminto
  - 🟢 Verde escuro → comendo
  - 🔴 Vermelho claro → esperando
  - 🔴 Vermelho → deadlock
- **Garfos** ficam amarelos quando segurados por um filósofo
- **Código C** da estratégia ativa exibido em tempo real
- **Estatísticas** — refeições, deadlocks detectados, tempo e fairness
- **Log** de todas as ações das threads em tempo real

---

## Requisitos

O projeto requer **Linux** ou **WSL** (Windows Subsystem for Linux) no Windows.

> **Windows:** Se estiver no Windows, você precisa usar o WSL. Para instalar o WSL, abra o PowerShell como administrador e execute:
> ```powershell
> wsl --install
> ```
> Após instalar, abra o terminal do WSL para seguir os passos abaixo.

---

## Instalação e Execução

### 1. Instalar as dependências

```bash
sudo apt update
sudo apt install gcc libsdl2-dev libsdl2-ttf-dev fonts-liberation
```

### 2. Compilar

```bash
gcc -Wall -pthread -o jantar jantar_filosofos.c \
    $(sdl2-config --cflags --libs) -lSDL2_ttf -lm
```

### 3. Executar

```bash
./jantar
```

A janela do simulador abrirá automaticamente.

---

## Controles

| Tecla | Ação |
|-------|------|
| `1` | Ordem total (padrão) |
| `2` | Deadlock intencional |
| `3` | Filósofo especial |
| `4` | Semáforo global |
| `5` | Monitor de Hoare |
| `Espaço` | Pausar / retomar simulação |
| `R` | Resetar estratégia atual |
| `ESC` / `Q` | Sair |

---

## As Quatro Condições de Coffman

Para que um deadlock ocorra, as quatro condições abaixo devem ser satisfeitas simultaneamente. A estratégia de **Ordem Total** elimina a condição 4, resolvendo o problema.

| # | Condição | No problema |
|---|----------|-------------|
| 1 | **Exclusão mútua** | Cada garfo só pode ser usado por um filósofo por vez |
| 2 | **Espera por recurso** | O filósofo segura um garfo enquanto espera o outro |
| 3 | **Não preempção** | Nenhum filósofo pode tomar o garfo de outro à força |
| 4 | **Espera circular** | F0 espera F1, F1 espera F2... F4 espera F0 ← **eliminada** |

---

## Tecnologias

- **Linguagem:** C (padrão C99)
- **Threads:** POSIX Threads (`pthread`)
- **Sincronização:** `pthread_mutex_t`, `pthread_cond_t`, `sem_t`
- **Interface gráfica:** SDL2 + SDL2_ttf
