#include "rix/ipc/endpoint.hpp"

namespace rix {

Endpoint::Endpoint() : address(""), port(0) {}

Endpoint::Endpoint(const std::string &address, int port)
    : address(address), port(port) {}

Endpoint::Endpoint(const msg::mediator::Endpoint &msg)
    : address(msg.address), port(msg.port) {}

Endpoint::Endpoint(const std::string &str) : address(""), port(0) {
  auto pos = str.find(':');
  if (pos != std::string::npos) {
    address = str.substr(0, pos);
    try {
      port = std::stoi(str.substr(pos + 1));
    } catch (const std::invalid_argument &e) {
      port = -1;
    } catch (const std::out_of_range &e) {
      port = -1;
    }
  } else {
    address = "";
    port = -1;
  }
}

bool Endpoint::operator<(const Endpoint &other) const {
  return address < other.address ||
         (address == other.address && port < other.port);
}

bool Endpoint::operator==(const Endpoint &other) const {
  return address == other.address && port == other.port;
}

bool Endpoint::operator!=(const Endpoint &other) const {
  return !(*this == other);
}

std::string Endpoint::to_string() const {
  return address + ":" + std::to_string(port);
}

} // namespace rix