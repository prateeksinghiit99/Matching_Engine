# Heading Matching Engine

Created a high-performance matching engine utilizing **TCP/IP** socket programming with **Boost.Asio** library and **multithreading** capabilities. This engine excels at connecting to multiple clients, efficiently matching orders using fair logic, and seamlessly handling high-frequency trading clients. It provides a robust and scalable solution for order matching.

## Table of Contents

1. [Requirements & Installations](#1-requirements--installations)
2. [Properties of Model](#2-properties-of-model)
3. [File Usage & Execution Guide](#3-file-usage--execution-guide)
4. [Step-by-Step Testing](#4-step-by-step-testing)
    - [4.1 Testing with Multiple Clients](#41-testing-with-multiple-clients)
    - [4.2 Testing with Automatic Clients](#42-testing-with-automatic-clients)
    - [4.3 Testing with Automatic HFT Clients](#43-testing-with-automatic-hft-clients)
    - [4.4 Testing with Multiple HFT Clients (100-100,000)](#44-testing-with-multiple-hft-clients-100-100000)
5. [Demo Videos](#5-demo-videos)

## 1. Requirements & Installations
1. Updates the package lists for upgrades and new package installations. [Optional]
    ```bash
    sudo apt update
    ```
2. To install g++, run the following command in your terminal:
    ```bash
    sudo apt-get install g++
    ```
3. Install Boost Libraries:
    ```bash
    sudo apt-get install libboost-all-dev
    ```

## 2. Properties of Model
- Non-Blocking: Read and Write.
- Server and Client can handle multiple requests.
- Order can be filled partially.
- Server: Multiple Client Connections Supported.
- Clean & Sleek User Interface.
- Client-Side Message Interpretation: Reducing Server Load.
- Fair & Efficient Logic: FIFO.
- Client: Send & Receive Simultaneously [Parallel Threads for Console I/O].
- Automatic High Frequency Clients to test Server.
- Robust Error Handling Code.

## 3. File Usage & Execution Guide
Follow the instructions below to compile and run the different files included in this repository.
1. **server.cpp** - This will act as our stock exchange.
Compile it using the following command:
```bash
g++ -std=c++17 server.cpp -lboost_system -pthread -o server
```
Run it using: 
```bash
./server
```
2. **manuClient.cpp** - This file acts as our manual/retail trader. Send orders to the exchange. 
Compile it using the following command: 
```bash
g++ -std=c++17 manuClient.cpp -lboost_system -pthread -o manuClient
```
Run it using:
```bash
./manuClient
```
Interaction/Commands:

Send orders in the format 
```bash
<Type>  ['B' or 'S'] <Price> Real <Quantity> Integer`
For example: 
B 6 100 
S 8.8 900
```
3. **autoClient.cpp** - This file acts as our automatic/bot trader. 
Compile it using the following command: 
```bash
g++ -std=c++17 autoClient.cpp -lboost_system -pthread -o autoClient`. 
```
Run it using: 
```bash
./autoClient
```
Interaction/Commands:

To start generating automatic orders, type `start`. To stop generating automatic orders, type `stop`.
    
4. **autoHFTClient.cpp** - This file acts as our automatic high-frequency trader. 
Compile it using the following command:
```bash
g++ -std=c++17 autoHFTClient.cpp -lboost_system -pthread -o autoHFTClient
```
Run it using:
```bash
./autoHFTClient
```
Interaction/Commands:

To start generating automatic orders, type `start`. To stop generating automatic orders, type `stop`.

5. **autoHFTClientGang.cpp** - This is similar to autoHFTClient but it will not wait for your response. It is created to be consumed by run_clients.sh script. 
Compile it using the following command: 
```bash
g++ -std=c++17 autoHFTClientGang.cpp -lboost_system -pthread -o autoHFTClientGang
```
Run it using:
```bash
./autoHFTClientGang
```
To kill it:
```bash
Ctrl + C
```

6. **run_clients.sh** - This is a bash script that takes the number of HFT clients to run as input. It will run ./autoHFTClientGang parallely in background.
Ensure that the bash script file has executable permissions using the command:
```bash
chmod +x run_clients.sh
```
Run it using:
```bash
./run_clients.sh
```
Interaction/Commands: To stop, type `Stop`. 
To kill:
```bash
Ctrl + C
```

7. **serverplus.cpp** - This is an improved version of server.cpp to handle up to 100k requests from a gang of HFT clients, where the previous version failed at 100. 
Compile it using the following command: 
```bash
g++ -std=c++17 serverplus.cpp -lboost_system -pthread -o serverplus
```
Run it using: 
```bash
./serverplus
```
## 4. Step-by-Step Testing
Follow the step-by-step guide to test the Matching Engine with various scenarios. This section provides detailed instructions on how to execute different types of tests.

### 4.1 Testing with Multiple Clients
[Demo-1](https://youtu.be/IkM7BKW0rt0)
1. Run Server first. See section [File Usage & Execution Guide](#3-file-usage--execution-guide)  for comprehensive instructions on how to compile and execute the code.
2. Run manuClient.

**Test Case -1**

S 6 50

S 6 50

S 8 100

B 8 200 

Output - Every order will be matched and order book will be empty

**Test Case -2**

S 6 100

S 8 100

B 8 180

Output - First order matched completely. Second order filled partially [80/100]. Final order filled with average price 6.88.

**Test Case -3**

Run manuClient 2 times to add more manual clients

Client 1- S 6 100

Client 2- S 6 100

Client 3- B 6 100

Output - Buy Order will be matched with Client1

Now 

Client 1- S 6 100

Client 3- B 6 100

Output - Buy order will be matched with Client2

**Test Case -4**

D 5 6

Output - Invalid Order. Only accept 'B' and 'S'

**Test Case- 5**

B 65 7

Output - Price not in range [Price should be between 1.0 - 10.0]

### 4.2 Testing with Automatic Clients
1. Run Server first
2. Run autoClient
3. To start automatic client to generate order: Type **Start** [Refer](#3-file-usage--execution-guide)
4. Add more autoClient
6. Try to stop autoClient: Type **Stop** : If it doesn't work type again
7. Try to punch orders using manuClient.
8. Try to kill autoClient process in between: Type **Ctrl + C**

### 4.3 Testing with Automatic HFT Clients
Explore the testing methodologies specific to high-frequency trading (HFT) clients and analyze the engine's performance under such conditions.
1. Run Server first.
2. Run autoHFTClient
3. To start automatic client to generate order: Type **Start** [Refer](#3-file-usage--execution-guide)
4. Add more autoHFTClient
5. Add autoClient and start it. Our server will accept orders from this client impartially.
6. Add a manuClient.
7. Try to stop autoHTClient: Type **Stop** : If it doesn't work type again

### 4.4 Testing with Multiple HFT Clients (100-100,000)
Get insights into the testing approach for multiple HFT clients, ranging from 100 to 100,000 clients. Evaluate the engine's scalability and efficiency in large-scale scenarios.

**Test -1**
1. Run server.
2. Run run_clients.sh. [Refer](#3-file-usage--execution-guide)
3. Type number of clients. Suggestion[0 - 100][Manually opening and running 100 or even 10 terminals for clients is impractical, but our bash script simplifies the process.]
4. Try to stop it. Type: Stop
5. Run run_clients.sh again.


The server can effortlessly handle up to 10 clients and up to a certain extent with 100 clients. However, this limitation is overcome in the serverplus version.

**Test -2**
1. Run serverplus. [The same testing process can be applied with serverplus, with the only difference being that it does not print the order book every time.]
2. Run run_clients.sh. [Refer](#3-file-usage--execution-guide)
3. Type number of clients. Suggestion[0 - 100,000]
4. Try to stop it. Type: Stop
5. Run run_clients.sh again.

Serverplus can effortlessly handle 10000 and up to a good extent with 100,000 clients.

## 5. Demo Videos
Watch the provided demo videos to visualize the Heading Matching Engine in action. Gain a visual understanding of how the engine performs and interacts with different inputs.

[Demo-1](https://youtu.be/IkM7BKW0rt0)
