#include <iostream>
#include <boost/asio.hpp>
#include <thread>
#include <mutex>
#include <ctime>
#include <unordered_map>
#include <memory>
#include <random>
#include <chrono> 

using boost::asio::ip::tcp;
using std::string;
using std::cout;
using std::cin;
using std::getline;
using std::stringstream;
using std::to_string;
using std::lock_guard;
using std::mutex;
using std::unordered_map;
using std::shared_ptr;
using std::make_shared;
using std::time_t;
using std::thread;

struct Order {
    int clientId;
    int orderId;
    char type;
    double price;
    int quantity;
    time_t time;
};

class Client {
public:
    Client(boost::asio::io_service& ioService, const string& serverIP, short serverPort)
        : socket_(ioService), serverIP_(serverIP), serverPort_(serverPort), isRunning_(true)
    {
        connect();
    }

    void run()
    {
        // Start the receiving thread
        thread receiveThread(&Client::receiveMessages, this);

        // Run the main thread for user input and sending orders
        while (isRunning_) {
            readInput();
        }

        // Wait for the receiving thread to finish
        receiveThread.join();
    }

private:
    // Private members
    tcp::socket socket_;
    string serverIP_;
    short serverPort_;
    bool isRunning_;
	bool isStarted_;
    mutex mutex_;
	
    unordered_map<int, shared_ptr<Order>> orders_;
    unordered_map<int, shared_ptr<Order>> filledOrders_;

    // Private methods
    void connect()
    {
        tcp::resolver resolver(socket_.get_io_service());
        tcp::resolver::query query(serverIP_, to_string(serverPort_));
        tcp::resolver::iterator endpointIterator = resolver.resolve(query);

        boost::asio::connect(socket_, endpointIterator);
    }

	void readInput()
	{
		string input;
		getline(cin, input);

		if (input == "start") {
		    isStarted_ = true;
		    
		    // Start the order generation thread
		    thread generateThread(&Client::generateOrders, this);

		    // Wait for the user to type "stop"
		    while (isStarted_) {
		        getline(cin, input);
		        if (input == "stop") {
		            isStarted_ = false;
		            break;
		        }
		    }

		    // Wait for the order generation thread to finish
		    generateThread.join();
		}
	}

	void generateOrders()
	{
		std::random_device rd;  // Seed for generator, non-deterministic random numbers from os
		std::mt19937 gen(rd()); // Random number generator engine gen [Mersenne Twister algorithm] 
		std::uniform_int_distribution<> priceDist(1, 10); // Randomly generate price between 1 and 10
		std::uniform_int_distribution<> quantityDist(1, 1000); // Randomly generate quantity between 1 and 1000
		std::bernoulli_distribution typeDist(0.5); // Randomly generate order type

		while (isStarted_) {
			// Sleep for 100 milliseconds
		    std::this_thread::sleep_for(std::chrono::milliseconds(100));

		    Order order;
		    order.type = typeDist(gen) ? 'B' : 'S';
		    order.price = priceDist(gen);
		    order.quantity = quantityDist(gen);
		    order.orderId = generateOrderId();
		    orders_[order.orderId] = make_shared<Order>(order);
		    
		    // Print the generated order
		    cout << "Generated Order: ";
		    cout << "Type: " << order.type << ", ";
		    cout << "Price: " << order.price << ", ";
		    cout << "Quantity: " << order.quantity << "\n";

		    // Send order
			sendOrder(order);
		}
		
	}

    void sendOrder(const Order& order)
    {
        boost::asio::write(socket_, boost::asio::buffer(&order, sizeof(order)));
    }

    void receiveMessages()
    {
        while (isRunning_) {
            Order order;
            size_t bytesRead = socket_.read_some(boost::asio::buffer(&order, sizeof(order)));

            // Acquire a lock to prevent interleaved output with user input
            lock_guard<mutex> lock(mutex_);

            // Interpret the received message
            interpretReceivedMessage(order);
        }
    }

    void interpretReceivedMessage(Order &order)
    {
        int orderId = order.orderId;
        switch(order.type) {
            case 'X':
                orders_.erase(orderId);
                cout << "Invalid Order\n";
                break;
            case 'O':
                orders_.erase(orderId);
                cout << "Price Not in Range Set by Exchange\n";
                break;
            case 'W':
                cout << "Welcome! You are ClientID " << to_string(order.clientId) << "\n";
                break;
            case 'A':
                order.quantity = 0;
                filledOrders_[orderId] = make_shared<Order>(order);
                cout << "Order Placed! OrderId: " << to_string(orderId) << "\n";
                break;
            case 'B':
            case 'S':
                int quantity;
                double price;
                price = weightedAveragePrice(order, *filledOrders_[orderId]);
                filledOrders_[orderId]->quantity += order.quantity;
                filledOrders_[orderId]->price = price;

                // Displaying result
                cout << "OrderId: " << to_string(orderId) << " | ";
                cout << to_string(filledOrders_[orderId]->quantity) << "/" << to_string(orders_[orderId]->quantity);
                cout << " Avg. Price: " << to_string(filledOrders_[orderId]->price);

                // Display Status and Clean OrderBook
                if (filledOrders_[orderId]->quantity == orders_[orderId]->quantity) {
                    orders_.erase(orderId);
                    filledOrders_.erase(orderId);
                    cout << " [Completed]\n";
                }
                else {
                    cout << " [Partially Filled]\n";
                }
                break;
            default:
                break;
        }
    }

    double weightedAveragePrice(Order &order1, Order &order2)
    {
        double cost1 = order1.price * static_cast<double>(order1.quantity);
        double cost2 = order2.price * static_cast<double>(order2.quantity);
        double total_quantity = static_cast<double>(order1.quantity) + static_cast<double>(order2.quantity);

        return (cost1 + cost2) / total_quantity;
    }

    int generateOrderId()
    {
        static int orderIdCounter = 0;
        return ++orderIdCounter;
    }
};

int main()
{
    boost::asio::io_service ioService;

    // Create and run the client
    Client client(ioService, "localhost", 8080);
    client.run();

    return 0;
}

