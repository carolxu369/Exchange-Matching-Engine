#include "database.hpp"
/*
    @func: destructure function for database
*/
Database::~Database(){
    C->disconnect();
    if (C){
        delete C;
    }
}
/*
    @func: establish a connection to the database
    @param: DOCKER = 1 means using docker, or use local database
*/
void Database::init(){
    //TODO: if docker, conncection should be change.
    #if DOCKER
    C = new connection("dbname=testdb user=postgres password=passw0rd host=db port=5432");
    #else
    C = new connection("dbname=testdb user=postgres password=passw0rd");
    #endif 
    if (C->is_open()) {
        drop();
        create();
    } else {
        cout << "Can't open database" << endl;
    }
}
/*
    @func: execute a query 
    @param: sql is SQL command line
    @return: if order_id existed, then return 0
*/
int Database::exec(string sql){
    work W(*C);
    // W.exec(sql);
    long order_id = 0; 
    try{
        result R(W.exec(sql));
        if (!R.empty()){
            order_id = R.begin()[0].as<long>();
        } 
        W.commit(); 
    } catch(const exception &e){
        cerr<<e.what()<<endl;
        W.abort();
    }    
    return order_id;
}
/*
    @func: drop tables that may already exist
    @param: "vector<string>& tables" table's name
*/
void Database::drop(){
  for (int i=0;i<tables.size();i++){
    string sql = "DROP TABLE IF EXISTS " + tables[i] + " CASCADE;";
    exec(sql);
  }
}
/*
    @func: create table according to the defined file
    @param: file'name to open
*/
void Database::create(){
  string sql, line;
  ifstream my_file(createFile.c_str());
  if (my_file.is_open()){
    while(getline(my_file,line)){
      sql += line;
    }
    my_file.close();
    exec(sql);
  }else{
    throw(ErrMsg("Cannot open create table file"));
  }
}
/*
    @func: check the symbol exsited or not
    @return: 0 is not existed, 1 is existed 
*/
int Database::hasSymbol(string name){
    string sql = "SELECT * FROM SYMBOL WHERE SYMBOL_NAME ="+C->quote(name)+";";
    nontransaction N(*C);
    result R(N.exec(sql));
    if (!R.empty()) return 1;
    return 0;
}
/*
    @func: check the account exsited or not
    @return: 0 is not existed, 1 is existed 
*/
int Database::hasAccount(int account_id){
    string sql = "SELECT * FROM ACCOUNT WHERE ACCOUNT_ID ="+C->quote(account_id)+";";
    nontransaction N(*C);
    result R(N.exec(sql));
    if (!R.empty()) return 1;
    return 0;
}
/*
    @func: add a symbol if not existed, or skip
    @return: 0 means symbol existed, 1 means create success
*/
int Database::addSymbol(string name){
    try{
        if (hasSymbol(name)==1){
            // cout<<name<<": The symbol exsisted.\n";
            return 0;
        }
        if (isAlphanumeric(name)==0){
            // cout<<name<<": symbol format is not correct.\n";
            return 0;
        }
        string sql_real = "INSERT INTO SYMBOL (SYMBOL_NAME) VALUES (" + C->quote(name) + ");";
        exec(sql_real); 
    }catch (const std::exception &e) {
        cerr << e.what() << std::endl;
    } 
    return 1;
}
/*
    @func: add an account if not existed, or skip 
    @return: 0 means account existed, 1 means create success
*/
int Database::addAccount(int id, int balance){
    if (hasAccount(id)==1){
        // cout<<id<<": The account id exsisted.\n";
        return 0;
    }
    string sql_real = "INSERT INTO ACCOUNT (ACCOUNT_ID, BALANCE) VALUES (" + C->quote(id) +","+C->quote(balance) + ");";
    exec(sql_real); 
    return 1;
}
/*
    @func: add an position if not existed, or update the position(add current amount to old amount)
    @return: 0 means no symbol or no account, 1 means create success
*/
int Database::addPosition(string name, int account_id, int amount){
    int update = 0;
    int amountOld = 0;
    if (hasSymbol(name)==0 || hasAccount(account_id)==0){
        // cout<<"No symbol or account existed.\n";
        return 0;
    }
    string sql = "SELECT * FROM POSITION WHERE ACCOUNT_ID ="+C->quote(account_id)+" AND "+"SYMBOL_NAME ="+C->quote(name)+";";
    nontransaction N(*C);
    result R(N.exec(sql));
    if (!R.empty()){
        amountOld = R.begin()["AMOUNT"].as<int>();
        update = 1;
    }
    N.commit();
    if (update==0){
        sql = "INSERT INTO POSITION (SYMBOL_NAME, ACCOUNT_ID, AMOUNT) VALUES (" + C->quote(name) +","+C->quote(account_id)+","+C->quote(amount)+");";        
    }else{
        sql = "UPDATE POSITION SET AMOUNT = " + C->quote(amount+amountOld) +" WHERE ACCOUNT_ID ="+C->quote(account_id)+" AND "+"SYMBOL_NAME ="+C->quote(name)+";"; 
    }
    exec(sql); 
    return 1;
}
/*
    @func: add a transaction, if the account or symbol not existed, then skip
    @note: a negative amount means to sell, positive amount means to buy
    @return: positive number returned is the order id, 0 means error
*/
long Database::addTransaction(string name, int id, double amount, double limit_price){
    long order_id = 0;
    try{
        if (hasSymbol(name)==0 || hasAccount(id)==0){
            return order_id;
        }
        stringstream sql;
        sql << "INSERT INTO TRANSACTION ( SYMBOL_NAME, ACCOUNT_ID, AMOUNT, LIMIT_PRICE) VALUES (";
        sql << C->quote(name) <<","<<C->quote(id) <<","<< C->quote(amount) <<","<< C->quote(limit_price) << ") RETURNING ORDER_ID;";
        order_id = exec(sql.str());   
    }catch (const std::exception &e){
        cerr << e.what() << std::endl;
    }
    return order_id;
}
/*
    @func: buy or sell, add a transaction, if the account or symbol not existed, then skip
    @return: positive number returned is the order id, 0 means error
    @note: 
        1. sellOrder: check the share in the account, change the share amount, post selling order
        2. buyOrder: checking the balance in the account, change the balance, post buying order
*/
long Database::sellOrder(string name, int account_id, double amount, double limit_price){
    string sql = "SELECT * FROM POSITION WHERE ACCOUNT_ID ="+C->quote(account_id)+" AND SYMBOL_NAME = "+C->quote(name)+";";
    nontransaction N(*C);
    result R(N.exec(sql));
    double owning = R.begin()["AMOUNT"].as<double>();
    if (owning <= (-amount)){
        // cout<<"lack position in account"<<endl;
        // throw(ErrMsg("lack remaining shares position in account"));
        return 0;
    } 
    N.commit();
    sql = "UPDATE POSITION SET AMOUNT = " + C->quote(owning + amount) +" WHERE ACCOUNT_ID ="+C->quote(account_id)+" AND "+"SYMBOL_NAME ="+C->quote(name)+";";
    exec(sql);
    return addTransaction(name, account_id, amount, limit_price);
}
long Database::buyOrder(string name, int account_id, double amount, double limit_price){
    long update = updateBalance(account_id,- amount * limit_price);
    if (update != 0){
        return addTransaction(name, account_id, amount, limit_price);
    }
    return 0;
}
/*
    @func: add data into DEAL table
*/
void Database::addDealHelper(string name, int order_id, int amount, int deal_price, long time){
    int deal_id = 0;
    stringstream sql;
    sql << "INSERT INTO DEAL ( ORDER_ID, SYMBOL_NAME, AMOUNT, EXECUTE_PRICE, UPDATE_TIME) VALUES (";
    sql << C->quote(order_id) <<","<<C->quote(name) <<","<< C->quote(amount) <<",";
    sql << C->quote(deal_price) <<","<<C->quote(time) << ") RETURNING DEAL_ID;";
    deal_id = exec(sql.str());   
    string sql_str = "SELECT * FROM TRANSACTION WHERE ORDER_ID = "+C->quote(order_id)+" ;";
    nontransaction N(*C);
    result R(N.exec(sql_str));
    int account_id = R.begin()["ACCOUNT_ID"].as<int>();
    N.commit();
    if (amount>0){//buy
        addPosition(name,account_id,amount);
    }else{//sell
        updateBalance(account_id,- amount * deal_price); 
    }
}
/*
    @func: add data into deal by sell side and buy side
    @param: one_id and another_id are all order_id
*/
void Database::addDeal(string name, int one_id, int another_id, int one_amount, int deal_price, long time){
    addDealHelper(name, one_id, one_amount, deal_price, time);
    addDealHelper(name, another_id, -one_amount, deal_price, time);
}
/*
    @func: match with all satisfied order in transaction and choose the best one
*/
int Database::findMatchOrder(string name, int amount, int price){
    long order_id = 0;
    string sql;
    if (amount>0){          //buy
        sql = "SELECT * FROM TRANSACTION WHERE AMOUNT < "+ C->quote(0)+" AND LIMIT_PRICE <= "+ C->quote(price)+" AND SYMBOL_NAME = "+ C->quote(name)+" AND STATUS = "+ C->quote("OPEN")+" ORDER BY LIMIT_PRICE ASC, ORDER_ID ASC;";
    }else if (amount<0){    //sell
        sql = "SELECT * FROM TRANSACTION WHERE AMOUNT > "+ C->quote(0)+" AND LIMIT_PRICE >= "+ C->quote(price)+" AND SYMBOL_NAME = "+ C->quote(name)+" AND STATUS = "+ C->quote("OPEN")+" ORDER BY LIMIT_PRICE DESC, ORDER_ID ASC;";
    }
    nontransaction N(*C);
    result R(N.exec(sql));
    if (!R.empty()){
        order_id = R.begin()["ORDER_ID"].as<double>();
    }
    return order_id;
}
/*
    @func: update the amount and status of a order with a specific order_id in transaction form
*/
void Database::updateTransaction(int order_id, double amount, string status){
    try{
        string sql = "UPDATE TRANSACTION SET AMOUNT = "+C->quote(amount)+" , STATUS = "+ C->quote(status)+" WHERE ORDER_ID = "+C->quote(order_id)+" ;";
        exec(sql);
    }catch (const std::exception &e) {
        cerr << e.what() << std::endl;
    }
}
/*
    @func: find the best matched order_id
    @param: 
        1. order_id_one: order_id of this order
        2. order_id_another: orde_id of matched order
*/
void Database::matchingOrder(long order_id_one,string symbol_name,long account_id,double amount,int price_this_one){
    while (amount!=0){
        int order_id_another = findMatchOrder(symbol_name, amount, price_this_one);
        if (order_id_another==0){
            break;
        }
        string sql = "SELECT * FROM TRANSACTION WHERE ORDER_ID = "+C->quote(order_id_another)+" ;";
        nontransaction N(*C);
        result R(N.exec(sql));
        double amount_another_order = R.begin()["AMOUNT"].as<double>();
        double price_another_order = R.begin()["LIMIT_PRICE"].as<double>();
        N.commit();
        double execute_price = (order_id_another>order_id_one)?price_this_one:price_another_order;
        //  b-1. add order_one to deal
        //  b-2. add order_another to deal
        long time = getCurrTime();
        double execute_amount;
        if ((abs(amount) < abs(amount_another_order))){
            //not use up all another order
            execute_amount = amount;
            updateTransaction(order_id_another, amount_another_order+amount, "OPEN");
        }else{
            // use up all another order
            execute_amount = -amount_another_order;
            updateTransaction(order_id_another, 0, "EXECUTED");
        }
        addDeal(symbol_name, order_id_one, order_id_another, execute_amount, execute_price, time);
        // check refund(different in price and buyer)
        if ((execute_price != price_this_one)&&(amount>0)){
            updateBalance(account_id, execute_amount * (price_this_one-execute_price));
        }
        // update amount
        amount = (abs(amount)>abs(amount_another_order))?(amount+amount_another_order):0;
    }
    if (amount==0){
        //  a-1. change status of this order in transaction 
        updateTransaction(order_id_one, amount, "EXECUTED");
    } else {
        //  a-2. or change the amount of this order in transaction   
        updateTransaction(order_id_one, amount, "OPEN");            
    }    
    return;
}

