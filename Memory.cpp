#include "memory.h"

// Fonction interne qui gère les transactions CPU ou DMA
void Memory::process_transaction(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay) {
    unsigned char* data_ptr = trans.get_data_ptr();
    std::uint64_t addr = trans.get_address();
    unsigned int len = trans.get_data_length();

    if (addr + len > size) {
        trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
        return;
    }

    if (trans.get_command() == tlm::TLM_READ_COMMAND) {
        std::memcpy(data_ptr, &mem[addr], len);
        delay += sc_core::sc_time(10, sc_core::SC_NS);
    } else if (trans.get_command() == tlm::TLM_WRITE_COMMAND) {
        std::memcpy(&mem[addr], data_ptr, len);
        delay += sc_core::sc_time(15, sc_core::SC_NS);
    }

    trans.set_response_status(tlm::TLM_OK_RESPONSE);
}

// Définitions obligatoires pour le linker
void Memory::b_transport_cpu(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay) {
    process_transaction(trans, delay);
}

void Memory::b_transport_dma(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay) {
    process_transaction(trans, delay);
}

