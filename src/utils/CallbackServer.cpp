#include "CallbackServer.h"
#include <iostream>

CallbackServer::CallbackServer(net::io_context& ioc, uint16_t port)
    : acceptor_(ioc, { net::ip::make_address("127.0.0.1"), port }) {
}

void CallbackServer::start(std::function<void(const std::string&)> callback_handler) {
    accept(callback_handler);
}

void CallbackServer::accept(std::function<void(const std::string&)> callback_handler) {
    acceptor_.async_accept(
        [this, callback_handler](beast::error_code ec, tcp::socket socket) {
            if (!ec) {
                handleConnection(std::move(socket), callback_handler);
            }
            accept(callback_handler);
        });
}

void CallbackServer::handleConnection(tcp::socket socket, std::function<void(const std::string&)> callback_handler) {
    auto connection = std::make_shared<Connection>(std::move(socket));
    connection->start(
        [callback_handler](const std::string& code) {
            callback_handler(code);
        }
    );
}

// Connection class implementation
CallbackServer::Connection::Connection(tcp::socket socket)
    : socket_(std::move(socket)) {
}

void CallbackServer::Connection::start(std::function<void(const std::string&)> callback_handler) {
    auto self = shared_from_this();
    http::async_read(
        socket_,
        buffer_,
        request_,
        [self, callback_handler](beast::error_code ec, std::size_t) {
            if (!ec) {
                self->handleRequest(callback_handler);
            }
        });
}

void CallbackServer::Connection::handleRequest(std::function<void(const std::string&)> callback_handler) {
    // Parse the query string to get the authorization code
    std::string target = std::string(request_.target());
    size_t code_pos = target.find("code=");
    if (code_pos != std::string::npos) {
        std::string code = target.substr(code_pos + 5);
        size_t end_pos = code.find('&');
        if (end_pos != std::string::npos) {
            code = code.substr(0, end_pos);
        }
        callback_handler(code);
    }

    // Send response
    http::response<http::string_body> response{ http::status::ok, request_.version() };
    response.set(http::field::server, "CallbackServer");
    response.set(http::field::content_type, "text/html");
    response.body() = "Authorization successful! You can close this window.";
    response.prepare_payload();

    http::async_write(
        socket_,
        response,
        [self = shared_from_this()](beast::error_code ec, std::size_t) {
            self->socket_.shutdown(tcp::socket::shutdown_send, ec);
        });
}