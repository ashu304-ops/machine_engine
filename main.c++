#include<iostream>
#include<map>
#include<queue>
#include<string>
#include<cmath>
#include<algorithm>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include<random>
#include<fstream>
#include <random>
#include <unordered_map>
using namespace std;

struct orders{
    int id;
    int quantity;
    string timestamp;
};

struct OrderInfo {
    bool isBuy;
    int price;
    int quantity;
    string status; // "OPEN", "PARTIAL", "FILLED", "CANCELLED"
};

class orderbook{
    private:
        map<int,queue<orders>,greater<int>> buybook;
        map<int,queue<orders>>              sellbook;
        unordered_map<int, OrderInfo>       orderLookup;

    public:
        // ── Utilities ────────────────────────────────────────────────────────
        string getTimeNow(){
            using namespace chrono;
            auto now      = system_clock::now();
            time_t now_t  = system_clock::to_time_t(now);
            tm *ltm       = localtime(&now_t);
            stringstream ss;
            ss << put_time(ltm, "%Y-%m-%d %H:%M:%S");
            return ss.str();
        }

        // ── Lookup helpers ───────────────────────────────────────────────────

        // Returns nullptr if id not found
        const OrderInfo* lookupOrder(int id) const {
            auto it = orderLookup.find(id);
            if(it == orderLookup.end()) return nullptr;
            return &it->second;
        }

        void printLookup(int id) const {
            const OrderInfo* info = lookupOrder(id);
            if(!info){
                cout << "LOOKUP: ID=" << id << " NOT FOUND\n";
                return;
            }
            cout << "LOOKUP: ID=" << id
                 << " SIDE="     << (info->isBuy ? "BUY" : "SELL")
                 << " PRICE="    << info->price
                 << " QTY="      << info->quantity
                 << " STATUS="   << info->status
                 << "\n";
        }

        void printAllLookup() const {
            cout << "\n===== ORDER LOOKUP TABLE =====\n";
            if(orderLookup.empty()){
                cout << "(empty)\n";
            }
            for(auto& [id, info] : orderLookup){
                cout << " ID="     << id
                     << " SIDE="   << (info.isBuy ? "BUY" : "SELL")
                     << " PRICE="  << info.price
                     << " QTY="    << info.quantity
                     << " STATUS=" << info.status
                     << "\n";
            }
            cout << "==============================\n";
        }

        // ── Best bid / ask ───────────────────────────────────────────────────
        void printBestBidAsk() const {
            cout << "\n===== BEST BID / ASK =====\n";
            if(!buybook.empty())
                cout << "Best Bid (Buy) : " << buybook.begin()->first  << "\n";
            else
                cout << "Best Bid (Buy) : NONE\n";

            if(!sellbook.empty())
                cout << "Best Ask (Sell): " << sellbook.begin()->first << "\n";
            else
                cout << "Best Ask (Sell): NONE\n";
            cout << "==========================\n";
        }

        // ── Limit buy ────────────────────────────────────────────────────────
        void addbuy(int id, int price, int quantity){
            cout << "BUY ORDER: ID=" << id
                 << " PRICE=" << price
                 << " QTY="   << quantity << "\n";

            // Register in lookup immediately (full qty, OPEN)
            orderLookup[id] = { true, price, quantity, "OPEN" };
            int originalQty = quantity;

            while(quantity > 0 && !sellbook.empty()){
                auto bestsell = sellbook.begin();
                if(price < bestsell->first) break;

                while(quantity > 0 && !bestsell->second.empty()){
                    orders& sellorder = bestsell->second.front();
                    int traded = min(quantity, sellorder.quantity);

                    cout << "TRADE: " << traded
                         << " @ "    << bestsell->first
                         << " | BUY_ID="  << id
                         << " | SELL_ID=" << sellorder.id
                         << " | SELL_TS=" << sellorder.timestamp << "\n";

                    // Update lookup for both sides
                    quantity                          -= traded;
                    sellorder.quantity                -= traded;
                    orderLookup[id].quantity          -= traded;
                    orderLookup[sellorder.id].quantity -= traded;

                    if(orderLookup[sellorder.id].quantity == 0)
                        orderLookup[sellorder.id].status = "FILLED";
                    else
                        orderLookup[sellorder.id].status = "PARTIAL";

                    if(sellorder.quantity == 0)
                        bestsell->second.pop();
                }
                if(bestsell->second.empty())
                    sellbook.erase(bestsell);
            }

            // Finalize buyer's status
            if(quantity == 0){
                orderLookup[id].status = "FILLED";
            } else {
                orderLookup[id].status = (quantity < originalQty) ? "PARTIAL" : "OPEN";
                orderLookup[id].quantity = quantity; // remaining
                buybook[price].push({id, quantity, getTimeNow()});
            }
        }

