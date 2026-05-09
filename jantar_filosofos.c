#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>
#include <math.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

/*Configurações iníciais*/

#define N   5
#define WIN_W   960
#define WIN_H   640
#define FPS 60
#define PI 3.14159

/*Tempos de simuação para os filosófos*/

#define T_PENSAR 600
#define T_COMER 500
#define T_VARIACAO 200

/*Fontes*/

#define FONT_PATH_REGULAR "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf"
#define FONT_PATH_BOLD    "/usr/share/fonts/truetype/liberation/LiberationSans-Bold.ttf"
#define FONT_PATH_MONO    "/usr/share/fonts/truetype/liberation/LiberationMono-Regular.ttf"

/*Estratégias*/ 

typedef enum {
    S_ORDER = 0,
    S_DEADLOCK = 1,
    S_SPECIAL = 2,
    S_SEMAPHORE = 3,
    S_MONITOR = 4
} Strategy;

static const char *STRAT_NAMES[] = {
    "Ordem Total", "Deadlock", "Filosofo Especial", "Semaforo Global", "Monitor"
};

static const char *STRAT_DESC[] = {
    "Garfo de menor ID primeiro -> elimina a espera circular",
    "Todos pegam o garfo da esquerda -> espera circular -> deadlock",
    "Filosofo 0 inverte a ordem, um rebelde basta",
    "Semaforo se limita em 4 filosofos simultaneos (N-1)",
    "Estados PENSANDO->fAMINTO->COMENDO, vizinhos verificados"
};

static const char *CODE_LINES[][8] = {
    { /* Ordem Total */
      "if (left < right) {",
      "  lock(&forks[left]);",
      "  lock(&forks[right]);",
      "} else {  // quebra ciclo",
      "  lock(&forks[right]);",
      "  lock(&forks[left]); }",
      "",
      ""  
    },
    { /* Deadlock */
      "// sem solucao -> trava!",
      "lock(&forks[left]);",
      "// todos esperam direito",
      "lock(&forks[right]);",
      "// espera circular!",
      "// sistema travado...",
      "",
      ""
    },
    { /* Folosofo Especial */
      "if (id == 0) { // rebelde",
      "  lock(&forks[right]);",
      "  lock(&forks[left]);",
      "} else {",
      "  lock(&forks[left]);",
      "  lock(&forks[right]); }",
      "",
      ""
    },
    { /*Semaforo*/ 
      "sem_wait(&sem_mesa);",
      "// maximo N-1 filosofos",
      "lock(&forks[left]);",
      "lock(&forks[right]);",
      "unlock(&forks[right]);",
      "unlock(&forks[left]);",
      "sem_post(&sem_mesa);",
      ""
    },
    { /*Monitor*/ 
      "estado[id] = FAMINTO;",
      "testar(id);",
      "while (estado[id] != COMENDO)",
      "  cond_wait(&pode_comer[id], &monitor_mutex);",
      "// come...",
      "estado[id] = PENSANDO;",   
      "testar(vizinho_esq);",     
      "testar(vizinho_dir);"
    }      

};

/*Estado do Filosofo*/
typedef enum {ST_THINK = 0, ST_HUNGRY, ST_EAT, ST_WAIT, ST_DEAD} PhilState;
static const char *STATE_NAMES[] = {"pensando", "faminto", "comendo", "esperando", "deadlock"};

/*Cores da tela*/
typedef struct { Uint8 r,g,b,a; } Color;

static Color COL_BG       = {18, 18, 20, 255};
static Color COL_PANEL    = {28, 28, 32, 255};
static Color COL_BORDER   = {55, 55, 65, 255};
static Color COL_TEXT     = {220,218,212,255};
static Color COL_MUTED    = {120,118,112,255};
static Color COL_TABLE    = {38, 38, 45, 255};
static Color COL_RING     = {55, 55, 65, 255};

/*Estado -> cor de fundo de cada filósofo*/
static Color STATE_COL[] = {
    {159, 225, 203, 255}, /*pensando = verde claro*/
    {250, 199, 117, 255}, /*faminto = âmbar*/
    {30, 160, 120, 255}, /*comendo = verde escuro*/
    {240, 149, 149, 255}, /*esperando = verde claro*/
    {200, 40, 40, 255}, /*deadlock = vermelho*/
};
static Color STATE_TEXT[] = {
    {8, 80, 65,255},
    {99, 56, 6,255},
    {255,255,255,255},
    {80, 20, 20,255},
    {255,255,255,255}
};

static Color COL_FORK_FREE= {80, 80, 95,255};
static Color COL_FORK_HELD= {250,199,117,255};
static Color COL_GREEN    = {93,202,165,255};
static Color COL_RED      = {226, 75, 74,255};
static Color COL_AMBER    = {239,159, 39,255};
static Color COL_BLUE     = {55, 139,221,255};

/*Estado global compartilhado*/

static volatile Strategy g_strategy = S_ORDER;
static volatile int g_running = 0;
static volatile int g_quit = 0;
static volatile int g_deadlocked = 0;

static volatile PhilState g_state[N];
static volatile int g_fork_owner[N]; /*quando o estado estiver em -1 quer dizer que está livre*/
static volatile int       g_meals[N];
static volatile int       g_deadlocks;
static volatile int       g_sem_count;
static volatile long      g_start_ms;
static volatile int       g_total_meals;

