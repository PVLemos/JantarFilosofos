---

# 🎓 **CURSO COMPLETO — PROJETO 6**
# **Simulador do Jantar dos Filósofos (Gerenciamento de Deadlock)**

---

# 📘 **Módulo 1 — Introdução ao Problema**

### 🎯 Objetivos de aprendizagem
Ao final deste módulo, o aluno será capaz de:

- Entender o problema clássico do Jantar dos Filósofos.
- Explicar como deadlocks surgem em sistemas concorrentes.
- Identificar as quatro condições necessárias para deadlock.
- Compreender estratégias de prevenção e evitação.

### 📚 Conteúdo
- O que é o Jantar dos Filósofos
- Por que é um problema clássico de sincronização
- As quatro condições de Coffman:
  - Exclusão mútua
  - Espera por recurso
  - Não preempção
  - Espera circular
- Como o deadlock ocorre no cenário clássico
- Por que esse problema é relevante hoje:
  - Sistemas operacionais
  - Bancos de dados
  - Redes
  - Sistemas distribuídos

### 📝 Atividade rápida
Explique por que a espera circular é a condição mais crítica para o deadlock no Jantar dos Filósofos.

---

# 📘 **Módulo 2 — Arquitetura do Sistema**

### 🎯 Objetivos
- Visualizar a arquitetura completa do problema.
- Entender como filósofos e garfos interagem.
- Compreender como deadlocks surgem naturalmente.

### 📚 Conteúdo
- Componentes principais:
  - Filósofos (threads)
  - Garfos (mutexes)
- Ciclo de vida do filósofo:
  - Pensar
  - Pegar garfos
  - Comer
  - Largar garfos
- Problema clássico:
  - Todos pegam o garfo da esquerda
  - Todos esperam o garfo da direita
  - Deadlock

### 🧠 Conceitos-chave
- Exclusão mútua
- Ordem total de recursos
- Prevenção de deadlock
- Evitar espera circular

---

# 📘 **Módulo 3 — Diagramas Explicativos**

Inclui:

- Arquitetura da mesa
- Fluxo de execução de um filósofo
- Diagrama de deadlock
- Diagrama da solução com ordem total
- Sequência de aquisição de garfos

*(Os diagramas serão integrados ao PDF/PPT quando você solicitar.)*

---

# 📘 **Módulo 4 — Implementação em C (Explicada Passo a Passo)**

### 🎯 Objetivos
- Entender cada parte do código.
- Saber como implementar a solução com ordem total.
- Saber como evitar deadlocks de forma determinística.

### 📚 Conteúdo
- Representação dos garfos com mutex
- Criação das threads filósofas
- Funções `think()` e `eat()`
- Estratégia anti-deadlock:
  - Sempre pegar primeiro o garfo de menor ID
  - Depois o de maior ID
- Por que isso funciona:
  - Remove a espera circular
  - Mantém exclusão mútua
  - Mantém progresso

### 🧩 Exercício guiado
Implemente uma versão que:

- Tem 5 filósofos
- Usa ordem total dos garfos
- Nunca entra em deadlock
- Imprime logs de cada ação

---

# 📘 **Módulo 5 — Exercícios Práticos**

### 🧪 Exercício 1 — Filósofo especial
Implemente a solução onde **um filósofo** pega os garfos na ordem inversa.

### 🧪 Exercício 2 — Semáforo global
Use um semáforo que permite apenas 4 filósofos tentarem comer simultaneamente.

### 🧪 Exercício 3 — Monitor com estados
Implemente a solução clássica de Hoare com estados:
- Pensando
- Faminto
- Comendo

### 🧪 Exercício 4 — Estatísticas
Meça:
- Tempo médio de espera
- Número de refeições por filósofo
- Fairness

---

# 📘 **Módulo 6 — Projeto Final**

### 🎓 Desafio
Implemente um **simulador completo do Jantar dos Filósofos** que:

- Usa mutexes para representar garfos
- Implementa uma estratégia anti-deadlock
- Garante fairness entre filósofos
- Permite configurar:
  - Número de filósofos
  - Tempo de pensar
  - Tempo de comer
- Gera logs detalhados
- Mede estatísticas de desempenho

### 🎯 Critérios de avaliação
- Ausência de deadlock
- Fairness
- Clareza do código
- Robustez
- Sincronização correta

---

# 📘 **Módulo 7 — Perguntas de Revisão**

1. Quais são as quatro condições para deadlock?
2. Por que a espera circular é essencial para o deadlock?
3. Como a ordem total dos garfos evita deadlock?
4. Por que mutexes são necessários?
5. O que é fairness?
6. Como evitar starvation?
7. Qual é a diferença entre prevenção e detecção de deadlock?

---
