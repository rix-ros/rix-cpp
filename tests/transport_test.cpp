/**
 * @file transport_test.cpp
 * @brief Integration tests for real transport implementations (TCP, SHM).
 *
 * Tests exercise the Acceptor and Stream interfaces using real sockets rather
 * than mocks. Each test is parameterized via TransportParams so new transports
 * (e.g. SHM) can be added with a single INSTANTIATE_TEST_SUITE_P block.
 *
 * Coverage includes: acceptor binding, client connection, endpoint correctness,
 * blocking mode, wait_readable/wait_writable/wait_exception, round-trip
 * messaging, multiple sequential messages, large payloads, and duplicate bind
 * failure.
 */

#include <future>
#include <thread>

#include <gtest/gtest.h>

#include "rix/core/common.hpp"
#include "rix/ipc/tcp_acceptor.hpp"
#include "rix/ipc/tcp_stream.hpp"
#include "rix/std_msgs/String.hpp"
#include "rix/sys_msgs/Operation.hpp"

using namespace rix;

// ---------------------------------------------------------------------------
// Params
// ---------------------------------------------------------------------------

struct TransportParams {
  std::string name;
  std::function<std::shared_ptr<Acceptor>(const Endpoint&)> acceptor_factory;
  std::function<std::shared_ptr<Stream>(const Endpoint&, bool)> stream_factory;
  Endpoint test_endpoint;
};

// ---------------------------------------------------------------------------
// Fixture
// ---------------------------------------------------------------------------

class TransportTest : public ::testing::TestWithParam<TransportParams> {
protected:
  void connect_client(const Endpoint& endpoint, std::function<void(std::shared_ptr<Stream>)> callback) {
    client_thread_ = std::thread([this, endpoint, callback]() {
      auto stream = GetParam().stream_factory(endpoint, true);
      EXPECT_NE(stream, nullptr);
      callback(stream);
    });
  }

  void TearDown() override {
    if (client_thread_.joinable()) {
      client_thread_.join();
    }
  }

private:
  std::thread client_thread_;
};

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

TEST_P(TransportTest, AcceptorBinds) {
  const auto& params = GetParam();
  auto acceptor = params.acceptor_factory(params.test_endpoint);
  ASSERT_NE(acceptor, nullptr);
  Endpoint local = acceptor->local_endpoint();
  EXPECT_FALSE(local.address.empty());
  EXPECT_NE(local.port, 0);
}

TEST_P(TransportTest, ClientConnects) {
  const auto& params = GetParam();
  auto acceptor = params.acceptor_factory(params.test_endpoint);
  ASSERT_NE(acceptor, nullptr);

  connect_client(acceptor->local_endpoint(), [](auto stream) {});

  Endpoint remote;
  auto server_stream = acceptor->accept(remote);
  EXPECT_NE(server_stream, nullptr);
  EXPECT_FALSE(remote.address.empty());
  EXPECT_NE(remote.port, 0);
}

TEST_P(TransportTest, EndpointsCorrect) {
  const auto& params = GetParam();
  auto acceptor = params.acceptor_factory(params.test_endpoint);
  ASSERT_NE(acceptor, nullptr);

  std::promise<Endpoint> client_local_promise;
  std::promise<Endpoint> client_remote_promise;

  connect_client(acceptor->local_endpoint(), [&](auto stream) {
    client_local_promise.set_value(stream->local_endpoint());
    client_remote_promise.set_value(stream->remote_endpoint());
  });

  Endpoint remote;
  auto server_stream = acceptor->accept(remote);
  ASSERT_NE(server_stream, nullptr);

  Endpoint client_local = client_local_promise.get_future().get();
  Endpoint client_remote = client_remote_promise.get_future().get();

  EXPECT_EQ(remote, client_local);
  EXPECT_EQ(client_remote, acceptor->local_endpoint());
}

TEST_P(TransportTest, DefaultBlocking) {
  const auto& params = GetParam();
  auto acceptor = params.acceptor_factory(params.test_endpoint);
  ASSERT_NE(acceptor, nullptr);
  EXPECT_TRUE(acceptor->get_blocking());
}

TEST_P(TransportTest, SetBlockingRoundTrip) {
  const auto& params = GetParam();
  auto acceptor = params.acceptor_factory(params.test_endpoint);
  ASSERT_NE(acceptor, nullptr);
  EXPECT_TRUE(acceptor->set_blocking(false));
  EXPECT_FALSE(acceptor->get_blocking());
  EXPECT_TRUE(acceptor->set_blocking(true));
  EXPECT_TRUE(acceptor->get_blocking());
}

TEST_P(TransportTest, WaitReadableTimeoutWhenNoClient) {
  const auto& params = GetParam();
  auto acceptor = params.acceptor_factory(params.test_endpoint);
  ASSERT_NE(acceptor, nullptr);
  EXPECT_FALSE(acceptor->wait_readable(Duration(0, 50'000'000))); // 50ms
}

