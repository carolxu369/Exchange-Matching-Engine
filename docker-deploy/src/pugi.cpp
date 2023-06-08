#include "pugi.hpp"

// declare the lock
std::mutex db_lock;

// if find create call create
std::string pugi_create(pugi::xml_document & doc, Database & DB){
    std::lock_guard<std::mutex> lock(db_lock);

    pugi::xml_node create = doc.child("create");
    pugi::xml_document response;
    pugi::xml_node results = response.append_child("results");

    // iterate all accounts
    for (pugi::xml_node curr_acc = create.child("account"); curr_acc; curr_acc = curr_acc.next_sibling("account")){
        int acc_id = curr_acc.attribute("id").as_int();
        int acc_bal = curr_acc.attribute("balance").as_int();

        // check the database, whether the account already existed
        // exist, error
        if (DB.hasAccount(acc_id) == 1){
            pugi::xml_node error_acc = results.append_child("error");
            error_acc.append_attribute("id") = acc_id;
            error_acc.text() = "Account Already Exists";
        }
        // not exist
        else{
            DB.addAccount(acc_id, acc_bal);

            pugi::xml_node created = results.append_child("created");
            created.append_attribute("id") = acc_id;
        }
    }

    for (pugi::xml_node curr_sym = create.child("symbol"); curr_sym; curr_sym = curr_sym.next_sibling("symbol")){
        std::string sym = curr_sym.attribute("sym").as_string();

        // add the symbol to the database if not exits
        DB.addSymbol(sym);

        for (pugi::xml_node sym_acc = curr_sym.child("account"); sym_acc; sym_acc = sym_acc.next_sibling("account")){
            int curr_id = sym_acc.attribute("id").as_int();
            int curr_num = sym_acc.text().as_int();

            // respond error if the account doesn't exist
            if (DB.hasAccount(curr_id) == 0){
                pugi::xml_node curr_err = results.append_child("error");
                curr_err.append_attribute("sym") = sym.c_str();
                curr_err.append_attribute("id") = curr_id;
                curr_err.text() = "Account Doesn't Exist for Symbol";
            }
            // exists
            else{
                DB.addPosition(sym, curr_id, curr_num);

                pugi::xml_node created = results.append_child("created");
                created.append_attribute("sym") = sym.c_str();
                created.append_attribute("id") = curr_id;
            }
        }
    }

    std::stringstream strstream;
    response.save(strstream, "\t", pugi::format_default);
    //std::cout << strstream.str() << std::endl;

    return strstream.str();
}