        // ── Limit sell ───────────────────────────────────────────────────────
        void addsell(int id, int price, int quantity){
            cout << "SELL ORDER: ID=" << id
                 << " PRICE=" << price
                 << " QTY="   << quantity << "\n";

            // Register in lookup immediately (full qty, OPEN)
            orderLookup[id] = { false, price, quantity, "OPEN" };
            int originalQty = quantity;

            while(quantity > 0 && !buybook.empty()){
                auto bestbuy = buybook.begin();
                if(price > bestbuy->first) break;

                while(quantity > 0 && !bestbuy->second.empty()){
                    orders& buyorder = bestbuy->second.front();
                    int traded = min(quantity, buyorder.quantity);

                    cout << "TRADE: " << traded
                         << " @ "    << bestbuy->first
                         << " | SELL_ID=" << id
                         << " | BUY_ID="  << buyorder.id
                         << " | BUY_TS="  << buyorder.timestamp << "\n";

                    // Update lookup for both sides
                    quantity                         -= traded;
                    buyorder.quantity                -= traded;
                    orderLookup[id].quantity         -= traded;
                    orderLookup[buyorder.id].quantity -= traded;

                    if(orderLookup[buyorder.id].quantity == 0)
                        orderLookup[buyorder.id].status = "FILLED";
                    else
                        orderLookup[buyorder.id].status = "PARTIAL";

                    if(buyorder.quantity == 0)
                        bestbuy->second.pop();
                }
                if(bestbuy->second.empty())
                    buybook.erase(bestbuy);
            }

            // Finalize seller's status
            if(quantity == 0){
                orderLookup[id].status = "FILLED";
            } else {
                orderLookup[id].status = (quantity < originalQty) ? "PARTIAL" : "OPEN";
                orderLookup[id].quantity = quantity; // remaining
                sellbook[price].push({id, quantity, getTimeNow()});
            }
        }

        // ── Market buy ───────────────────────────────────────────────────────
        void addmarketbuy(int id, int quantity){
            string market_ts = getTimeNow();
            cout << "\nMARKET BUY: ID=" << id
                 << " QTY=" << quantity
                 << " TS="  << market_ts << "\n";

            orderLookup[id] = { true, 0, quantity, "OPEN" };
            int originalQty = quantity;

            while(quantity > 0 && !sellbook.empty()){
                auto bestsell = sellbook.begin();

                while(quantity > 0 && !bestsell->second.empty()){
                    orders& sellorder = bestsell->second.front();
                    int traded = min(quantity, sellorder.quantity);

                    cout << "TRADE: " << traded
                         << " @ "    << bestsell->first
                         << " | MARKET_BUY_TS="  << market_ts
                         << " | SELL_ID="         << sellorder.id
                         << " | SELL_TS="         << sellorder.timestamp << "\n";

                    quantity                           -= traded;
                    sellorder.quantity                 -= traded;
                    orderLookup[id].quantity           -= traded;
                    orderLookup[sellorder.id].quantity -= traded;

                    if(orderLookup[sellorder.id].quantity == 0)
                        orderLookup[sellorder.id].status = "FILLED";
                    else
                        orderLookup[sellorder.id].status = "PARTIAL";

                    if(sellorder.quantity == 0)
                        bestsell->second.pop();
                }
                if(bestsell->second.empty()){
                    sellbook.erase(bestsell);
                    continue;
                }
            }

            if(quantity == 0)
                orderLookup[id].status = "FILLED";
            else {
                orderLookup[id].status = (quantity < originalQty) ? "PARTIAL" : "OPEN";
                cout << "UNFILLED MARKET BUY QTY=" << quantity
                     << " TS=" << market_ts << "\n";
            }
        }

        // ── Market sell ──────────────────────────────────────────────────────
        void addmarketsell(int id, int quantity){
            string market_ts = getTimeNow();
            cout << "\nMARKET SELL: ID=" << id
                 << " QTY=" << quantity
                 << " TS="  << market_ts << "\n";

            orderLookup[id] = { false, 0, quantity, "OPEN" };
            int originalQty = quantity;

            while(quantity > 0 && !buybook.empty()){
                auto bestbuy = buybook.begin();

                while(quantity > 0 && !bestbuy->second.empty()){
                    orders& buyorder = bestbuy->second.front();
                    int traded = min(quantity, buyorder.quantity);

                    cout << "TRADE: " << traded
                         << " @ "    << bestbuy->first
                         << " | MARKET_SELL_TS=" << market_ts
                         << " | BUY_ID="          << buyorder.id
                         << " | BUY_TS="          << buyorder.timestamp << "\n";

                    quantity                          -= traded;
                    buyorder.quantity                 -= traded;
                    orderLookup[id].quantity          -= traded;
                    orderLookup[buyorder.id].quantity -= traded;

                    if(orderLookup[buyorder.id].quantity == 0)
                        orderLookup[buyorder.id].status = "FILLED";
                    else
                        orderLookup[buyorder.id].status = "PARTIAL";

                    if(buyorder.quantity == 0)
                        bestbuy->second.pop();
                }
                if(bestbuy->second.empty()){
                    buybook.erase(bestbuy);
                    continue;
                }
            }

            if(quantity == 0)
                orderLookup[id].status = "FILLED";
            else {
                orderLookup[id].status = (quantity < originalQty) ? "PARTIAL" : "OPEN";
                cout << "UNFILLED MARKET SELL QTY=" << quantity
                     << " TS=" << market_ts << "\n";
            }
        }

