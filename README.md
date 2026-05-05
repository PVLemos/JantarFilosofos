# Grupo 6 

- Júlia Marques Boaventura - 8105
- Moisés Moreira - 8108
- Myrella Meireles - 9080
- Paulo Lemos - 9353

# Projeto - Jantar dos Filósofos

# Descrição

O problema do Jantar dos Filósofos é um clássico problema de sincronização e concorrência na ciência da computação, proposto por Edsger Dijkstra. Nele, 5 filósofos sentam-se em uma mesa redonda para alternar entre duas ações: comer e pensar. Cada filósofo precisa de dois garfos para comer (um à esquerda e um à direita), mas só existem 5 garfos na mesa, compartilhados entre eles. O desafio é criar um algoritmo que permita que todos os filósofos comam sem que ocorra um **deadlock** (quando todos pegam o garfo de um lado e ficam esperando infinitamente pelo outro) ou **starvation** (quando um filósofo morre de fome por nunca conseguir pegar os garfos).

# Solução

A solução implementada neste projeto utiliza a estratégia de **Ordem Total dos Recursos** (Resource Hierarchy).

Para evitar o ciclo de espera que causa o deadlock, cada garfo (representado por um mutex) recebe um ID numérico (de 0 a 4). A regra para evitar o travamento é: **cada filósofo deve sempre pegar o garfo de menor ID primeiro**.

Na prática:
- Os filósofos de 0 a 3 pegarão primeiro o garfo da esquerda (ID menor) e depois o da direita.
- O filósofo 4 (último da mesa) precisará dos garfos 4 (esquerda) e 0 (direita). Pela nossa regra, ele vai tentar pegar primeiro o garfo 0 e depois o garfo 4.

Essa simples quebra de simetria impede a espera circular. É impossível que todos os filósofos fiquem segurando um garfo e esperando o outro simultaneamente, eliminando completamente a chance de deadlock.

# Execução

```bash
# 1. Compilar (na pasta onde está o código)
gcc jantar_filosofos.c -o jantar_filosofos -pthread

# 2. Executar
./jantar_filosofos
```

# Diagrama

```text
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