/*Monitor*/
#define MON_THINK 0 
#define MON_HUNGRY 1
#define MON_EAT 2
static volatile int g_mon_state[N]; /*estado de cada filósofo no monitor*/

static pthread_mutex_t g_fork_mutex[N]; /*cada garfo é protegido por um mutex, isso impede que 2 filosofos peguem o mesmo garfo*/
static sem_t g_sem_mesa; 
static pthread_mutex_t g_mon_mutex;
static pthread_cond_t  g_mon_cond[N];

static pthread_mutex_t g_state_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_t g_threads[N];
static volatile int g_threads_alive = 0;

/*Log círcular*/
#define LOG_MAX 120
#define LOG_LINE 80
static char g_log[LOG_MAX][LOG_LINE];
static int g_log_type[LOG_MAX]; /*0=normal, 1=comendo, 2=esperar, 3=deadlock, 4=informação*/
static int g_log_head = 0, g_log_count = 0;
static pthread_mutex_t g_log_mutex = PTHREAD_MUTEX_INITIALIZER; /*garante que 2 thread não escrevam no log ao mesmo tempo*/

/*Log*/
static long now_ms(void){
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000L + ts.tv_nsec / 1000000L;
}

static void sim_log(const char *msg, int type){
    pthread_mutex_lock(&g_log_mutex); /*trava o mutex*/
    long t = g_running ? now_ms() - g_start_ms : 0;
    snprintf(g_log[g_log_head], LOG_LINE, "[%4.1fs] %s", t/1000.0, msg); /*utilizado para evitar overflow de buffer*/
    g_log_type[g_log_head] = type;
    g_log_head = (g_log_head + 1) % LOG_MAX;
    if (g_log_count < LOG_MAX) g_log_count++;
    pthread_mutex_unlock(&g_log_mutex);
}

/*Núcleo da simulação (thread)*/

static void sim_sleep(int ms) {
    int var = (g_strategy == S_DEADLOCK) ? 0 : (int)(rand() % T_VARIACAO);
    struct timespec ts = { 0, (ms + var) * 1000000L };
    nanosleep(&ts, NULL);
}

static void do_think(int id){
    pthread_mutex_lock(&g_state_mutex); /*trava o mutex*/
    g_state[id] = ST_THINK; /*altera o estado do filósofo -> pensando*/
    pthread_mutex_unlock(&g_state_mutex); /*libera o mutex*/
    sim_sleep(T_PENSAR); /*faz ele pensar*/
}

static void do_eat(int id){
    char buf[64];
    pthread_mutex_lock(&g_state_mutex);
    g_state[id] = ST_EAT; /*altera o estado -> comendo*/
    g_meals[id]++;
    g_total_meals++;
    pthread_mutex_unlock(&g_state_mutex);
    snprintf(buf, sizeof(buf), "F%d comendo! (refeicao #%d)", id, g_meals[id]);
    sim_log(buf, 1);
    sim_sleep(T_COMER); /*mantém o filósofo ocupado*/
}

static void release_forks(int f1, int f2){
    pthread_mutex_lock(&g_state_mutex);
    g_fork_owner[f1] = -1; /*garfo livre*/
    g_fork_owner[f2] = -1; /*garfo livre*/
    pthread_mutex_unlock(&g_state_mutex);
}

static void acquire_fork_vis(int id, int fork_id){ /*registra que o filósofo id pegou o garfo*/
    pthread_mutex_lock(&g_state_mutex);
    g_fork_owner[fork_id] = id;
    pthread_mutex_unlock(&g_state_mutex);
}

static void set_state_vis(int id, PhilState s){
    pthread_mutex_lock(&g_state_mutex);
    g_state[id] = s;
    pthread_mutex_unlock(&g_state_mutex);
}

/*Ordem Total*/

static void *phil_order(void *arg) {
    int id    = (int)(long)arg;
    int left  = id; /*garfo da esquerda = seu índice*/
    int right = (id + 1) % N; /*garfo da direita = próximo na mesa (circuçar)*/
    int first = (left < right) ? left  : right; /*sempre pegar o garfo de menor índice primeiro*/
    int second= (left < right) ? right : left;
    char buf[64];

    while (g_running && !g_quit) {
        do_think(id);
        if (!g_running) break;

        set_state_vis(id, ST_HUNGRY);
        snprintf(buf, sizeof(buf), "F%d aguardando garfos (ordem total)...", id);
        sim_log(buf, 2);

        pthread_mutex_lock(&g_fork_mutex[first]); /*trava o garfo*/
        acquire_fork_vis(id, first); /*atualiza a visualização*/

        pthread_mutex_lock(&g_fork_mutex[second]);
        acquire_fork_vis(id, second);

        do_eat(id);

        pthread_mutex_unlock(&g_fork_mutex[second]);
        pthread_mutex_unlock(&g_fork_mutex[first]);
        release_forks(first, second);
    }
    return NULL;
}

static pthread_mutex_t g_barrier_mutex = PTHREAD_MUTEX_INITIALIZER; /*protege o contador*/
static pthread_cond_t  g_barrier_cond  = PTHREAD_COND_INITIALIZER; /*acorda as threads*/
static volatile int    g_barrier_count = 0; 

