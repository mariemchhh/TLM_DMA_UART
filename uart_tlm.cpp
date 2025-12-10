#ifndef UART_TLM_H
#define UART_TLM_H

#include <systemc>
#include <tlm>
#include <tlm_utils/simple_target_socket.h>
#include <queue>
#include <iostream>

struct UartTLM : sc_core::sc_module {
    // Socket target TLM (réception des octets à transmettre)
    tlm_utils::simple_target_socket<UartTLM> socket;

    // FIFO interne pour stocker les octets avant émission physique
    sc_core::sc_fifo<unsigned char> tx_fifo;

    // Temps d'un bit (paramétrable)
    sc_core::sc_time bit_time;

    SC_CTOR(UartTLM)
    : socket("socket"), tx_fifo(32), bit_time(sc_core::sc_time(10, sc_core::SC_NS))
    {
        // Enregistre la méthode b_transport
        socket.register_b_transport(this, &UartTLM::b_transport);

        // Thread de transmission série (simulé)
        SC_THREAD(tx_thread);
        // Pas de sensibilité; on attend sur la fifo
    }

    // b_transport : attendu un WRITE de 1 octet
    void b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay) {
        // ne traiter que les écritures
        if (trans.get_command() == tlm::TLM_WRITE_COMMAND && trans.get_data_length() > 0) {
            unsigned char val = *trans.get_data_ptr();

            // mettre l'octet dans la fifo (découplage initiateur/uart)
            tx_fifo.write(val);

            // simuler un petit délai côté registre/periph
            delay += sc_core::sc_time(5, sc_core::SC_NS);

            trans.set_response_status(tlm::TLM_OK_RESPONSE);
        } else {
            trans.set_response_status(tlm::TLM_COMMAND_ERROR_RESPONSE);
        }
    }

    // Thread qui prend les octets et "émet" bit par bit (affichage console)
    void tx_thread() {
        while (true) {
            unsigned char b = tx_fifo.read(); // bloquant

            // start bit (0)
            print_time();
            std::cout << "[UART] start bit\n";
            wait(bit_time);

            // 8 bits LSB first
            for (int i = 0; i < 8; ++i) {
                bool bit = (b >> i) & 0x1;
                print_time();
                std::cout << "[UART] data bit " << i << " = " << bit << "\n";
                wait(bit_time);
            }

            // stop bit (1)
            print_time();
            std::cout << "[UART] stop bit\n";
            wait(bit_time);

            // afficher le caractère complet pour lisibilité
            print_time();
            std::cout << "[UART] transmitted char = '" << (char)b << "' (0x"
                      << std::hex << int(b) << std::dec << ")\n";
        }
    }

    // helper : afficher timestamp simulation
    void print_time() {
        std::cout << "[" << sc_core::sc_time_stamp() << "] ";
    }
};

#endif // UART_TLM_H