        // ── Cancel ───────────────────────────────────────────────────────────
        void cancel_order(int id){
            cout << "\nCANCEL ORDER ID=" << id << "\n";

            bool found = false;

            for(auto& [price, q] : buybook){
                queue<orders> tmp;
                while(!q.empty()){
                    orders ord = q.front(); q.pop();
                    if(ord.id != id) tmp.push(ord);
                    else             found = true;
                }
                q = tmp;
            }
            for(auto& [price, q] : sellbook){
                queue<orders> tmp;
                while(!q.empty()){
                    orders ord = q.front(); q.pop();
                    if(ord.id != id) tmp.push(ord);
                    else             found = true;
                }
                q = tmp;
            }

            if(found){
                orderLookup[id].status   = "CANCELLED";
                orderLookup[id].quantity = 0;
                cout << "ORDER CANCELLED: ID=" << id << "\n";
            } else {
                cout << "CANCEL FAILED: ID=" << id << " not found in book\n";
            }
        }

        // ── Print book ───────────────────────────────────────────────────────
        void printbook() const {
            cout << "\n-------------------------------------\n";
            cout << "ORDER BOOK\n";
            cout << "-------------------------------------\n";

            cout << "== SELL BOOK (ascending price) ==\n";
            for(auto& [price, q] : sellbook){
                queue<orders> tmp = q;
                int totalVol = 0;
                while(!tmp.empty()){ totalVol += tmp.front().quantity; tmp.pop(); }

                cout << "  Price: " << price << " | Volume: " << totalVol << "\n";
                tmp = q;
                while(!tmp.empty()){
                    cout << "    ID=" << tmp.front().id
                         << " QTY="  << tmp.front().quantity
                         << " TS="   << tmp.front().timestamp << "\n";
                    tmp.pop();
                }
            }

            cout << "== BUY BOOK (descending price) ==\n";
            for(auto& [price, q] : buybook){
                queue<orders> tmp = q;
                int totalVol = 0;
                while(!tmp.empty()){ totalVol += tmp.front().quantity; tmp.pop(); }

                cout << "  Price: " << price << " | Volume: " << totalVol << "\n";
                tmp = q;
                while(!tmp.empty()){
                    cout << "    ID=" << tmp.front().id
                         << " QTY="  << tmp.front().quantity
                         << " TS="   << tmp.front().timestamp << "\n";
                    tmp.pop();
                }
            }
        }
};

// ── CSV loader ───────────────────────────────────────────────────────────────
void loadOrders(orderbook& ob, const string& filename){
    ifstream file(filename);
    if(!file.is_open()){
        cout << "Failed to open " << filename << "\n";
        return;
    }
    string line;
    getline(file, line); // skip header
    while(getline(file, line)){
        stringstream ss(line);
        string idStr, side, priceStr, qtyStr;
        getline(ss, idStr,   ',');
        getline(ss, side,    ',');
        getline(ss, priceStr,',');
        getline(ss, qtyStr,  ',');
        int id = stoi(idStr), price = stoi(priceStr), qty = stoi(qtyStr);
        if(side == "BUY")  ob.addbuy (id, price, qty);
        else if(side == "SELL") ob.addsell(id, price, qty);
    }
    file.close();
}

// ── Main ─────────────────────────────────────────────────────────────────────
int main(){
    orderbook ob;

    mt19937 rng(time(nullptr));
    uniform_int_distribution<int> sideDist (0,  1);
    uniform_int_distribution<int> priceDist(90, 110);
    uniform_int_distribution<int> qtyDist  (1,  100);

    const int N = 20; // small for readable demo

    for(int i = 1; i <= N; i++){
        int side  = sideDist(rng);
        int price = priceDist(rng);
        int qty   = qtyDist(rng);
        if(side == 0) ob.addbuy (i, price, qty);
        else          ob.addsell(i, price, qty);
    }

    // Demo: lookup a few specific orders
    ob.printLookup(1);
    ob.printLookup(2);

    // Demo: cancel an order and verify lookup reflects it
    ob.cancel_order(3);
    ob.printLookup(3);

    // Demo: market orders
    ob.addmarketbuy (N+1, 50);
    ob.addmarketsell(N+2, 50);

    ob.printBestBidAsk();
    ob.printbook();
    ob.printAllLookup();

    return 0;
}
//order