static void barrier_wait(void) {
    pthread_mutex_lock(&g_barrier_mutex); /*entra na região crítica*/
    g_barrier_count++; /*incrementa o contador -> vai marcar que mais uma thread chegou*/
    if (g_barrier_count >= N) {
        g_barrier_count = 0; /*reseta o contador*/
        pthread_cond_broadcast(&g_barrier_cond); /*acorda todas as threads que estavam esperando*/
    } else {
        pthread_cond_wait(&g_barrier_cond, &g_barrier_mutex);
    }
    pthread_mutex_unlock(&g_barrier_mutex);
}

/* Deadlock intencional*/
static void *phil_deadlock(void *arg) {
    int id    = (int)(long)arg;
    int left  = id;
    int right = (id + 1) % N;
    char buf[64];

    do_think(id);
    if (!g_running) return NULL;

    set_state_vis(id, ST_HUNGRY);

    barrier_wait(); /*todos esperam aqui antes de agir*/

    snprintf(buf, sizeof(buf), "F%d pegando garfo %d (esquerdo)...", id, left);
    sim_log(buf, 2);

    pthread_mutex_lock(&g_fork_mutex[left]);
    acquire_fork_vis(id, left);

    barrier_wait(); /* todos segurando o esquerdo — agora tenta o direito */

    snprintf(buf, sizeof(buf), "F%d esperando garfo %d (direito)...", id, right);
    sim_log(buf, 3);
    set_state_vis(id, ST_WAIT);

    pthread_mutex_lock(&g_fork_mutex[right]);
    acquire_fork_vis(id, right);
    do_eat(id);
    pthread_mutex_unlock(&g_fork_mutex[right]);
    pthread_mutex_unlock(&g_fork_mutex[left]);
    release_forks(left, right);
    return NULL;
}

/*Filósofo especial*/
/*Todos os filósofos fazem: pegar garfo esquerdo -> pegar garfo direito = deadlock*/
/*Filósofo 0: pega direito -> esquerdo e os demais pegam esquerdo -> direito = evita deadlock*/
static void *phil_special(void *arg) {
    int id    = (int)(long)arg;
    int left  = id;
    int right = (id + 1) % N;
    int first, second;
    char buf[64];

    if (id == 0) { first = right; second = left; } /*Filósofo 0 = rebelde, ele inverte a ordem*/
    else         { first = left;  second = right; }

    while (g_running && !g_quit) {
        do_think(id); /*pensa*/
        if (!g_running) break;

        set_state_vis(id, ST_HUNGRY); /*fica faminto*/
        if (id == 0)
            snprintf(buf, sizeof(buf), "F0 [ESPECIAL] pegando garfo %d (direito)...", first);
        else
            snprintf(buf, sizeof(buf), "F%d aguardando garfos...", id);
        sim_log(buf, 2);

        /*pega os garfos*/
        pthread_mutex_lock(&g_fork_mutex[first]);
        acquire_fork_vis(id, first);

        pthread_mutex_lock(&g_fork_mutex[second]);
        acquire_fork_vis(id, second);

        do_eat(id); /*come*/

        /*libera os garfos*/
        pthread_mutex_unlock(&g_fork_mutex[second]);
        pthread_mutex_unlock(&g_fork_mutex[first]);
        release_forks(first, second);
    }
    return NULL;
}

/*Semáforo global*/
/*Ideia: O semáforo é inicializado com N-1. Então, no máximo N-1 filósofos entram na disputa*/
/*Isso garante que sempre sobre pelo menos um garfo livre, logo não corre espera circular = sem deadlock*/
static void *phil_semaphore(void *arg) {
    int id    = (int)(long)arg;
    int left  = id;
    int right = (id + 1) % N;
    char buf[64];

    while (g_running && !g_quit) {
        do_think(id); /*pensar*/
        if (!g_running) break;

        set_state_vis(id, ST_HUNGRY); /*indica que o filósofo quer comer, mas ainda não pode*/
        snprintf(buf, sizeof(buf), "F%d aguardando vaga no semaforo...", id);
        sim_log(buf, 2);

        sem_wait(&g_sem_mesa); /*se já houver N-1 filósofos tentando comer*/
        pthread_mutex_lock(&g_state_mutex); /*essa thread bloqueia aqui*/
        g_sem_count++; /*atualiza os contadores*/
        pthread_mutex_unlock(&g_state_mutex);

        /*Agora ele pode competir pelos recursos*/
        pthread_mutex_lock(&g_fork_mutex[left]);
        acquire_fork_vis(id, left);

        pthread_mutex_lock(&g_fork_mutex[right]);
        acquire_fork_vis(id, right);

        do_eat(id); /*comer*/

        /*libera os garfos*/
        pthread_mutex_unlock(&g_fork_mutex[right]);
        pthread_mutex_unlock(&g_fork_mutex[left]);
        release_forks(left, right);

        sem_post(&g_sem_mesa); /*outro filósofo pode entrar*/
        pthread_mutex_lock(&g_state_mutex);
        g_sem_count--;
        pthread_mutex_unlock(&g_state_mutex);
    }
    return NULL;
}

/*Monitor de Hoare*/
/*Ideia geral: Em vez de cada filósofo disputar os garfos diretamente*/
/*Existe um controlador central (monitor) que decide: quem pode comer e quem deve esperar*/
static void mon_testar(int id) {
    int left  = (id + N - 1) % N;
    int right = (id + 1) % N;
    if (g_mon_state[id] == MON_HUNGRY && /*ele está faminto*/
        g_mon_state[left]  != MON_EAT && /*vizinho esquerdo não está comendo*/
        g_mon_state[right] != MON_EAT) /*vizinho direito não está comendo*/
    {
        g_mon_state[id] = MON_EAT; /*muda o estado para comendo*/
        pthread_cond_signal(&g_mon_cond[id]); /*acorda a thread que estava esperando*/
    }
}

