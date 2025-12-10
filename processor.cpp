#ifndef PROCESSOR_H
#define PROCESSOR_H

#include <systemc>
#include <tlm>
#include <tlm_utils/simple_initiator_socket.h>
#include <cstring>
#include <iostream>

struct Processor : sc_core::sc_module {
    // initiator socket pour control (écrire registres DMA)
    tlm_utils::simple_initiator_socket<Processor> ctrl_socket;

    // initiator pour écrire la mémoire directement (testbench style)
    tlm_utils::simple_initiator_socket<Processor> mem_socket;

    SC_CTOR(Processor)
    : ctrl_socket("ctrl_socket"), mem_socket("mem_socket")
    {
        SC_THREAD(main_thread);
    }

    // Thread principal du processeur : initialise la mémoire et lance le DMA
    void main_thread() {
        wait(10, sc_core::SC_NS); // petit délai avant démarrage

        // Exemple de données à copier : "HELLO DMA\n"
        const char payload[] = "HELLO DMA\n";
        const unsigned int n = sizeof(payload) - 1; // sans le \0
        const std::uint32_t mem_addr = 0x100; // adresse de destination en mémoire

        // 1) écrire les données dans la mémoire via mem_socket (writes byte par byte)
        for (unsigned int i = 0; i < n; ++i) {
            unsigned char b = payload[i];
            tlm::tlm_generic_payload trans;
            sc_core::sc_time delay = sc_core::SC_ZERO_TIME;

            trans.set_command(tlm::TLM_WRITE_COMMAND);
            trans.set_address(mem_addr + i);
            trans.set_data_ptr(&b);
            trans.set_data_length(1);
            trans.set_streaming_width(1);
            trans.set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);

            mem_socket->b_transport(trans, delay);

            if (trans.get_response_status() != tlm::TLM_OK_RESPONSE) {
                SC_REPORT_ERROR("Processor", "Memory write failed");
            }

            wait(delay); // respecter le délai renvoyé
        }

        std::cout << "[" << sc_core::sc_time_stamp() << "] Processor: memory initialized\n";

        // 2) construire le registre de contrôle DMA : [src_addr(4), length(4), start(4)]
        unsigned char ctrl_block[12];
        // src_addr little endian
        ctrl_block[0] = (mem_addr & 0xFF);
        ctrl_block[1] = ((mem_addr >> 8) & 0xFF);
        ctrl_block[2] = ((mem_addr >> 16) & 0xFF);
        ctrl_block[3] = ((mem_addr >> 24) & 0xFF);
        // length
        ctrl_block[4] = (n & 0xFF);
        ctrl_block[5] = ((n >> 8) & 0xFF);
        ctrl_block[6] = ((n >> 16) & 0xFF);
        ctrl_block[7] = ((n >> 24) & 0xFF);
        // start flag = 1
        ctrl_block[8]  = 1;
        ctrl_block[9]  = 0;
        ctrl_block[10] = 0;
        ctrl_block[11] = 0;

        // 3) écrire le registre de contrôle au DMA
        tlm::tlm_generic_payload trans;
        sc_core::sc_time delay = sc_core::SC_ZERO_TIME;
        trans.set_command(tlm::TLM_WRITE_COMMAND);
        trans.set_address(0); // adresse d'écriture contrôle (non utilisée par DMA dans cet exemple)
        trans.set_data_ptr(ctrl_block);
        trans.set_data_length(12);
        trans.set_streaming_width(12);
        trans.set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);

        ctrl_socket->b_transport(trans, delay);

        if (trans.get_response_status() != tlm::TLM_OK_RESPONSE) {
            SC_REPORT_ERROR("Processor", "Failed to write DMA control");
        }

        wait(delay);
        std::cout << "[" << sc_core::sc_time_stamp() << "] Processor: DMA started\n";

        // Fin du scénario : attendre suffisamment que le DMA transmette tout
        wait(2000, sc_core::SC_NS);

        // Fin simulation volontaire
        sc_core::sc_stop();
    }
};

#endif // PROCESSOR_H
