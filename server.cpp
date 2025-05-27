#include "server.hpp"
#include <sstream>
#include <iostream>
#include <numeric>
#include <thread>

// SyncTCPServer implementation
SyncTCPServer::SyncTCPServer(boost::asio::io_context& io_context, short port)
    : acceptor_(io_context, tcp::endpoint(tcp::v4(), port)) {}

void SyncTCPServer::start() {
    while (true) {
        tcp::socket socket(acceptor_.get_executor());
        acceptor_.accept(socket);
        
        std::thread([this, sock = std::move(socket)]() mutable {
            handle_client(std::move(sock));
        }).detach();
    }
}

void SyncTCPServer::handle_client(tcp::socket socket) {
    try {
        boost::asio::streambuf buffer;
        boost::asio::read_until(socket, buffer, '\n');
        
        std::istream is(&buffer);
        std::string request;
        std::getline(is, request);
        
        std::string response = process_request(request) + "\n";
        boost::asio::write(socket, boost::asio::buffer(response));
    } catch (std::exception& e) {
        std::cerr << "Error in client handler: " << e.what() << std::endl;
    }
}

std::string SyncTCPServer::process_request(const std::string& request) {
    std::istringstream iss(request);
    double num1, num2;
    char op;
    
    if (!(iss >> num1 >> op >> num2)) {
        return "Invalid request format. Use: number op number";
    }
    
    double result;
    switch (op) {
        case '+': result = num1 + num2; break;
        case '-': result = num1 - num2; break;
        case '*': result = num1 * num2; break;
        case '/':
            if (num2 == 0) return "Division by zero";
            result = num1 / num2;
            break;
        default: return "Unknown operator";
    }
    
    std::ostringstream oss;
    if (op == '/') {
        oss << std::fixed << std::setprecision(2) << result;
    } else {
        oss << result;
    }
    
    return oss.str();
}

// AsyncTCPServer implementation
AsyncTCPServer::AsyncTCPServer(boost::asio::io_context& io_context, short port)
    : io_context_(io_context), acceptor_(io_context, tcp::endpoint(tcp::v4(), port)) {
    start_accept();
}

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
        
        if (data.find("remind ") == 0) {
            std::istringstream iss(data.substr(7));
            int seconds;
            std::string reminder;
            
            if (iss >> seconds && std::getline(iss, reminder)) {
                reminder.erase(0, reminder.find_first_not_of(" "));
                handle_reminder(socket, seconds, reminder);
            } else {
                std::string response = "Invalid remind format. Use: remind N message\n";
                boost::asio::async_write(socket, boost::asio::buffer(response),
                    [](const boost::system::error_code&, size_t) {});
            }
        } else if (data.find(' ') != std::string::npos) {
            boost::asio::post(io_context_,
                [this, &socket, data]() {
                    calculate_average(socket, data);
                });
        } else {
            std::string response = SyncTCPServer::process_request(data) + "\n";
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

void AsyncTCPServer::handle_reminder(tcp::socket& socket, int seconds, const std::string& reminder) {
    std::string ack = "Напоминание будет через " + std::to_string(seconds) + " секунд\n";
    boost::asio::async_write(socket, boost::asio::buffer(ack),
        [this, &socket, seconds, reminder](const boost::system::error_code&, size_t) {
            auto timer = std::make_shared<boost::asio::steady_timer>(io_context_);
            timer->expires_after(std::chrono::seconds(seconds));
            timer->async_wait(
                [this, &socket, reminder, timer](const boost::system::error_code& ec) {
                    if (!ec) {
                        std::string response = reminder + "\n";
                        boost::asio::async_write(socket, boost::asio::buffer(response),
                            [](const boost::system::error_code&, size_t) {});
                    }
                });
        });
}

// Session implementation
Session::Session(tcp::socket socket, 
                boost::asio::strand<boost::asio::io_context::executor_type> strand,
                std::vector<std::string>& log)
    : socket_(std::move(socket)), strand_(std::move(strand)), log_(log) {}

void Session::start() {
    do_read();
}

void Session::do_read() {
    auto self(shared_from_this());
    socket_.async_read_some(boost::asio::buffer(data_, max_length),
        boost::asio::bind_executor(strand_,
            [this, self](boost::system::error_code ec, std::size_t length) {
                if (!ec) {
                    std::string request(data_, length);
                    handle_request(request);
                }
            }));
}

void Session::handle_request(const std::string& request) {
    try {
        int number = std::stoi(request);
        uint64_t result = 1;
        
        for (int i = 2; i <= number; ++i) {
            result *= i;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        std::string response = "Factorial: " + std::to_string(result) + "\n";
        log_message("Processed request: " + request + " -> " + std::to_string(result));
        do_write(response);
    } catch (...) {
        std::string response = "Invalid input\n";
        log_message("Invalid request: " + request);
        do_write(response);
    }
}

void Session::do_write(const std::string& response) {
    auto self(shared_from_this());
    boost::asio::async_write(socket_, boost::asio::buffer(response),
        boost::asio::bind_executor(strand_,
            [this, self](boost::system::error_code ec, std::size_t /*length*/) {
                if (!ec) {
                    do_read();
                }
            }));
}

void Session::log_message(const std::string& message) {
    boost::asio::post(strand_,
        [this, message]() {
            log_.push_back(message);
            std::cout << "Logged: " << message << std::endl;
        });
}

// MultithreadedServer implementation
MultithreadedServer::MultithreadedServer(boost::asio::io_context& io_context, 
                                       short port, int thread_pool_size)
    : io_context_(io_context),
      acceptor_(io_context, tcp::endpoint(tcp::v4(), port)),
      strand_(boost::asio::make_strand(io_context)),
      thread_pool_size_(thread_pool_size) {
    do_accept();
}

void MultithreadedServer::start_accept() {
    do_accept();
}

void MultithreadedServer::do_accept() {
    acceptor_.async_accept(
        boost::asio::make_strand(io_context_),
        [this](boost::system::error_code ec, tcp::socket socket) {
            if (!ec) {
                std::make_shared<Session>(std::move(socket), strand_, log_)->start();
            }
            do_accept();
        });
}