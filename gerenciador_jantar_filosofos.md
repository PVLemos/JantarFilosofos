---

# ✅ **PROJETO 6 — Simulador do Jantar dos Filósofos (Gerenciamento de Deadlock)**

---

## 🎯 **Objetivo**
Implementar uma simulação do clássico **Jantar dos Filósofos**, de Dijkstra, com foco em:

- **Detecção e prevenção de deadlock**
- **Espera circular** e como quebrá-la
- Uso de **mutexes** para representar garfos
- Estratégias para evitar deadlock:
  - Ordem total dos recursos
  - Filósofo especial
  - Semáforo global (máx. 4 filósofos comendo)

O objetivo é **mostrar o problema** e **implementar uma solução correta**.

---

# 🧱 **Arquitetura do Sistema**

### **Componentes**
1. **N filósofos (threads)**
   - Ciclo infinito: pensar → pegar garfos → comer → largar garfos
   - Cada filósofo tem dois garfos: esquerdo e direito

2. **Garfos (mutexes)**
   - Um mutex por garfo
   - Cada garfo é compartilhado entre dois filósofos

3. **Estratégia anti-deadlock**
   - Ordem total: filósofos pegam sempre o garfo de menor ID primeiro
   - Isso **quebra a espera circular**, eliminando deadlock

---

# 📊 **Diagrama de Arquitetura (ASCII)**

```
        (F0)----fork0----(F1)
          |               |
        fork4           fork1
          |               |
        (F4)----fork3----(F2)
                  |
                fork2
                  |
                (F3)
```

Cada `(F#)` é um filósofo (thread).
Cada `fork#` é um mutex.

---

# 🧩 **Problema do Deadlock**

O deadlock ocorre quando:

1. Todos os filósofos pegam o garfo da esquerda
2. Todos ficam esperando o garfo da direita
3. Ninguém solta o garfo
4. O sistema trava para sempre

Isso é a **espera circular**, uma das quatro condições de deadlock.

---

# 🧪 **Solução: Ordem Total dos Recursos**

Cada filósofo pega primeiro o garfo de **menor ID**, depois o de maior ID.

Exemplo para filósofo i:

```
left  = i
right = (i + 1) % N

if (left < right):
    pegar(left)
    pegar(right)
else:
    pegar(right)
    pegar(left)
```

Isso **elimina ciclos**, logo elimina deadlock.

---

# 🧪 **Código Inicial em C (Skeleton)**
*(POSIX threads, mutexes, ordem total)*

```c
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define N 5

pthread_mutex_t forks[N];

void think(int id) {
    printf("Filosofo %d pensando...\n", id);
    usleep(100000);
}

void eat(int id) {
    printf("Filosofo %d comendo!\n", id);
    usleep(150000);
}

void *philosopher(void *arg) {
    int id = (int)(long)arg;

    int left  = id;
    int right = (id + 1) % N;

    while (1) {
        think(id);

        // Ordem total para evitar deadlock
        if (left < right) {
            pthread_mutex_lock(&forks[left]);
            pthread_mutex_lock(&forks[right]);
        } else {
            pthread_mutex_lock(&forks[right]);
            pthread_mutex_lock(&forks[left]);
        }

        eat(id);

        pthread_mutex_unlock(&forks[left]);
        pthread_mutex_unlock(&forks[right]);
    }
}

int main() {
    pthread_t phil[N];

    for (int i = 0; i < N; i++)
        pthread_mutex_init(&forks[i], NULL);

    for (int i = 0; i < N; i++)
        pthread_create(&phil[i], NULL, philosopher, (void *)(long)i);

    pthread_exit(NULL);
    return 0;
}
```

---

# 🛠️ **Roteiro Completo de Implementação**

### **1. Criar N mutexes**
- Cada mutex representa um garfo
- Inicializar com `pthread_mutex_init()`

### **2. Criar N threads filósofos**
- Cada thread recebe seu ID
- Ciclo infinito: pensar → comer

### **3. Implementar função `think()`**
- Apenas imprime e dorme

### **4. Implementar função `eat()`**
- Apenas imprime e dorme

### **5. Implementar estratégia anti-deadlock**
- Ordem total dos garfos
- Filósofo sempre pega primeiro o garfo de menor ID

### **6. Liberar garfos**
- Sempre liberar ambos após comer

### **7. Testar**
- Verificar que o programa nunca trava
- Aumentar número de filósofos
- Aumentar tempo de comer/pensar

---

# ⭐ Alternativas de Solução (opcionais)

### ✔ **Semáforo global (máx. 4 filósofos comendo)**
Evita deadlock limitando concorrência.

### ✔ **Filósofo especial**
Um filósofo pega garfos na ordem inversa.

### ✔ **Monitor com estados (pensando, faminto, comendo)**
Solução clássica de Hoare.

---
