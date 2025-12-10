# TLM_DMA_UART

## Description
Ce projet est une simulation **SystemC/TLM 2.0** représentant le flux de données entre un **CPU**, une **DMA**, une **FIFO**, un **UART** et la **mémoire**.  
L'objectif est de visualiser et d'analyser le transfert de données à travers ces modules à l'aide de traces **VCD**.

---

## Contenu du projet
- `main.cpp` : point d'entrée de la simulation SystemC.
- `Memory.cpp/h` : module mémoire.
- `dma.cpp` : module DMA.
- `processor.cpp` : module processeur.
- `uart_tlm.cpp` : module UART.
- `simulation_trace.vcd` : fichier de trace généré par la simulation.
- `push_github.sh` : script pour pousser automatiquement le projet sur GitHub.
- `tlm_flow.mp4` : animation du flux de données générée à partir des traces VCD.
- `server.py` : script Python pour créer l'animation.

---

## Installation et exécution
### Prérequis
- SystemC 3.0.2
- Python 3 avec `matplotlib` et `vcd` (ou `pyvcd`)

### Lancer la simulation SystemC
```bash
g++ -std=c++17 -I/usr/local/systemc-3.0.2/include \
    -L/usr/local/systemc-3.0.2/lib \
    main.cpp Memory.cpp dma.cpp processor.cpp uart_tlm.cpp \
    -lsystemc -o main_exec

./main_exec