static void mon_pegar(int id) {
    char buf[64];
    pthread_mutex_lock(&g_mon_mutex);
    g_mon_state[id] = MON_HUNGRY;
    set_state_vis(id, ST_HUNGRY);
    snprintf(buf, sizeof(buf), "F%d [monitor] FAMINTO — testando...", id);
    sim_log(buf, 2);
    mon_testar(id);
    while (g_mon_state[id] != MON_EAT && g_running)
        pthread_cond_wait(&g_mon_cond[id], &g_mon_mutex);
    /* Marca os garfos como ocupados visualmente */
    int left  = (id + N - 1) % N;
    int right = (id + 1) % N;
    acquire_fork_vis(id, left);
    acquire_fork_vis(id, right);
    pthread_mutex_unlock(&g_mon_mutex);
}

static void mon_largar(int id) {
    char buf[64];
    int left  = (id + N - 1) % N;
    int right = (id + 1) % N;
    pthread_mutex_lock(&g_mon_mutex);
    g_mon_state[id] = MON_THINK;
    release_forks(left, right);
    snprintf(buf, sizeof(buf), "F%d [monitor] soltando garfos, testando vizinhos...", id);
    sim_log(buf, 0);
    mon_testar(left);
    mon_testar(right);
    pthread_mutex_unlock(&g_mon_mutex);
}

static void *phil_monitor(void *arg) {
    int id = (int)(long)arg;
    while (g_running && !g_quit) {
        do_think(id);
        if (!g_running) break;
        mon_pegar(id);
        do_eat(id);
        mon_largar(id);
    }
    return NULL;
}

/*Detector de deadlock -> vai ficar rodando em loop e verificando se todos os filósofos estão esperando. Se sim, declara deadlock*/
static void *deadlock_detector(void *arg) {
    (void)arg;
    while (!g_quit) { /*a thread roda até o programa encerrar g_quit = 1*/
        usleep(400000);
        if (!g_running || g_strategy != S_DEADLOCK) continue; /*só vai verificar o deadlpck se: a simulação estiver rodando e a estratégia atual for de deadlock*/

        int waiting = 0;
        pthread_mutex_lock(&g_state_mutex);
        for (int i = 0; i < N; i++)
            if (g_state[i] == ST_WAIT || g_state[i] == ST_HUNGRY) waiting++;
        pthread_mutex_unlock(&g_state_mutex);

        if (waiting == N && !g_deadlocked) { 
            g_deadlocked = 1; /*marca que o sistema entrou em deadlock*/
            pthread_mutex_lock(&g_state_mutex);
            for (int i = 0; i < N; i++) g_state[i] = ST_DEAD;
            g_deadlocks++;
            pthread_mutex_unlock(&g_state_mutex);
            sim_log("DEADLOCK DETECTADO — espera circular!", 3);
        }
    }
    return NULL;
}

/*Gerenciamento de Threads*/
static void sim_reset(void) { /*reseta toda a simulação para o estado inicial*/
    pthread_mutex_lock(&g_state_mutex);
    for (int i = 0; i < N; i++) {
        g_state[i]      = ST_THINK; /*filósofo começa pensando*/
        g_fork_owner[i] = -1; /*ninguém tem garfo*/
        g_meals[i]      = 0; /*zera as refeições*/
        g_mon_state[i]  = MON_THINK; /*monitora o estado do monitor*/
    }
    /*todas variáveis globais são resetadas*/
    g_total_meals = 0;
    g_deadlocks   = 0;
    g_sem_count   = 0;
    g_deadlocked  = 0;
    g_barrier_count = 0;
    pthread_mutex_unlock(&g_state_mutex);

    pthread_mutex_lock(&g_log_mutex);
    g_log_head = 0; g_log_count = 0;
    pthread_mutex_unlock(&g_log_mutex);
}

static void sim_stop_threads(void) { /*Para as threads antigas*/
    if (!g_threads_alive) return;
    g_running = 0;

    /*Acorda threads bloqueadas*/
    for (int i = 0; i < N; i++) {
        pthread_mutex_trylock(&g_fork_mutex[i]); /*libera mutex se estiver preso*/
        pthread_mutex_unlock(&g_fork_mutex[i]); /*libera mutex se estiver preso*/
        pthread_cond_broadcast(&g_mon_cond[i]); /*acorda quem está esperando condição*/
        sem_post(&g_sem_mesa); /*libera semáforo*/
    }
    pthread_mutex_trylock(&g_mon_mutex);
    pthread_mutex_unlock(&g_mon_mutex);

    for (int i = 0; i < N; i++)
        pthread_join(g_threads[i], NULL);

    g_threads_alive = 0;
}

static void sim_init_sync(void) {
    for (int i = 0; i < N; i++) {
        pthread_mutex_init(&g_fork_mutex[i], NULL);
        pthread_cond_init(&g_mon_cond[i], NULL);
    }
    sem_init(&g_sem_mesa, 0, N - 1); /*permite no máximo N-1 filósofos na mesa -> evita deadlock*/
    pthread_mutex_init(&g_mon_mutex, NULL);
}