/*
    @func: add order info into transaction, then matching best order and return the order_id of 
*/
long Database::addOrder(string name, int account_id, double amount, double limit_price){
    double order_id = 0;
    if (amount>0){ 
        order_id = buyOrder(name,account_id,amount,limit_price);
    }else if(amount<0){
        order_id = sellOrder(name,account_id,amount,limit_price);
    }
    matchingOrder(order_id,name,account_id,amount,limit_price);
    return order_id;
}
/*
    @func: cancel the transaction, making the status to be cancelled.
    @return: if the order is not canceled by its owner, then return 0;
             return 1 if the order is cancelled successfully.
*/
int Database::cancelOrder(int order_id, int input_account_id){
    string sql = "SELECT * FROM TRANSACTION WHERE ORDER_ID ="+C->quote(order_id)+" AND STATUS = 'OPEN' ;";
    nontransaction N(*C);
    result R(N.exec(sql));
    if (R.empty()){
        return 0;
    }
    string symbol_name = R.begin()["SYMBOL_NAME"].as<string>();
    double amount = R.begin()["AMOUNT"].as<double>();
    double limit_price = R.begin()["LIMIT_PRICE"].as<double>();
    double account_id = R.begin()["ACCOUNT_ID"].as<double>();
    N.commit();
    // verify the auth
    if (account_id != input_account_id){
        return 0;
    }
    if (amount>0){ 
        // buy: Canceling a Buy order refunds the purchase price to the buyer’s account.
        updateBalance(account_id,amount * limit_price);
    } else if (amount<0){
        // sell: Canceling a Sell order returns the shares to the seller’s account.
        addPosition(symbol_name,account_id,-amount);
    }
    double amount_in_transaction;
    double time = getCurrTime();
    string status = "CANCEL";
    string sql_real = "UPDATE TRANSACTION SET STATUS = " + C->quote(status) +" WHERE ORDER_ID ="+C->quote(order_id)+";";
    exec(sql_real); 
    sql_real="INSERT INTO DEAL ( ORDER_ID, SYMBOL_NAME, AMOUNT, UPDATE_TIME, STATUS) VALUES ("+C->quote(order_id) +" ,"+C->quote(symbol_name)+" ,"+C->quote(amount)+" ,"+ C->quote(time)+" ,"+ C->quote(status)+" );";
    exec(sql_real);
    return 1; 
}
/*
    @func:  update balance in an account and then add the newBalance to it
    @note:  make sure the balance existed
            if balance lower than 0, it will throw an exception
*/
long Database::updateBalance(long account_id, double newBalance){
    string sql1 = "SELECT * FROM ACCOUNT WHERE ACCOUNT_ID ="+C->quote(account_id)+";";
    nontransaction N1(*C);
    result R1(N1.exec(sql1));
    double balance = R1.begin()["BALANCE"].as<double>();
    if (balance + newBalance < 0){
        // cout << "cannot buy a order due to BALANCE"<<endl;
        // throw(ErrMsg("cannot buy a order due to BALANCE"));
        return 0;
    }
    N1.commit();
    string sql = "UPDATE ACCOUNT SET BALANCE = " + C->quote(balance + newBalance) +" WHERE ACCOUNT_ID ="+C->quote(account_id)+";";
    exec(sql);
    return 1;
}

