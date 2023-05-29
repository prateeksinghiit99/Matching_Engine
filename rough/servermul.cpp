#include <iostream>
#include <boost/asio.hpp>
#include <unordered_map>
#include <vector>
#include <string>

using boost::asio::ip::tcp;

class Server {
public:
    Server(boost::asio::io_context& ioContext, short port)
        : acceptor_(ioContext, tcp::endpoint(tcp::v4(), port))
    {
        startAccept();
    }

private:
    void startAccept()
    {
        auto newConnection = std::make_shared<Connection>(acceptor_.get_executor().context());
        acceptor_.async_accept(newConnection->socket(),
            [this, newConnection](const boost::system::error_code& error) {
                handleAccept(newConnection, error);
            });
    }

    void handleAccept(std::shared_ptr<Connection> connection, const boost::system::error_code& error)
    {
        if (!error) {
            connections_.emplace(connection.get(), connection);
            connection->start();
        }

        startAccept();
    }

    class Connection : public std::enable_shared_from_this<Connection> {
    public:
        explicit Connection(boost::asio::io_context& ioContext)
            : socket_(ioContext)
        {
        }

        tcp::socket& socket()
        {
            return socket_;
        }

        void start()
        {
            asyncRead();
        }

    private:
        void asyncRead()
        {
            auto self(shared_from_this());
            socket_.async_read_some(boost::asio::buffer(buffer_),
                [this, self](const boost::system::error_code& error, std::size_t bytesRead) {
                    handleRead(error, bytesRead);
                });
        }

        void handleRead(const boost::system::error_code& error, std::size_t bytesRead)
        {
            if (!error) {
                std::string request(buffer_.data(), bytesRead);
                processRequest(request);
                asyncRead();
            } else {
                connections_.erase(this);
            }
        }

        void processRequest(const std::string& request)
        {
            // Simulate order matching logic
            std::string response;
            if (orderMatches(request)) {
                response = "Order matched!";
            } else {
                response = "Order received.";
            }

            asyncWrite(response);
        }

        void asyncWrite(const std::string& response)
        {
            auto self(shared_from_this());
            boost::asio::async_write(socket_, boost::asio::buffer(response),
                [this, self](const boost::system::error_code& error, std::size_t /*bytesSent*/) {
                    if (error) {
                        connections_.erase(this);
                    }
                });
        }

        bool orderMatches(const std::string& order)
        {
            // Implement your order matching logic here
            // Return true if order matches, false otherwise
            return false;
        }

        tcp::socket socket_;
        std::array<char, 1024> buffer_;
    };

    tcp::acceptor acceptor_;
    std::unordered_map<void*, std::shared_ptr<Connection>> connections_;
};

int main()
{
    boost::asio::io_context ioContext;

    // Create and run the server on port 8080
    Server server(ioContext, 8080);

    // Start the IO context
    ioContext.run();

    return 0;
}