TEST_P(TransportTest, WaitReadableReturnsTrueOnConnect) {
  const auto& params = GetParam();
  auto acceptor = params.acceptor_factory(params.test_endpoint);
  ASSERT_NE(acceptor, nullptr);

  std::promise<void> client_ready;
  connect_client(acceptor->local_endpoint(), [&](auto stream) { client_ready.set_value(); });

  client_ready.get_future().get();
  EXPECT_TRUE(acceptor->wait_readable(Duration(1, 0)));

  // consume the pending connection so TearDown doesn't leave a dangling thread
  acceptor->accept();
}

TEST_P(TransportTest, WaitExceptionHealthySocket) {
  const auto& params = GetParam();
  auto acceptor = params.acceptor_factory(params.test_endpoint);
  ASSERT_NE(acceptor, nullptr);
  EXPECT_FALSE(acceptor->wait_exception(Duration(0, 50'000'000))); // 50ms
}

TEST_P(TransportTest, MultipleSequentialAccepts) {
  const auto& params = GetParam();
  auto acceptor = params.acceptor_factory(params.test_endpoint);
  ASSERT_NE(acceptor, nullptr);

  Endpoint bound = acceptor->local_endpoint();
  std::vector<std::shared_ptr<Stream>> client_streams;
  const int count = 3;

  // connect all clients before accepting any, to avoid a deadlock between
  // connect_client (which only stores one thread) and sequential accepts
  std::vector<std::thread> clients;
  for (int i = 0; i < count; i++) {
    clients.emplace_back([&params, bound]() { auto stream = params.stream_factory(bound, true); });
  }

  std::vector<std::shared_ptr<Stream>> server_streams;
  std::set<int> remote_ports;
  for (int i = 0; i < count; i++) {
    Endpoint remote;
    auto stream = acceptor->accept(remote);
    EXPECT_NE(stream, nullptr);
    EXPECT_NE(remote.port, 0);
    remote_ports.insert(remote.port);
    server_streams.push_back(stream);
  }

  EXPECT_EQ(static_cast<int>(remote_ports.size()), count);

  for (auto& t : clients)
    t.join();
}

TEST_P(TransportTest, DuplicateBindFails) {
  const auto& params = GetParam();
  auto acceptor1 = params.acceptor_factory(params.test_endpoint);
  ASSERT_NE(acceptor1, nullptr);

  Endpoint bound = acceptor1->local_endpoint();
  ASSERT_NE(bound.port, 0);

  // Second acceptor on the same port - construction fails silently,
  // local_endpoint() should return an invalid (port 0) endpoint.
  auto acceptor2 = params.acceptor_factory(bound);
  EXPECT_EQ(acceptor2->local_endpoint().port, 0);
}

// ---------------------------------------------------------------------------
// Stream tests
// ---------------------------------------------------------------------------

TEST_P(TransportTest, StreamDefaultBlocking) {
  const auto& params = GetParam();
  auto acceptor = params.acceptor_factory(params.test_endpoint);

  std::promise<bool> client_blocking;
  connect_client(acceptor->local_endpoint(), [&](auto stream) { client_blocking.set_value(stream->get_blocking()); });

  acceptor->accept();
  EXPECT_TRUE(client_blocking.get_future().get());
}

TEST_P(TransportTest, StreamSetBlockingRoundTrip) {
  const auto& params = GetParam();
  auto acceptor = params.acceptor_factory(params.test_endpoint);

  std::promise<void> done;
  connect_client(acceptor->local_endpoint(), [&](auto stream) {
    EXPECT_TRUE(stream->set_blocking(false));
    EXPECT_FALSE(stream->get_blocking());
    EXPECT_TRUE(stream->set_blocking(true));
    EXPECT_TRUE(stream->get_blocking());
    done.set_value();
  });

  acceptor->accept();
  done.get_future().get();
}

TEST_P(TransportTest, StreamWaitReadableTimeoutWhenEmpty) {
  const auto& params = GetParam();
  auto acceptor = params.acceptor_factory(params.test_endpoint);

  std::promise<bool> result;
  std::promise<void> server_ready;

  connect_client(acceptor->local_endpoint(), [&](auto stream) {
    server_ready.get_future().get();                                  // wait until server is holding the stream
    result.set_value(stream->wait_readable(Duration(0, 50'000'000))); // 50ms
  });

  auto server_stream = acceptor->accept();
  server_ready.set_value(); // signal client: server stream is alive, don't close yet
  EXPECT_FALSE(result.get_future().get());
}

TEST_P(TransportTest, StreamWaitWritableReturnsTrue) {
  const auto& params = GetParam();
  auto acceptor = params.acceptor_factory(params.test_endpoint);

  std::promise<bool> result;
  connect_client(acceptor->local_endpoint(), [&](auto stream) {
    result.set_value(stream->wait_writable(Duration(0, 50'000'000))); // 50ms
  });

  acceptor->accept();
  EXPECT_TRUE(result.get_future().get());
}