/*
    @func: check whether the transaction_id exists or not
    @return: -1 if not exists
             0 if not match with the account
             1 otherwise
*/
int Database::hasOrder(long order_id, int input_account_id){
    string sql = "SELECT * FROM TRANSACTION WHERE ORDER_ID = " + C->quote(order_id) + ";";
    exec(sql);
    nontransaction N(*C);
    result R(N.exec(sql));
    if (R.empty()){
        return -1;
    }
    else{
        double account_id = R.begin()["ACCOUNT_ID"].as<double>();
        if (account_id != input_account_id){
            return 0;
        }
    }

    return 1;
}

/*
    @func: query the order and return a structure
    @return: if the status is error, then cannot find the order
*/
_openOrder Database::queryOrderInTrans(long order_id){
    string sql = "SELECT * FROM TRANSACTION WHERE ORDER_ID = " + C->quote(order_id) + " AND STATUS = 'OPEN' ;";
    exec(sql);
    nontransaction N(*C);
    result R(N.exec(sql));
    _openOrder openOrder;
    if (R.empty()){
        openOrder.id = 0;
        return openOrder;
    }
    openOrder.id = order_id;
    openOrder.status = R.begin()["STATUS"].as<string>();
    openOrder.share = R.begin()["AMOUNT"].as<int>();
    // cout<<order_id<<":<open shares="<<openOrder.share<<"/>\n";
    return openOrder;
}
/*
    @func: querry order in deal table
    @return: return structure array, return size = array size + 1(ending)
    @note: need to delete[] in main function
*/
_finishOrder* Database::queryOrderInDeal(long order_id, pugi::xml_node & results){
    string sql = "SELECT * FROM DEAL WHERE ORDER_ID = " + C->quote(order_id) + " ;";
    nontransaction N(*C);
    result R(N.exec(sql));
    int numRecord = R.size();
    _finishOrder* finishOrder = new _finishOrder[numRecord+1];
    for (int i=0;i<numRecord;i++){
        finishOrder[i].id = order_id;
        finishOrder[i].status = R[i]["STATUS"].as<string>();    
        finishOrder[i].share = R[i]["AMOUNT"].as<int>();
        finishOrder[i].price = R[i]["EXECUTE_PRICE"].as<double>();
        finishOrder[i].time = R[i]["UPDATE_TIME"].as<double>();
        if (finishOrder[i].status=="CANCEL"){
            pugi::xml_node canceled = results.append_child("canceled");
            canceled.append_attribute("shares") = finishOrder[i].share;
            canceled.append_attribute("time") = finishOrder[i].time;
            // cout<<order_id<<":<canceled shares="<<finishOrder[i].share<<",time="<<finishOrder[i].time<<"/>\n";
        }else{
            pugi::xml_node executed = results.append_child("executed");
            executed.append_attribute("shares") = finishOrder[i].share;
            executed.append_attribute("price") = finishOrder[i].price;
            executed.append_attribute("time") = finishOrder[i].time;
            //cout<<order_id<<":<executed shares="<<finishOrder[i].share<<",price="<<finishOrder[i].price<<",time="<<finishOrder[i].time<<"/>\n";
        }
    }
    finishOrder[numRecord]={0,"",0,0,0};
    return finishOrder;
}
/*
    @func: only for test to show the buy or sell results
*/
void Database::showAllBuy(){
    string sql = "SELECT * FROM TRANSACTION WHERE AMOUNT > "+ C->quote(0)+" ORDER BY LIMIT_PRICE DESC;";
    nontransaction N(*C);
    result R(N.exec(sql));
    // cout <<"id amount price\n";
    for(result::const_iterator row = R.begin(); row != R.end(); ++row){
        // cout<<row["ORDER_ID"].as<int>()<<" "
         //   <<row["AMOUNT"].as<int>()<<" "
          //  <<row["LIMIT_PRICE"].as<int>()<<"\n";
    }
}
void Database::showAllSell(){
    string sql = "SELECT * FROM TRANSACTION WHERE AMOUNT < "+ C->quote(0)+" ORDER BY LIMIT_PRICE DESC;";
    nontransaction N(*C);
    result R(N.exec(sql));
    // cout <<"id amount price\n";
    for(result::const_iterator row = R.begin(); row != R.end(); ++row){
        // cout<<row["ORDER_ID"].as<int>()<<" "
           // <<row["AMOUNT"].as<int>()<<" "
            // <<row["LIMIT_PRICE"].as<int>()<<"\n";
    }
}
/*
    @func: check if the symbol only contains the alphabet or numer 
    @return: return 1 if format is correct, return 0 is format is wrong
*/
int isAlphanumeric(std::string str){
    for (string::iterator it = str.begin(); it != str.end(); it++){
        if (isdigit(*it)||isalpha(*it)){
            return 1;
        }
    }
    return 0;
}
/*
    @func: get current time in long format 
*/
long getCurrTime(){
    time_t now = time(nullptr);
    stringstream ss;
    ss << now;
    long time;
    ss >> time;
    return time;
}