#include <systemc>
#include <tlm>
#include "memory.h"
#include "uart_tlm.cpp"
#include "dma.cpp"
#include "processor.cpp"
using namespace sc_core;

int sc_main(int argc, char* argv[]) {
    // Instancier modules
    Processor cpu("CPU");
    Memory mem("MEMORY");   // mémoire avec deux sockets : CPU et DMA
    DMA dma("DMA");
    UartTLM uart("UART");

    // Connecter les sockets
    cpu.mem_socket.bind(mem.socket_cpu);    // CPU -> Memory socket CPU
    cpu.ctrl_socket.bind(dma.ctrl_socket);  // CPU -> DMA control
    dma.mem_socket.bind(mem.socket_dma);    // DMA -> Memory socket DMA
    dma.uart_socket.bind(uart.socket);      // DMA -> UART

    // ----------------------------
    // Création fichier VCD pour traces
    sc_core::sc_trace_file* tf = sc_core::sc_create_vcd_trace_file("simulation_trace");
    tf->set_time_unit(1, sc_core::SC_NS);

    // Signaux pour tracer l'activité
    sc_signal<bool> cpu_mem_access("cpu_mem_access");
    sc_signal<bool> dma_mem_access("dma_mem_access");

    // Ajouter traces
    sc_core::sc_trace(tf, cpu_mem_access, "CPU_MemAccess");
    sc_core::sc_trace(tf, dma_mem_access, "DMA_MemAccess");

    // Exemple : lier ces signaux dans tes modules pour mettre à 1 pendant les accès
    mem.cpu_access_signal = &cpu_mem_access;
    mem.dma_access_signal = &dma_mem_access;

    // ----------------------------
    // Lancer simulation (par exemple 1 us)
    sc_core::sc_start(1, sc_core::SC_US);

    // Fermer le fichier VCD
    sc_core::sc_close_vcd_trace_file(tf);

    return 0;
}

