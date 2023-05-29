#include <iostream>
#include <boost/asio.hpp>
#include <ctime>
#include <unordered_map>
#include <memory>
#include <queue>

using boost::asio::ip::tcp;
using std::shared_ptr;
using std::make_shared;
using std::enable_shared_from_this;
using std::unordered_map;
using std::priority_queue;
using std::vector;
using std::greater;
using std::greater_equal;
using std::less;
using std::less_equal;
using std::string;
using std::time_t;
using std::time;
using std::size_t;
using std::cout;

struct Order {
    int clientId;
    int orderId;
    char type;
    double price;
    int quantity;
    time_t time;  // Using std::time_t for time representation

    // Implementing the comparison function for the BidOrders priority_queue - MAXHEAP
    bool operator<(const Order& other) const {
        if (price < other.price)
            return true;
        else if (price == other.price)
            return time > other.time;  // Compare by time if prices are equal
        else
            return false;
    }

    // Implementing the comparison function for the AskOrders priority_queue - MINHEAP
    bool operator>(const Order& other) const {
        if (price > other.price)
            return true;
        else if (price == other.price)
            return time > other.time;  // Compare by time if prices are equal
        else
            return false;
    }
};



double lower_limit = 1.0;
double upper_limit = 10.0;
// Creating priority queues for bid (buy) and ask (sell) orders
priority_queue<Order> bidOrders; // Highest bid on top
priority_queue<Order, vector<Order>, greater<Order>> askOrders; // Lowest ask on top


class Connection : public enable_shared_from_this<Connection> {
public:
    explicit Connection(boost::asio::io_service& ioService, unordered_map<int, shared_ptr<Connection>>& connections)
        : socket_(ioService), connections_(connections)
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

	void setClientID(int clientId)
    {
		clientId_ = clientId;
    }

	void asyncWrite()
    {
        auto self(shared_from_this());
        boost::asio::async_write(socket_, boost::asio::buffer(&order_, sizeof(order_)),
            [this, self](const boost::system::error_code& error, size_t /*bytesSent*/) {
                if (!error) {
                    asyncRead(); // Start reading the next order
                } else {
                    connections_.erase(clientId_);
                }
            });
    }

	void asyncWriteToClient(const Order& order)
	{
		int clientId = order.clientId;

		// Check if the client ID exists in the connections map
		if (connections_.count(clientId) == 0) return;

		auto self(shared_from_this());
		boost::asio::io_service& ioService = connections_[clientId]->socket().get_io_service();
		boost::asio::async_write(connections_[clientId]->socket(), boost::asio::buffer(&order, sizeof(order)),
		    [this, self, clientId](const boost::system::error_code& error, size_t /*bytesSent*/) {
		        if (!error) {
		            asyncRead(); // Start reading the next order
		        } else {
		            std::cout << "Write error to client: " << error.message() << std::endl;
		            connections_.erase(clientId);
		        }
		    });
	}

private:
	// Private members
	tcp::socket socket_;
    Order order_;
    unordered_map<int, shared_ptr<Connection>>& connections_;
    int clientId_;

	// Private methods
    void asyncRead()
    {
        auto self(shared_from_this());
        boost::asio::async_read(socket_, boost::asio::buffer(&order_, sizeof(order_)),
            [this, self](const boost::system::error_code& error, size_t /*bytesRead*/) {
                handleRead(error);
            });
    }

	void handleRead(const boost::system::error_code& error)
    {
        if (!error) {
			order_.clientId = clientId_;
			order_.time = time(nullptr);
            cout << "Received order: ClientID: " << order_.clientId << ", OrderId: " << order_.orderId << ", Type: " << order_.type
                      << ", Price: " << order_.price << ", Quantity: " << order_.quantity << "\n";
			
			if(!isvalidOrder()) return;		

			handleOrders();

			//PrintOrderBook();
        } else {
            connections_.erase(clientId_);
        }
    }

	void handleOrders(){

		// Acknowledge to Client that Order is placed
		Order acknowledgeMessage(order_);
		acknowledgeMessage.type = 'A';
        asyncWriteToClient(acknowledgeMessage);

		Order placedOrder = order_;
		if(order_.type == 'B'){
			handleOrdersHelper(askOrders, bidOrders, greater_equal<double>());
		}
		else if(order_.type == 'S'){
			handleOrdersHelper(bidOrders, askOrders, less_equal<double>());
		}

	}
		
	bool isvalidOrder(){
		if(order_.type != 'B' && order_.type != 'S') {
			order_.type = 'X';
			asyncWrite();
			return false;
		}

		if(order_.price > upper_limit ||  order_.price < lower_limit) {
			order_.type = 'O';
			asyncWrite();
			return false;
		}

		return true;
	}
	