static void sim_start(Strategy s) { /*inicia uma nova simulação -> começa do zero*/
    sim_stop_threads();
    sim_reset(); /*reseta tudo*/

    /*Reinicia os mutexes limpos*/
    for (int i = 0; i < N; i++) {
        pthread_mutex_destroy(&g_fork_mutex[i]);
        pthread_mutex_init(&g_fork_mutex[i], NULL);
        pthread_cond_destroy(&g_mon_cond[i]);
        pthread_cond_init(&g_mon_cond[i], NULL);
    }
    sem_destroy(&g_sem_mesa);
    sem_init(&g_sem_mesa, 0, N - 1);
    pthread_mutex_destroy(&g_mon_mutex);
    pthread_mutex_init(&g_mon_mutex, NULL);

    
    pthread_mutex_destroy(&g_barrier_mutex);
    pthread_mutex_init(&g_barrier_mutex, NULL);
    pthread_cond_destroy(&g_barrier_cond);
    pthread_cond_init(&g_barrier_cond, NULL);
    g_barrier_count = 0;

    g_strategy = s;
    g_running  = 1;
    g_start_ms = now_ms();

    char buf[64];
    snprintf(buf, sizeof(buf), "Iniciando: %s", STRAT_NAMES[s]);
    sim_log(buf, 4);

    void *(*funcs[5])(void*) = {
        phil_order, phil_deadlock, phil_special, phil_semaphore, phil_monitor
    };
    for (int i = 0; i < N; i++)
        pthread_create(&g_threads[i], NULL, funcs[s], (void*)(long)i);

    g_threads_alive = 1;
}

/*Renderização com SDL*/

static SDL_Window   *win  = NULL;
static SDL_Renderer *ren  = NULL;
static TTF_Font     *font_sm  = NULL;  
static TTF_Font     *font_md  = NULL;  
static TTF_Font     *font_lg  = NULL;  
static TTF_Font     *font_mono= NULL;  

static void set_color(Color c) {
    SDL_SetRenderDrawColor(ren, c.r, c.g, c.b, c.a);
}

static void fill_rect(int x, int y, int w, int h, Color c) {
    SDL_Rect r = {x,y,w,h};
    set_color(c);
    SDL_RenderFillRect(ren, &r);
}

static void draw_rect_outline(int x, int y, int w, int h, Color c, int thickness) {
    for (int t = 0; t < thickness; t++) {
        SDL_Rect r = {x+t, y+t, w-2*t, h-2*t};
        set_color(c);
        SDL_RenderDrawRect(ren, &r);
    }
}

/*Círculo preenchido (midpoint algorithm)*/
static void fill_circle(int cx, int cy, int r, Color c) {
    set_color(c);
    for (int y = -r; y <= r; y++) {
        int dx = (int)sqrt((double)(r*r - y*y));
        SDL_RenderDrawLine(ren, cx-dx, cy+y, cx+dx, cy+y);
    }
}

/*Círculo borda*/
static void draw_circle(int cx, int cy, int r, Color c) {
    set_color(c);
    int x = r, y = 0, d = 1 - r;
    while (x >= y) {
        SDL_RenderDrawPoint(ren, cx+x, cy+y); SDL_RenderDrawPoint(ren, cx-x, cy+y);
        SDL_RenderDrawPoint(ren, cx+x, cy-y); SDL_RenderDrawPoint(ren, cx-x, cy-y);
        SDL_RenderDrawPoint(ren, cx+y, cy+x); SDL_RenderDrawPoint(ren, cx-y, cy+x);
        SDL_RenderDrawPoint(ren, cx+y, cy-x); SDL_RenderDrawPoint(ren, cx-y, cy-x);
        if (d < 0) d += 2*y+3; else { d += 2*(y-x)+5; x--; }
        y++;
    }
}

/*Renderiza texto com fonte e cor*/
static void render_text(const char *text, int x, int y,
                         TTF_Font *font, Color c,
                         int center_x, int center_y) {
    if (!text || !font) return;
    SDL_Color sc = {c.r, c.g, c.b, c.a};
    SDL_Surface *surf = TTF_RenderUTF8_Blended(font, text, sc);
    if (!surf) return;
    SDL_Texture *tex = SDL_CreateTextureFromSurface(ren, surf);
    SDL_Rect dst = {x, y, surf->w, surf->h};
    if (center_x) dst.x -= surf->w/2;
    if (center_y) dst.y -= surf->h/2;
    SDL_RenderCopy(ren, tex, NULL, &dst);
    SDL_DestroyTexture(tex);
    SDL_FreeSurface(surf);
}

static void render_text_c(const char *text, int x, int y, TTF_Font *f, Color c) {
    render_text(text, x, y, f, c, 1, 1);
}

static void render_text_l(const char *text, int x, int y, TTF_Font *f, Color c) {
    render_text(text, x, y, f, c, 0, 0);
}

/*Painel esquerdo: mesa*/
#define TABLE_CX  290
#define TABLE_CY  320
#define TABLE_R   220   /*raio dos filósofos*/
#define FORK_R    170   /*raio dos garfos*/
#define PHIL_R    52    /*raio do círculo do filósofo*/
#define FORK_SIZE 16    /*raio do ícone do garfo*/

