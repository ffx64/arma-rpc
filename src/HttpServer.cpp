#include "HttpServer.h"

#include <iostream>

HttpServer::HttpServer(uint16_t port, PresenceController& controller)
    : port_(port),
      controller_(controller),
      limiter_(1.0, 2.0),
      running_(false) {
    ConfigureRoutes();
}

HttpServer::~HttpServer() {
    Stop();
}

bool HttpServer::Start() {
    if (running_) {
        return true;
    }

    running_ = true;
    serverThread_ = std::thread([this]() {
        try {
            app_.bindaddr("127.0.0.1").port(port_).run();
        } catch (const std::exception& ex) {
            running_ = false;
            std::cerr << "[error] HTTP server failed: " << ex.what() << "\n";
        }
    });

    std::cout << "[info] HTTP server started successfully on port " << port_ << "\n";
    return true;
}

void HttpServer::Stop() {
    if (!running_) {
        return;
    }

    running_ = false;
    app_.stop();

    if (serverThread_.joinable()) {
        serverThread_.join();
    }

    std::cout << "[info] HTTP server shutdown successfully\n";
}

bool HttpServer::IsRunning() const {
    return running_;
}

void HttpServer::ConfigureRoutes() {
    app_.route_dynamic("/endpoint/update")
        .methods(crow::HTTPMethod::POST)
        ([this](const crow::request& request) {
            if (!limiter_.Allow()) {
                return TextResponse(429, "Too many requests. Please try again later.");
            }

            int statusCode = 200;
            const std::string body = controller_.HandleUpdate(request.body, statusCode);
            return TextResponse(statusCode, body);
        });

    app_.route_dynamic("/endpoint/close")
        .methods(crow::HTTPMethod::POST)
        ([this]() {
            if (!limiter_.Allow()) {
                return TextResponse(429, "Too many requests. Please try again later.");
            }

            int statusCode = 200;
            const std::string body = controller_.HandleClose(statusCode);
            return TextResponse(statusCode, body);
        });
}

crow::response HttpServer::TextResponse(int statusCode, const std::string& body) const {
    crow::response response(statusCode, body);
    response.set_header("Content-Type", "text/plain; charset=utf-8");
    return response;
}
