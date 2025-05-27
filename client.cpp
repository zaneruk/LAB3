#include "client.hpp"
#include <iostream>

SyncTCPClient::SyncTCPClient(const std::string& server, const std::string& port)
    : resolver_(io_context_) {}

std::string SyncTCPClient::send_request(const std::string& request) {
    try {
        tcp::socket socket(io_context_);
        boost::asio::connect(socket, resolver_.resolve("127.0.0.1", "12345"));
        
        std::string req = request + "\n";
        boost::asio::write(socket, boost::asio::buffer(req));
        
        boost::asio::streambuf response;
        boost::asio::read_until(socket, response, '\n');
        
        std::istream is(&response);
        std::string result;
        std::getline(is, result);
        
        return result;
    } catch (std::exception& e) {
        return "Error: " + std::string(e.what());
    }
}