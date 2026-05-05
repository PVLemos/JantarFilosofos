# 📄 **PROJETO 6 — Simulador do Jantar dos Filósofos (Deadlock Management)**
### Diagramas em ASCII / Texto

---

## **1. Arquitetura da Mesa**

```
   F0      F1      F2      F3      F4
  ( )----( )----( )----( )----( )
   |       |       |       |      |
  C0      C1      C2      C3      C4
```

---

## **2. Ciclo de Vida do Filósofo**

```
pensar -> pegar garfo esquerdo -> pegar garfo direito -> comer -> soltar -> pensar
```

---

## **3. Estratégias anti-deadlock**

```
- Ordem global de garfos
- Um filósofo pega garfos invertidos
- Máximo de 4 filósofos comendo simultaneamente
```

---