static void draw_table(void) {
    /*Fundo do painel*/
    fill_rect(0, 0, 580, WIN_H, COL_BG);

    /*Mesa central*/
    fill_circle(TABLE_CX, TABLE_CY, 85, COL_TABLE);
    draw_circle(TABLE_CX, TABLE_CY, 85, COL_BORDER);
    render_text_c("mesa", TABLE_CX, TABLE_CY, font_sm, COL_MUTED);

    /*Anel guia*/
    for (int t = 0; t < 2; t++)
        draw_circle(TABLE_CX, TABLE_CY, TABLE_R + t, COL_RING);

    /*Garfos*/
    for (int i = 0; i < N; i++) {
        double fa = (2*PI*i/N - PI/2 + 2*PI*(0.5/N));
        int fx = (int)(TABLE_CX + FORK_R * cos(fa));
        int fy = (int)(TABLE_CY + FORK_R * sin(fa));

        int owner = g_fork_owner[i];
        Color fc = (owner >= 0) ? COL_FORK_HELD : COL_FORK_FREE;
        Color bc = (owner >= 0) ? COL_AMBER : COL_BORDER;

        fill_circle(fx, fy, FORK_SIZE, fc);
        draw_circle(fx, fy, FORK_SIZE, bc);

        char lbl[4];
        snprintf(lbl, sizeof(lbl), "f%d", i);
        render_text_c(lbl, fx, fy + FORK_SIZE + 10, font_sm, COL_MUTED);
    }

    /*Filósofos*/
    static const char *NAMES[N] = {"Plato","Aristoteles","Socrates","Descartes","Kant"};

    for (int i = 0; i < N; i++) {
        double a = 2*PI*i/N - PI/2;
        int px = (int)(TABLE_CX + TABLE_R * cos(a));
        int py = (int)(TABLE_CY + TABLE_R * sin(a));

        PhilState st = g_state[i];
        Color bg  = STATE_COL[st];
        Color txt = STATE_TEXT[st];

        /*Halo quando comendo*/
        if (st == ST_EAT) {
            Color halo = {30,160,120,60};
            fill_circle(px, py, PHIL_R + 8, halo);
        }
        if (st == ST_DEAD) {
            Color halo = {200,40,40,60};
            fill_circle(px, py, PHIL_R + 8, halo);
        }

        fill_circle(px, py, PHIL_R, bg);

        /*Borda*/
        Color border = (st==ST_EAT) ? COL_GREEN :
                       (st==ST_DEAD)? COL_RED    : COL_BORDER;
        int bthick = (st==ST_EAT||st==ST_DEAD) ? 2 : 1;
        for(int t=0;t<bthick;t++)
            draw_circle(px, py, PHIL_R - t, border);

        /*ID*/
        char id_str[4];
        snprintf(id_str, sizeof(id_str), "F%d", i);
        render_text_c(id_str, px, py - 10, font_md, txt);

        /*Estado*/
        render_text_c(STATE_NAMES[st], px, py + 10, font_sm, txt);

        /*Nome fora*/
        double nx = TABLE_CX + (TABLE_R + PHIL_R + 18) * cos(a);
        double ny = TABLE_CY + (TABLE_R + PHIL_R + 18) * sin(a);
        render_text_c(NAMES[i], (int)nx, (int)ny, font_sm, COL_MUTED);

        /*Badge de refeições*/
        int bx = px + PHIL_R - 12, by = py - PHIL_R + 12;
        fill_circle(bx, by, 12, COL_BLUE);
        char mc[4];
        snprintf(mc, sizeof(mc), "%d", g_meals[i]);
        render_text_c(mc, bx, by, font_sm, (Color){255,255,255,255});
    }
}

/*Painel direito*/
#define PX  588   /*x início painel*/
#define PW  (WIN_W - PX - 8)  /*largura painel*/

