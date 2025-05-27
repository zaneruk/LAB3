#include "server.hpp"
#include <sstream>
#include <numeric>
#include <vector>

AsyncTCPServer::AsyncTCPServer(boost::asio::io_context& io_context, short port)
    : io_context_(io_context), acceptor_(io_context, tcp::endpoint(tcp::v4(), port)) {
    start_accept();
}
std::string SyncTCPServer::process_request(const std::string& request) {
    public:
    SyncTCPServer(boost::asio::io_context& io_context, short port);
    void start();

private:
    void handle_client(tcp::socket socket);
    std::string process_request(const std::string& request);
    
    tcp::acceptor acceptor_;
};

void AsyncTCPServer::start_accept() {
    auto socket = std::make_shared<tcp::socket>(io_context_);
    
    acceptor_.async_accept(*socket,
        [this, socket](const boost::system::error_code& error) {
            if (!error) {
                auto buffer = std::make_shared<boost::asio::streambuf>();
                
                boost::asio::async_read_until(*socket, *buffer, '\n',
                    [this, socket, buffer](const boost::system::error_code& ec, size_t bytes) {
                        handle_read(*socket, *buffer, ec, bytes);
                    });
            }
            start_accept();
        });
}

void AsyncTCPServer::handle_read(tcp::socket& socket, boost::asio::streambuf& buffer,
                                const boost::system::error_code& error, size_t bytes_transferred) {
    if (!error) {
        std::istream is(&buffer);
        std::string data;
        std::getline(is, data);
        
        if (data.find(' ') != std::string::npos) {
            // Если есть пробелы - это запрос на вычисление среднего
            boost::asio::post(io_context_,
                [this, &socket, data]() {
                    calculate_average(socket, data);
                });
        } else {
            // Иначе - обычный калькулятор
            std::string response = process_request(data) + "\n";
            boost::asio::async_write(socket, boost::asio::buffer(response),
                [](const boost::system::error_code&, size_t) {});
        }
    }
}

void AsyncTCPServer::calculate_average(tcp::socket& socket, const std::string& data) {
    std::istringstream iss(data);
    std::vector<double> numbers;
    double num;
    
    while (iss >> num) {
        numbers.push_back(num);
    }
    
    if (numbers.empty()) {
        std::string response = "No numbers provided\n";
        boost::asio::async_write(socket, boost::asio::buffer(response),
            [](const boost::system::error_code&, size_t) {});
        return;
    }
    
    double sum = std::accumulate(numbers.begin(), numbers.end(), 0.0);
    double average = sum / numbers.size();
    
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << average;
    std::string response = "Среднее: " + oss.str() + "\n";
    
    boost::asio::async_write(socket, boost::asio::buffer(response),
        [](const boost::system::error_code&, size_t) {});
}