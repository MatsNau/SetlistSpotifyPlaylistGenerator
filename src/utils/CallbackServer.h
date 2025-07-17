#pragma once
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio.hpp>
#include <functional>
#include <memory>
#include <string>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

class CallbackServer {
public:
    explicit CallbackServer(net::io_context& ioc, uint16_t port);
    void start(std::function<void(const std::string&)> callback_handler);

private:
    void accept(std::function<void(const std::string&)> callback_handler);
    void handleConnection(tcp::socket socket, std::function<void(const std::string&)> callback_handler);

    class Connection : public std::enable_shared_from_this<Connection> {
    public:
        explicit Connection(tcp::socket socket);
        void start(std::function<void(const std::string&)> callback_handler);

    private:
        void handleRequest(std::function<void(const std::string&)> callback_handler);

        tcp::socket socket_;
        beast::flat_buffer buffer_;
        http::request<http::string_body> request_;
    };

    tcp::acceptor acceptor_;
};