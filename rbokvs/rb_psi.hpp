// rb_psi.hpp 
#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include "rb_okvs_gf2.hpp"  

namespace rbpsi {
using FE = rbokvs::FE2_128;

// Receiver: listen on port, run PSI with set X, return X ∩ Y.
std::vector<FE> psi_recv(uint16_t port, const std::vector<FE>& X);

// Sender: connect to host:port with set Y, send tags (no return).
void psi_send(const std::string& host, uint16_t port, const std::vector<FE>& Y);

}
