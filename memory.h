#ifndef MEMORY_H
#define MEMORY_H

#include <systemc>
#include <tlm>
#include <tlm_utils/simple_target_socket.h>
#include <vector>
#include <cstring>

using namespace sc_core;

struct Memory : sc_module {
    // Sockets séparés pour CPU et DMA
    tlm_utils::simple_target_socket<Memory> socket_cpu;
    tlm_utils::simple_target_socket<Memory> socket_dma;

    // Taille et contenu de la mémoire
    std::size_t size;
    std::vector<unsigned char> mem;

    // Signaux optionnels pour VCD
    sc_signal<bool>* cpu_access_signal;
    sc_signal<bool>* dma_access_signal;

    SC_CTOR(Memory) : size(1024) {
        mem.resize(size, 0);

        socket_cpu.register_b_transport(this, &Memory::b_transport_cpu);
        socket_dma.register_b_transport(this, &Memory::b_transport_dma);

        cpu_access_signal = nullptr;
        dma_access_signal = nullptr;
    }

    // Déclarations des méthodes b_transport
    void b_transport_cpu(tlm::tlm_generic_payload& trans, sc_time& delay);
    void b_transport_dma(tlm::tlm_generic_payload& trans, sc_time& delay);

private:
    void process_transaction(tlm::tlm_generic_payload& trans, sc_time& delay);
};

#endif // MEMORY_H
