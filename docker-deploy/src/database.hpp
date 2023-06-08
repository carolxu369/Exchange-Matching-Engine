#ifndef _DATABASE_H_
#define _DATABASE_H_

#include <iostream>
#include <pqxx/pqxx>
#include <string>
#include <fstream>
#include <iomanip>
#include <cstdlib>
#include "socket.hpp"
#include "pugixml/pugixml.hpp"
using namespace std;
using namespace pqxx;

#define DOCKER 1

/* 
    @func:
    1. _openOrder: store info of open order in transaction form
    2. _finishOrder: store info of executed or cancel order in deal form
*/
struct _openOrder {
    long id;
    string status;
    double share;//amount
};
struct _finishOrder {
    long id;
    string status;
    double share;//amount
    double price;
    long time;
};
/*
    @func: store all infomation and function about database
    @note: status for all order are{"OPEN", "CANCEL", "EXECUTED"}
    @private variable: 
        1.   tables = table name
        2.   createFile = the sql file which include the code to create a tables in postgres
    @public function:
        1.  addOrder: once user post buy or sell, use this function, matching inside
        2.  queryOrderInTrans + queryOrderInDeal = querry function
        3.  cancelOrder: cancel this order and put this order into deal

    Database(){}
    ~Database();
    void init();
    // table operation: add to table 
    int addSymbol(string name);
    int addAccount(int id, int balance);
    int addPosition(string name, int account_id, int amount);
    long addTransaction(string name, int id, double amount, double limit_price);
    void addDeal(string name, int one_id, int another_id, int one_amount, int deal_price, long time);
    // main function
    long addOrder(string name, int id, double amount, double limit_price);
    _openOrder queryOrderInTrans(long order_id);
    // _finishOrder* queryOrderInDeal(long order_id);
    _finishOrder* queryOrderInDeal(long order_id, pugi::xml_node & results);
    void cancelOrder(int id);
    // for test
    void showAllBuy();
    void showAllSell();
};
int isAlphanumeric(std::string str);
long getCurrTime();
#endif
*/
class Database{
private:
    connection *C;
    vector<string> tables = {"SYMBOL", "ACCOUNT", "POSITION", "TRANSACTION","DEAL"};
    string createFile = "createTB.sql";
    // execute sql and return the order_id if needed
    int exec(string sql);
    // drop existed table
    void drop();
    // create table
    void create();
    // helper function for addDeal function 
    void addDealHelper(string name, int order_id, int amount, int deal_price, long time);
    // find the id of most matching order
    int findMatchOrder(string name, int amount, int price);
    // update the amount and status of transaction 
    void updateTransaction(int order_id, double amount, string status);
    // find the matching order and then update tables of transaction and 
    void matchingOrder(long order_id_one, string symbol_name, long account_id, double amount, int price);
    // update the balance in account
    long updateBalance(long account_id, double newBalance);
public:
    Database(){}
    ~Database();
    void init();
    // table operation: add to table 
    // check if symbols exists
    int hasSymbol(string name);
    // check if symbols exists
    int hasAccount(int account_id);
    // reduce the position when selling order
    long sellOrder(string name, int account_id, double amount, double limit_price);
    // reduce the balance when buying order
    long buyOrder(string name, int account_id, double amount, double limit_price);
    int addSymbol(string name);
    int addAccount(int id, int balance);
    int addPosition(string name, int account_id, int amount);
    long addTransaction(string name, int id, double amount, double limit_price);
    void addDeal(string name, int one_id, int another_id, int one_amount, int deal_price, long time);
    // main function
    long addOrder(string name, int id, double amount, double limit_price);
    int hasOrder(long order_id, int input_account_id);
    _openOrder queryOrderInTrans(long order_id);
    _finishOrder* queryOrderInDeal(long order_id, pugi::xml_node & results);
    int cancelOrder(int order_id, int input_account_id);
    // for test
    void showAllBuy();
    void showAllSell();
};
int isAlphanumeric(std::string str);
long getCurrTime();
#endif