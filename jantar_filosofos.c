#include <pthread.h>
#include <stdio.h>
// #include <stdlib.h>
#include <unistd.h>

// O número total de filósofos (e também o número de garfos na mesa)
#define N 5

// Cada garfo é um mutex. Usamos mutex porque apenas um filósofo
pthread_mutex_t forks[N];

void think(int id) {
  printf("Filosofo %d pensando...\n", id);
  usleep(100000); // 100ms
}

void eat(int id) {
  printf("Filosofo %d comendo!\n", id);
  usleep(150000); // 150ms
}

void *philosopher(void *arg) {
  int id = (int)(long)arg;

  // Identificando os garfos da esquerda e da direita usando o ID do filósofo.
  int left = id;
  int right = (id + 1) % N;

  while (1) {
    think(id);

    /* Se todo mundo pegar o garfo da esquerda ao mesmo tempo, teremos deadlock.
    Para evitar a espera circular, eu obrigo os filósofos a sempre pegarem
    o garfo de MENOR ID primeiro. Isso quebra o ciclo */
    if (left < right) {
      pthread_mutex_lock(&forks[left]);
      pthread_mutex_lock(&forks[right]);
    } else {
      /* O último filósofo (id 4) terá left=4 e right=0. Ele entra nesse 'else'
      e pega o garfo 0 (right) primeiro, evitando o deadlock. */
      pthread_mutex_lock(&forks[right]);
      pthread_mutex_lock(&forks[left]);
    }

    eat(id);

    // Depois de comer, posso soltar os garfos em qualquer ordem.
    pthread_mutex_unlock(&forks[left]);
    pthread_mutex_unlock(&forks[right]);
  }
}

int main() {
  pthread_t phil[N];

  // Inicializar todos os mutexes antes de criar as threads.
  for (int i = 0; i < N; i++) {
    pthread_mutex_init(&forks[i], NULL);
  }

  // Criar as threads, uma para cada filósofo.
  for (int i = 0; i < N; i++) {
    pthread_create(&phil[i], NULL, philosopher, (void *)(long)i);
  }

  // Esperar as threads terminarem.
  for (int i = 0; i < N; i++) {
    pthread_join(phil[i], NULL);
  }

  // Limpar a sujeira (boa prática em C), destruindo os mutexes no final.
  for (int i = 0; i < N; i++) {
    pthread_mutex_destroy(&forks[i]);
  }

  return 0;
}