TEST_P(TransportTest, StreamWaitExceptionHealthy) {
  const auto& params = GetParam();
  auto acceptor = params.acceptor_factory(params.test_endpoint);

  std::promise<bool> result;
  connect_client(acceptor->local_endpoint(), [&](auto stream) {
    result.set_value(stream->wait_exception(Duration(0, 50'000'000))); // 50ms
  });

  acceptor->accept();
  EXPECT_FALSE(result.get_future().get());
}

TEST_P(TransportTest, RoundTripMessage) {
  const auto& params = GetParam();
  auto acceptor = params.acceptor_factory(params.test_endpoint);

  std_msgs::String sent;
  sent.data = "hello rix";

  std::promise<std_msgs::String> received_promise;
  connect_client(acceptor->local_endpoint(), [&](auto stream) {
    stream->send_message(OPCODE::PUB_MESSAGE, sent);

    sys_msgs::Operation op;
    std_msgs::String reply;
    stream->recv_message(op, reply);
    received_promise.set_value(reply);
  });

  auto server_stream = acceptor->accept();
  ASSERT_NE(server_stream, nullptr);

  sys_msgs::Operation op;
  std_msgs::String server_received;
  EXPECT_TRUE(server_stream->recv_message(op, server_received));
  EXPECT_EQ(server_received, sent);

  std_msgs::String reply;
  reply.data = "hello back";
  server_stream->send_message(OPCODE::PUB_MESSAGE, reply);

  std_msgs::String client_received = received_promise.get_future().get();
  EXPECT_EQ(client_received, reply);
}

TEST_P(TransportTest, MultipleMessages) {
  const auto& params = GetParam();
  auto acceptor = params.acceptor_factory(params.test_endpoint);

  const int count = 5;
  std::vector<std::string> payloads = {"one", "two", "three", "four", "five"};

  std::promise<void> done;
  connect_client(acceptor->local_endpoint(), [&](auto stream) {
    for (const auto& p : payloads) {
      std_msgs::String msg;
      msg.data = p;
      EXPECT_TRUE(stream->send_message(OPCODE::PUB_MESSAGE, msg));
    }
    done.set_value();
  });

  auto server_stream = acceptor->accept();
  ASSERT_NE(server_stream, nullptr);

  for (int i = 0; i < count; i++) {
    sys_msgs::Operation op;
    std_msgs::String msg;
    EXPECT_TRUE(server_stream->recv_message(op, msg));
    EXPECT_EQ(msg.data, payloads[i]);
  }

  done.get_future().get();
}

TEST_P(TransportTest, LargeMessage) {
  const auto& params = GetParam();
  auto acceptor = params.acceptor_factory(params.test_endpoint);

  std_msgs::String sent;
  sent.data = std::string(1024 * 1024, 'x'); // 1 MB

  std::promise<void> done;
  connect_client(acceptor->local_endpoint(), [&](auto stream) {
    EXPECT_TRUE(stream->send_message(OPCODE::PUB_MESSAGE, sent));
    done.set_value();
  });

  auto server_stream = acceptor->accept();
  ASSERT_NE(server_stream, nullptr);

  sys_msgs::Operation op;
  std_msgs::String received;
  EXPECT_TRUE(server_stream->recv_message(op, received));
  EXPECT_EQ(received, sent);

  done.get_future().get();
}

TEST_P(TransportTest, WaitReadableReturnsTrueAfterSend) {
  const auto& params = GetParam();
  auto acceptor = params.acceptor_factory(params.test_endpoint);

  std::promise<void> sent;
  connect_client(acceptor->local_endpoint(), [&](auto stream) {
    std_msgs::String msg;
    msg.data = "ping";
    stream->send_message(OPCODE::PUB_MESSAGE, msg);
    sent.set_value();
  });

  auto server_stream = acceptor->accept();
  ASSERT_NE(server_stream, nullptr);

  sent.get_future().get();
  EXPECT_TRUE(server_stream->wait_readable(Duration(1, 0)));
}

// ---------------------------------------------------------------------------
// Instantiation
// ---------------------------------------------------------------------------

INSTANTIATE_TEST_SUITE_P(TCP,
                         TransportTest,
                         ::testing::Values(TransportParams{
                             .name = "TCP",
                             .acceptor_factory = [](const Endpoint& ep) { return std::make_shared<TCPAcceptor>(ep); },
                             .stream_factory = [](const Endpoint& ep,
                                                  bool blocking) { return std::make_shared<TCPStream>(ep, blocking); },
                             .test_endpoint = Endpoint("127.0.0.1", 0)}),
                         [](const auto& info) { return info.param.name; });
