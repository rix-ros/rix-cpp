#include "rix/ipc/endpoint.hpp"

#include <utility>

namespace rix {

Endpoint::Endpoint() : port(0) {}

Endpoint::Endpoint(std::string address, int port) : address(std::move(address)), port(port) {}

Endpoint::Endpoint(const msg::mediator::Endpoint& msg) : address(msg.address), port(msg.port) {}

Endpoint::Endpoint(const std::string& str) : port(0) {
  auto pos = str.find(':');
  if (pos != std::string::npos) {
    address = str.substr(0, pos);
    try {
      port = std::stoi(str.substr(pos + 1));
    } catch (const std::invalid_argument&) {
      port = -1;
    } catch (const std::out_of_range&) {
      port = -1;
    }
  } else {
    address = "";
    port = -1;
  }
}

bool Endpoint::operator<(const Endpoint& other) const {
  return address < other.address || (address == other.address && port < other.port);
}

bool Endpoint::operator==(const Endpoint& other) const { return address == other.address && port == other.port; }

bool Endpoint::operator!=(const Endpoint& other) const { return !(*this == other); }

std::string Endpoint::to_string() const { return address + ":" + std::to_string(port); }

} // namespace rix