static void draw_panel(void) {
    int y = 10;

    /* Fundo painel */
    fill_rect(PX - 8, 0, WIN_W - PX + 8, WIN_H, COL_PANEL);
    SDL_SetRenderDrawColor(ren, COL_BORDER.r,COL_BORDER.g,COL_BORDER.b,255);
    SDL_RenderDrawLine(ren, PX-8, 0, PX-8, WIN_H);

    /*Título estratégia*/
    Color strat_col = (g_strategy==S_DEADLOCK) ? COL_RED : COL_GREEN;
    render_text_l(STRAT_NAMES[g_strategy], PX, y, font_lg, strat_col);
    y += 28;
    render_text_l(STRAT_DESC[g_strategy], PX, y, font_sm, COL_MUTED);
    y += 20;

    SDL_SetRenderDrawColor(ren, COL_BORDER.r,COL_BORDER.g,COL_BORDER.b,255);
    SDL_RenderDrawLine(ren, PX, y, WIN_W-8, y);
    y += 10;

    /*Código*/
    render_text_l("codigo C:", PX, y, font_sm, COL_MUTED);
    y += 18;

    fill_rect(PX, y, PW, 100, (Color){22,22,28,255});
    draw_rect_outline(PX, y, PW, 100, COL_BORDER, 1);
    int cy2 = y + 8;
    for (int l = 0; l < 6; l++) {
        const char *line = CODE_LINES[g_strategy][l];
        Color lc = (strstr(line,"//") || strstr(line,"/*"))
                   ? (Color){100,120,100,255}
                   : (strstr(line,"if")||strstr(line,"while")||strstr(line,"else"))
                     ? COL_BLUE
                     : COL_TEXT;
        render_text_l(line, PX+8, cy2, font_mono, lc);
        cy2 += 14;
    }
    y += 108;

    SDL_SetRenderDrawColor(ren, COL_BORDER.r,COL_BORDER.g,COL_BORDER.b,255);
    SDL_RenderDrawLine(ren, PX, y, WIN_W-8, y);
    y += 8;

    /*Stats*/
    long elapsed = g_running ? (now_ms() - g_start_ms) / 1000 : 0;
    int max_m = 0, min_m = 9999;
    for (int i = 0; i < N; i++) {
        if (g_meals[i] > max_m) max_m = g_meals[i];
        if (g_meals[i] < min_m) min_m = g_meals[i];
    }
    double fairness = max_m > 0
        ? 100.0 * (1.0 - (double)(max_m-min_m)/max_m) : 100.0;

    char buf[48];
    /*Grid 2x2*/
    int sx = PX, sw = PW/2 - 4, sh = 46;

    /* Refeições */
    fill_rect(sx, y, sw, sh, (Color){30,30,36,255});
    draw_rect_outline(sx, y, sw, sh, COL_BORDER, 1);
    render_text_l("refeicoes", sx+8, y+6, font_sm, COL_MUTED);
    snprintf(buf, sizeof(buf), "%d", g_total_meals);
    render_text_l(buf, sx+8, y+22, font_lg, COL_TEXT);

    /*Deadlocks*/
    fill_rect(sx+sw+4, y, sw, sh, (Color){30,30,36,255});
    draw_rect_outline(sx+sw+4, y, sw, sh, COL_BORDER, 1);
    render_text_l("deadlocks", sx+sw+12, y+6, font_sm, COL_MUTED);
    snprintf(buf, sizeof(buf), "%d", g_deadlocks);
    render_text_l(buf, sx+sw+12, y+22, font_lg, g_deadlocks>0?COL_RED:COL_TEXT);
    y += sh + 4;

    /*Tempo*/
    fill_rect(sx, y, sw, sh, (Color){30,30,36,255});
    draw_rect_outline(sx, y, sw, sh, COL_BORDER, 1);
    render_text_l("tempo", sx+8, y+6, font_sm, COL_MUTED);
    snprintf(buf, sizeof(buf), "%lds", elapsed);
    render_text_l(buf, sx+8, y+22, font_lg, COL_TEXT);

    /*Fairness*/
    fill_rect(sx+sw+4, y, sw, sh, (Color){30,30,36,255});
    draw_rect_outline(sx+sw+4, y, sw, sh, COL_BORDER, 1);
    render_text_l("fairness", sx+sw+12, y+6, font_sm, COL_MUTED);
    snprintf(buf, sizeof(buf), "%.0f%%", fairness);
    render_text_l(buf, sx+sw+12, y+22, font_lg, COL_GREEN);
    y += sh + 8;

    /*Barras por filósofo*/
    SDL_SetRenderDrawColor(ren, COL_BORDER.r,COL_BORDER.g,COL_BORDER.b,255);
    SDL_RenderDrawLine(ren, PX, y, WIN_W-8, y);
    y += 8;
    render_text_l("refeicoes por filosofo:", PX, y, font_sm, COL_MUTED);
    y += 18;

    static const char *FNAMES[N] = {"F0","F1","F2","F3","F4"};
    int bar_max = max_m > 0 ? max_m : 1;
    int bar_w = PW - 40;

    for (int i = 0; i < N; i++) {
        Color fc = STATE_COL[g_state[i]];
        render_text_l(FNAMES[i], PX, y+2, font_sm, COL_MUTED);
        /* Track */
        fill_rect(PX+26, y, bar_w, 12, (Color){38,38,45,255});
        /* Fill */
        int fill = (int)((double)g_meals[i]/bar_max * bar_w);
        if (fill > 0) fill_rect(PX+26, y, fill, 12, fc);
        /* Count */
        snprintf(buf, sizeof(buf), "%d", g_meals[i]);
        render_text_l(buf, PX+26+bar_w+4, y+2, font_sm, COL_MUTED);
        y += 18;
    }

    /*Semáforo counter (só no modo semáforo)*/
    if (g_strategy == S_SEMAPHORE) {
        y += 4;
        snprintf(buf, sizeof(buf), "Semaforo: %d/%d filosofos na mesa", g_sem_count, N-1);
        render_text_l(buf, PX, y, font_sm, COL_AMBER);
        y += 16;
    }

    SDL_SetRenderDrawColor(ren, COL_BORDER.r,COL_BORDER.g,COL_BORDER.b,255);
    SDL_RenderDrawLine(ren, PX, y, WIN_W-8, y);
    y += 8;

    /*Log*/
    render_text_l("log:", PX, y, font_sm, COL_MUTED);
    y += 18;

    int log_lines = (WIN_H - y - 55) / 14;
    if (log_lines < 1) log_lines = 1;

    pthread_mutex_lock(&g_log_mutex);
    int count = g_log_count < log_lines ? g_log_count : log_lines;
    int start = (g_log_head - count + LOG_MAX) % LOG_MAX;
    for (int i = 0; i < count; i++) {
        int idx = (start + i) % LOG_MAX;
        Color lc;
        switch (g_log_type[idx]) {
            case 1: lc = COL_GREEN;  break;
            case 2: lc = COL_AMBER;  break;
            case 3: lc = COL_RED;    break;
            case 4: lc = COL_BLUE;   break;
            default: lc = COL_MUTED; break;
        }
        render_text_l(g_log[idx], PX, y, font_mono, lc);
        y += 14;
    }
    pthread_mutex_unlock(&g_log_mutex);

    /*Teclas de controle*/
    int ky = WIN_H - 42;
    fill_rect(PX-8, ky, WIN_W-PX+8, 42, (Color){22,22,28,255});
    SDL_SetRenderDrawColor(ren, COL_BORDER.r,COL_BORDER.g,COL_BORDER.b,255);
    SDL_RenderDrawLine(ren, PX-8, ky, WIN_W, ky);

    render_text_l("[1]Ordem [2]Deadlock [3]Especial [4]Semaforo [5]Monitor",
                  PX, ky+6, font_sm, COL_MUTED);
    render_text_l("[ESPACO]Pausar  [R]Resetar  [ESC/Q]Sair",
                  PX, ky+22, font_sm, COL_MUTED);

    /* Status rodando/pausado */
    int sx2 = PX; int sy2 = ky - 22;
    const char *status_str = g_running ? "● rodando" : "■ pausado";
    Color status_col = g_running ? COL_GREEN : COL_RED;
    if (g_deadlocked) { status_str = "✕ DEADLOCK"; status_col = COL_RED; }
    render_text_l(status_str, sx2, sy2, font_md, status_col);
}