std::string pugi_transaction(pugi::xml_document & doc, Database & DB){
    std::lock_guard<std::mutex> lock(db_lock);

    pugi::xml_node tran = doc.child("transactions");
    int tran_acc = tran.attribute("account").as_int();
    pugi::xml_document response;
    pugi::xml_node results = response.append_child("results");

    // order
    for (pugi::xml_node order = tran.child("order"); order; order = order.next_sibling("order")){
        std::string sym = order.attribute("sym").as_string();
        int amt = order.attribute("amount").as_int();
        float limit = order.attribute("limit").as_float();

        // check account existance
        if (DB.hasAccount(tran_acc) == 0){
            pugi::xml_node error_acc = results.append_child("error");
            error_acc.append_attribute("sym") = sym.c_str();
            error_acc.append_attribute("amount") = amt;
            error_acc.append_attribute("limit") = limit;
            error_acc.text() = "Order Failed: Account Doesn't Exist";

            continue;
        }

        // if no symbol, error
        if (DB.hasSymbol(sym) == 0){
            pugi::xml_node error_acc = results.append_child("error");
            error_acc.append_attribute("sym") = sym.c_str();
            error_acc.append_attribute("amount") = amt;
            error_acc.append_attribute("limit") = limit;
            error_acc.text() = "Symbol Doesn't Exist";
        }
        else {
            // if buy, check account balance, reject if not enough balance
            if (amt > 0){
                long order_id = DB.addOrder(sym, tran_acc, amt, limit);
                if (order_id > 0){
                    pugi::xml_node open = results.append_child("opened");
                    open.append_attribute("sym") = sym.c_str();
                    open.append_attribute("amount") = amt;
                    open.append_attribute("limit") = limit;
                    open.append_attribute("id") = order_id;
                }
                else{
                    pugi::xml_node error_acc = results.append_child("error");
                    error_acc.append_attribute("sym") = sym.c_str();
                    error_acc.append_attribute("amount") = amt;
                    error_acc.append_attribute("limit") = limit;
                    error_acc.text() = "Request Rejected: Insufficient Buyer Balance";
                }
            }
            // if sell, check account amount, reject if not enough amount
            else {
                long order_id = DB.addOrder(sym, tran_acc, amt, limit);
                if (order_id > 0){
                    pugi::xml_node open = results.append_child("opened");
                    open.append_attribute("sym") = sym.c_str();
                    open.append_attribute("amount") = amt;
                    open.append_attribute("limit") = limit;
                    open.append_attribute("id") = order_id;
                }
                else{
                    pugi::xml_node error_acc = results.append_child("error");
                    error_acc.append_attribute("sym") = sym.c_str();
                    error_acc.append_attribute("amount") = amt;
                    error_acc.append_attribute("limit") = limit;
                    error_acc.text() = "Request Rejected: Insufficient Seller Amount";
                }
            }
        }
    }

    // query
    for (pugi::xml_node query = tran.child("query"); query; query = query.next_sibling("query")){
        int tran_id = query.attribute("id").as_int();

        // check account existance
        if (DB.hasAccount(tran_acc) == 0){
            pugi::xml_node error_acc = results.append_child("error");
            error_acc.append_attribute("id") = tran_id;
            error_acc.text() = "Query Failed: Account Doesn't Exist";

            continue;
        }

        pugi::xml_node status = results.append_child("status");
        status.append_attribute("id") = tran_id;
        // if transaction id not exists
        if (DB.hasOrder(tran_id, tran_acc) == -1){
            pugi::xml_node error_acc = status.append_child("error");
            error_acc.text() = "Error: Transaction ID Doesn't Exist";
        }
        else{
            // check whether the tran_id matches the account_id
            if (DB.hasOrder(tran_id, tran_acc) == 0){
                pugi::xml_node error_acc = status.append_child("error");
                error_acc.text() = "Error: Transaction ID Doesn't Match With Account ID";
            }
            else{
                _openOrder open_order;
                open_order = DB.queryOrderInTrans(tran_id);
                if (open_order.id != 0){
                    pugi::xml_node open = status.append_child("open");
                    open.append_attribute("shares") = open_order.share;
                }
                // the deal
                _finishOrder * finish_order = DB.queryOrderInDeal(tran_id, status);
            }
        }
    }

    // cancel
    for (pugi::xml_node cancel = tran.child("cancel"); cancel; cancel = cancel.next_sibling("cancel")){
        int tran_id = cancel.attribute("id").as_int();

        // check account existance
        if (DB.hasAccount(tran_acc) == 0){
            pugi::xml_node error_acc = results.append_child("error");
            error_acc.append_attribute("id") = tran_id;
            error_acc.text() = "Cancel Failed: Account Doesn't Exist";

            continue;
        }

        pugi::xml_node canceled = results.append_child("canceled");
        canceled.append_attribute("id") = tran_id;
        // if transaction id not exists
        if (DB.hasOrder(tran_id, tran_acc) == -1){
            pugi::xml_node error_acc = canceled.append_child("error");
            error_acc.text() = "Error: Transaction ID Doesn't Exist";
        }
        else{
            // check whether the tran_id matches the account_id
            if (DB.hasOrder(tran_id, tran_acc) == 0){
                pugi::xml_node error_acc = canceled.append_child("error");
                error_acc.text() = "Error: Transaction ID Doesn't Match With Account ID";
            }
            else{
                // cancel order
                DB.cancelOrder(tran_id, tran_acc);
                _finishOrder * finish_order = DB.queryOrderInDeal(tran_id, canceled);
            }
        }
    }

    std::stringstream strstream;
    response.save(strstream, "\t", pugi::format_default);
    //std::cout << strstream.str() << std::endl;

    return strstream.str();
}
