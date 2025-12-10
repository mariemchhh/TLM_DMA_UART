#ifndef DMA_H
#define DMA_H

#include <systemc>
#include <tlm>
#include <tlm_utils/simple_target_socket.h>
#include <tlm_utils/simple_initiator_socket.h>
#include <queue>
#include <iostream>

// Simple DMA : target socket pour control, initiator sockets pour memory & uart
struct DMA : sc_core::sc_module {
    // socket pour recevoir commandes (Processor écrit des registres de contrôle)
    tlm_utils::simple_target_socket<DMA> ctrl_socket;

    // initiator sockets pour mémoire et pour uart (DMA lit mem, écrit uart)
    tlm_utils::simple_initiator_socket<DMA> mem_socket;
    tlm_utils::simple_initiator_socket<DMA> uart_socket;

    // FIFO interne pour buffer data (optionnel)
    sc_core::sc_fifo<unsigned char> dma_fifo;

    // Variables de configuration (registres)
    std::uint32_t src_addr;   // adresse source en mémoire
    std::uint32_t length;     // longueur en octets
    bool busy;

    SC_CTOR(DMA)
    : ctrl_socket("ctrl_socket"), mem_socket("mem_socket"),
      uart_socket("uart_socket"), dma_fifo(64), src_addr(0), length(0), busy(false)
    {
        // enregistrer le handler de contrôle
        ctrl_socket.register_b_transport(this, &DMA::b_transport_ctrl);

        // thread qui lit la fifo et envoie à l'uart (découplage)
        SC_THREAD(uart_forward_thread);
    }

    // Control b_transport : on attend des écritures simples sur le port contrôle
    // Convention simple : CPU écrit un bloc structuré en 12 octets : [src_addr(4), length(4), start(4)]
    void b_transport_ctrl(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay) {
        if (trans.get_command() != tlm::TLM_WRITE_COMMAND) {
            trans.set_response_status(tlm::TLM_COMMAND_ERROR_RESPONSE);
            return;
        }

        unsigned char* data = trans.get_data_ptr();
        unsigned int len = trans.get_data_length();

        if (len < 12) {
            // on force un format minimal
            trans.set_response_status(tlm::TLM_BURST_ERROR_RESPONSE);
            return;
        }

        // décoder
        // src_addr (little endian)
        src_addr = (data[0]) | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
        length = (data[4]) | (data[5] << 8) | (data[6] << 16) | (data[7] << 24);
        std::uint32_t start_flag = (data[8]) | (data[9] << 8) | (data[10] << 16) | (data[11] << 24);

        // réponse immédiate (contrôle)
        trans.set_response_status(tlm::TLM_OK_RESPONSE);

        if (start_flag != 0 && !busy) {
            // démarrer la copie asynchrone (thread sc_spawn pour ne pas bloquer b_transport)
            busy = true;
            SC_REPORT_INFO("DMA", "Starting DMA transfer");
            // Spawn a method to perform the transfer so b_transport finishes quickly
            sc_core::sc_spawn(sc_core::sc_bind(&DMA::do_transfer, this));
        }
    }

    // Méthode qui effectue le transfert : lit mémoire via mem_socket, met dans fifo pour l'uart_forward_thread
    void do_transfer() {
        // on utilisera des transactions TLM par octet (simplification)
        for (std::uint32_t offset = 0; offset < length; ++offset) {
            // construire une transaction READ sur mem_socket
            tlm::tlm_generic_payload trans;
            sc_core::sc_time delay = sc_core::SC_ZERO_TIME;
            unsigned char data_buf = 0;

            trans.set_command(tlm::TLM_READ_COMMAND);
            trans.set_address(src_addr + offset);
            trans.set_data_ptr(&data_buf);
            trans.set_data_length(1);
            trans.set_streaming_width(1);
            trans.set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);

            // Appel bloquant b_transport (synchrones) sur mem_socket
            mem_socket->b_transport(trans, delay);

            if (trans.get_response_status() != tlm::TLM_OK_RESPONSE) {
                SC_REPORT_ERROR("DMA", "Memory read error during DMA");
                break;
            }

            // attendre le délai renvoyé par le memory target
            wait(delay);

            // mettre dans la fifo interne
            dma_fifo.write(data_buf);
        }

        // signale fin de transfert pour information (optionnel)
        busy = false;
        SC_REPORT_INFO("DMA", "DMA transfer completed");
    }

    // Thread qui lit la fifo interne et envoie octet par octet vers l'UART via uart_socket b_transport
    void uart_forward_thread() {
        while (true) {
            unsigned char b = dma_fifo.read(); // bloquant sur FIFO

            // Construire transaction write
            tlm::tlm_generic_payload trans;
            sc_core::sc_time delay = sc_core::SC_ZERO_TIME;
            trans.set_command(tlm::TLM_WRITE_COMMAND);
            trans.set_address(0); // UART n'utilise pas l'adresse ici
            trans.set_data_ptr(&b);
            trans.set_data_length(1);
            trans.set_streaming_width(1);
            trans.set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);

            // Appel bloquant vers le UART target
            uart_socket->b_transport(trans, delay);

            // attendre le délai imposé par l'uart
            wait(delay);
        }
    }
};

#endif // DMA_H
