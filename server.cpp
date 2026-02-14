#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include "sqlite3.h"

#define SERVER_PORT 4552

int main() {
    sqlite3* db;
    int rc = sqlite3_open("stocks.db", &db); // Opens or creates the DB file

    if (rc) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
        return 1;
    }

    // Temporary line to give $1000 for testing
    //sqlite3_exec(db, "UPDATE Users SET usd_balance = 1000.0 WHERE ID = 1;", 0, 0, 0);

    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // 1. Creating socket file descriptor
    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    // 2. Forcefully attaching socket to the port 4552
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(SERVER_PORT);

    // 3. Bind and Listen
    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, 3);

    std::cout << "Server is listening on port " << SERVER_PORT << "..." << std::endl;


    const char* sqlUsers = "CREATE TABLE IF NOT EXISTS Users ("
                        "ID INTEGER PRIMARY KEY AUTOINCREMENT,"
                        "first_name TEXT,"
                        "last_name TEXT,"
                        "user_name TEXT NOT NULL,"
                        "password TEXT,"
                        "usd_balance DOUBLE NOT NULL);";

    const char* sqlStocks = "CREATE TABLE IF NOT EXISTS Stocks ("
                        "ID INTEGER PRIMARY KEY AUTOINCREMENT,"
                        "stock_symbol VARCHAR(4) NOT NULL UNIQUE," 
                        "stock_name VARCHAR(20),"
                        "stock_balance DOUBLE,"
                        "user_id INTEGER,"
                        "FOREIGN KEY (user_id) REFERENCES Users (ID));";

    // Execute the SQL to create tables
    sqlite3_exec(db, sqlUsers, 0, 0, 0);
    sqlite3_exec(db, sqlStocks, 0, 0, 0);

    const char* sqlInitUser = "INSERT INTO Users (user_name, usd_balance) "
                            " SELECT 'default_user', 100.00 "
                            "WHERE NOT EXISTS (SELECT 1 FROM Users);";
    sqlite3_exec(db, sqlInitUser, 0, 0, 0);

    // 4. Accept connection
    new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
    std::cout << "Connection established!" << std::endl;

    char buffer[1024] = {0};
    int valread;              
    std::string message;      
    std::string response;     

    while (true) {
        memset(buffer, 0, 1024);
        valread = read(new_socket, buffer, 1024); 
        
        if (valread <= 0) {
            std::cout << "Client disconnected." << std::endl;
            break;
        }

        message = buffer; // Remove 'std::string' here
        std::cout << "Received from client: " << message << std::endl; 

        // --- YOUR IF STATEMENTS GO HERE ---
        if (message.find("BALANCE") == 0) {
                sqlite3_stmt* stmt;
                const char* sqlBalance = "SELECT usd_balance FROM Users WHERE ID = 1;";
                
                // Prepare the SQL statement
                if (sqlite3_prepare_v2(db, sqlBalance, -1, &stmt, NULL) == SQLITE_OK) {
                    // Execute the query and check if a row was returned
                    if (sqlite3_step(stmt) == SQLITE_ROW) {
                        double bal = sqlite3_column_double(stmt, 0);
                        response = "200 OK\nBalance: $" + std::to_string(bal) + "\n";
                    } else {
                        response = "404 User Not Found\n";
                    }
                }
                sqlite3_finalize(stmt); // Clean up the statement
            }
        else if (message.find("BUY") == 0) {
            char cmd[10], symbol[10];
            int amount;
            sscanf(message.c_str(), "%s %s %d", cmd, symbol, &amount);

            double stock_price = 10.0; 
            double total_cost = amount * stock_price;

            sqlite3_stmt* stmt;
            const char* sqlCheck = "SELECT usd_balance FROM Users WHERE ID = 1;";
            
            if (sqlite3_prepare_v2(db, sqlCheck, -1, &stmt, NULL) == SQLITE_OK) {
                if (sqlite3_step(stmt) == SQLITE_ROW) {
                    double current_balance = sqlite3_column_double(stmt, 0);
                    
                    if (current_balance >= total_cost) {
                        // 1. Update Balance
                        std::string upUser = "UPDATE Users SET usd_balance = usd_balance - " + std::to_string(total_cost) + " WHERE ID = 1;";
                        sqlite3_exec(db, upUser.c_str(), 0, 0, 0);

                        // 2. Add/Update Stock (FIXED QUOTES HERE)
                        std::string upStock = "INSERT INTO Stocks (stock_symbol, stock_balance, user_id) VALUES ('" + 
                                            std::string(symbol) + "', " + std::to_string(amount) + ", 1) " +
                                            "ON CONFLICT(stock_symbol) DO UPDATE SET stock_balance = stock_balance + " + std::to_string(amount) + ";";
                    } 
                    // Checking if clinet has enough funds
                    else {
                        response = "403 Insufficient Funds\n";
                    }
                }
            }
            sqlite3_finalize(stmt);
        }
        else if (message.find("SELL") == 0) {
            char cmd[10], symbol[10];
            int amount_to_sell;
            sscanf(message.c_str(), "%s %s %d", cmd, symbol, &amount_to_sell);

            // 1. Check if the user owns enough of this stock
            sqlite3_stmt* stmt;
            std::string sqlCheckStock = "SELECT stock_balance FROM Stocks WHERE stock_symbol = '" + std::string(symbol) + "' AND user_id = 1;";
            
            if (sqlite3_prepare_v2(db, sqlCheckStock.c_str(), -1, &stmt, NULL) == SQLITE_OK) {
                if (sqlite3_step(stmt) == SQLITE_ROW) {
                    double current_stock_qty = sqlite3_column_double(stmt, 0);

                    if (current_stock_qty >= amount_to_sell) {
                        // 2. Calculate earnings (assuming fixed price of $10.00)
                        double stock_price = 10.0;
                        double total_earnings = amount_to_sell * stock_price;

                        // 3. Update Balance (Add money)
                        std::string upUser = "UPDATE Users SET usd_balance = usd_balance + " + std::to_string(total_earnings) + " WHERE ID = 1;";
                        sqlite3_exec(db, upUser.c_str(), 0, 0, 0);

                        // 4. Update Stocks (Subtract shares)
                        std::string upStock = "UPDATE Stocks SET stock_balance = stock_balance - " + std::to_string(amount_to_sell) + 
                                            " WHERE stock_symbol = '" + std::string(symbol) + "' AND user_id = 1;";
                        sqlite3_exec(db, upStock.c_str(), 0, 0, 0);

                        response = "200 OK\nSold " + std::to_string(amount_to_sell) + " shares of " + symbol + "\n";
                    } else {
                        response = "403 Insufficient Stock Quantity\n";
                    }
                } else {
                    response = "404 Stock Not Found in Portfolio\n";
                }
            }
            sqlite3_finalize(stmt);
        }
        else if (message.find("LIST") == 0) {
            sqlite3_stmt* stmt;
            const char* sqlList = "SELECT stock_symbol, stock_balance FROM Stocks WHERE user_id = 1;";
            response = "List of stocks:\n";
            
            if (sqlite3_prepare_v2(db, sqlList, -1, &stmt, NULL) == SQLITE_OK) {
                bool found = false;
                while (sqlite3_step(stmt) == SQLITE_ROW) {
                    found = true;
                    std::string sym = (char*)sqlite3_column_text(stmt, 0);
                    int qty = sqlite3_column_int(stmt, 1);
                    response += sym + " | Quantity: " + std::to_string(qty) + "\n";
                }
                if (!found) response = "Portfolio is empty.\n";
            }
            sqlite3_finalize(stmt);
        }
        else if (message.find("QUIT") == 0) {
        // Confirmation for client to exit
            response = "200 OK\n"; 
        }
        else if (message.find("SHUTDOWN") == 0) {
            // Return 200 OK before closing
            response = "200 OK\n";
            send(new_socket, response.c_str(), response.length(), 0);
            
            // Clean up resources 
            close(new_socket);
            sqlite3_close(db);
            std::cout << "Server shutting down..." << std::endl;
            exit(0); // Terminate the server process 
        }
        else {
            response = "Server received: " + message;
        }

        send(new_socket, response.c_str(), response.length(), 0);
    }
    sqlite3_close(db);
    return 0;
}