/*Linha de título superior*/
static void draw_title(void) {
    fill_rect(0, 0, 580, 32, (Color){24,24,28,255});
    SDL_SetRenderDrawColor(ren,COL_BORDER.r,COL_BORDER.g,COL_BORDER.b,255);
    SDL_RenderDrawLine(ren, 0, 32, 580, 32);
    render_text_l("Jantar dos Filosofos — Simulador Visual",
                  12, 8, font_md, COL_TEXT);
}

/*--------MAIN--------*/

int main(void) {
    srand((unsigned)time(NULL));

    /* SDL init */
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError()); return 1;
    }
    if (TTF_Init() < 0) {
        fprintf(stderr, "TTF_Init: %s\n", TTF_GetError()); return 1;
    }

    win = SDL_CreateWindow(
        "Jantar dos Filosofos — Simulador Visual",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WIN_W, WIN_H,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );
    if (!win) { fprintf(stderr, "Window: %s\n", SDL_GetError()); return 1; }

    ren = SDL_CreateRenderer(win, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!ren) { fprintf(stderr, "Renderer: %s\n", SDL_GetError()); return 1; }

    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);

    /* Fontes */
    font_sm   = TTF_OpenFont(FONT_PATH_REGULAR, 13);
    font_md   = TTF_OpenFont(FONT_PATH_BOLD,    16);
    font_lg   = TTF_OpenFont(FONT_PATH_BOLD,    20);
    font_mono = TTF_OpenFont(FONT_PATH_MONO,    12);

    if (!font_sm || !font_md || !font_lg || !font_mono) {
        fprintf(stderr, "Font: %s\n", TTF_GetError()); return 1;
    }

    /*Init sincronização*/
    sim_init_sync();
    sim_reset();

    /*Thread detector de deadlock*/
    pthread_t det_thread;
    pthread_create(&det_thread, NULL, deadlock_detector, NULL);

    /*Inicia com ordem total*/
    sim_start(S_ORDER);

    /*Loop principal*/
    SDL_Event ev;
    Uint32 frame_ms = 1000 / FPS;
    while (!g_quit) {
        Uint32 t0 = SDL_GetTicks();

        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) g_quit = 1;
            if (ev.type == SDL_KEYDOWN) {
                switch (ev.key.keysym.sym) {
                    case SDLK_ESCAPE:
                    case SDLK_q: g_quit = 1; break;

                    case SDLK_1: sim_start(S_ORDER);    break;
                    case SDLK_2: sim_start(S_DEADLOCK);  break;
                    case SDLK_3: sim_start(S_SPECIAL);   break;
                    case SDLK_4: sim_start(S_SEMAPHORE); break;
                    case SDLK_5: sim_start(S_MONITOR);   break;

                    case SDLK_SPACE:
                        if (g_running) {
                            g_running = 0;
                            sim_log("Simulacao pausada.", 4);
                        } else {
                            g_running = 1;
                            g_start_ms = now_ms();
                            sim_log("Simulacao retomada.", 4);
                        }
                        break;

                    case SDLK_r:
                        sim_start(g_strategy);
                        break;
                }
            }
        }

        /*Render*/
        set_color(COL_BG);
        SDL_RenderClear(ren);

        draw_table();
        draw_title();
        draw_panel();

        SDL_RenderPresent(ren);

        Uint32 dt = SDL_GetTicks() - t0;
        if (dt < frame_ms) SDL_Delay(frame_ms - dt);
    }

    /*Cleanup*/
    sim_stop_threads();
    g_quit = 1;
    pthread_join(det_thread, NULL);

    TTF_CloseFont(font_sm);
    TTF_CloseFont(font_md);
    TTF_CloseFont(font_lg);
    TTF_CloseFont(font_mono);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    TTF_Quit();
    SDL_Quit();

    for (int i = 0; i < N; i++) {
        pthread_mutex_destroy(&g_fork_mutex[i]);
        pthread_cond_destroy(&g_mon_cond[i]);
    }
    sem_destroy(&g_sem_mesa);
    pthread_mutex_destroy(&g_mon_mutex);

    return 0;
}