	template<typename OppositeBook,typename OwnBook,typename CompareOperator>
	void handleOrdersHelper(OppositeBook &oppositeBook, OwnBook &ownBook, CompareOperator compare){
		Order placedOrder = order_;
		double total_cost = matching(order_, oppositeBook, compare); // Order is now representing remaining order
		placedOrder.quantity -= order_.quantity; // Order.quantity = remaining quanity - This will give placed quantities
		
		// Send Placed Order Details to client
		if(placedOrder.quantity>0){
			
			placedOrder.price = total_cost/(double)placedOrder.quantity;
			asyncWriteToClient(placedOrder);
		}
		
		if(order_.quantity>0)
			ownBook.push(order_);
	}

	template<typename OppositeBook, typename CompareOperator>
	double matching(Order &order, OppositeBook &oppositeBook, CompareOperator compare){
		if(oppositeBook.empty()){
			return 0;
		}

		Order topOrder = oppositeBook.top();

		if(compare(order.price, topOrder.price)){

			double total_cost;			

			if(order.quantity >= topOrder.quantity){ 
				//Buyer received ask price and ask quantities.
				// Total cost of Matched order
				total_cost = topOrder.price * static_cast<double>(topOrder.quantity);

				// Update OrderBook
				oppositeBook.pop();

				// Update Current order
				order.quantity -= topOrder.quantity;
			}
			else{ 
				// Buyer received ask price and bid quantities with no remaining quantities.
				// Total cost of Matched order
				total_cost = topOrder.price * static_cast<double>(order.quantity);

				// Update OrderBook
				Order remainingOrder = topOrder;
				remainingOrder.quantity -= order.quantity;
				oppositeBook.pop();
				oppositeBook.push(remainingOrder);

				// Update the Matched Orders
				topOrder.quantity = order.quantity;
				order.quantity = 0;
			}

			if(topOrder.quantity!=0) /*Bug Fix*/
				asyncWriteToClient(topOrder);
			if(order.quantity!=0)
				total_cost += matching(order, oppositeBook, compare);
			
			return total_cost;
		}
		
		return 0; // If Bid Price is less than ask price
	}
	
	void PrintOrderBook() {
		PrintOrderBookHelper(askOrders, "Top 5 Best Asks");
		PrintOrderBookHelper(bidOrders, "Top 5 Best Bids");
	}
		
	template<typename PriorityQueue>
	void PrintOrderBookHelper(const PriorityQueue& orders, const string& title) {
		PriorityQueue temp = orders;
		
		cout << "--------------------------------\n";
		cout << title << "-----------------\n";
		cout << "Orders\tPrice\tVolume\n";
		
		int limit = 5;
		while (!temp.empty() && limit--) {
		    typename PriorityQueue::value_type tempOrder = temp.top();
		    int orderCount = 0;
		    int volume = 0;
		    
		    while (!temp.empty() && temp.top().price == tempOrder.price) {
		        typename PriorityQueue::value_type tempOrderInner = temp.top();
		        orderCount++;
		        volume += tempOrderInner.quantity;
		        temp.pop();
		    }
		    
		    cout << orderCount << "\t" << tempOrder.price << "\t" << volume << "\n";
		}
	}
};


class Server {
public:
    Server(boost::asio::io_service& ioService, short port)
        : acceptor_(ioService, tcp::endpoint(tcp::v4(), port))
    {
        startAccept();
    }

private:
	// Private members
	tcp::acceptor acceptor_;
    unordered_map<int, shared_ptr<Connection>> connections_;
	
	// Private methods
    void startAccept()
    {
        auto newConnection = make_shared<Connection>(acceptor_.get_io_service(), connections_);
        acceptor_.async_accept(newConnection->socket(),
            [this, newConnection](const boost::system::error_code& error) {
                handleAccept(newConnection, error);
            });
    }

	void handleAccept(shared_ptr<Connection> connection, const boost::system::error_code& error)
	{
		if (!error) {
		    connection->start();

			// Generate a unique client ID and assign it to the new connection
            int clientId = generateClientId();
            connection->setClientID(clientId);
            connections_.emplace(clientId, connection);

		    // Send a welcome message to the new client
			Order welcomeMessage;
			welcomeMessage.clientId = clientId;
			welcomeMessage.type = 'W';
			connection->asyncWriteToClient(welcomeMessage);
		}

		startAccept();
	}

	int generateClientId()
    {
        static int clientIdCounter = 0;
        return ++clientIdCounter;
    }
};

int main()
{
    boost::asio::io_service ioService;

    // Create and run the server on port 8080
    Server server(ioService, 8080);

    // Start the IO service
    ioService.run();

    return 0;
}

