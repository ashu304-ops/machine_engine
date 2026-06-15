#include <iostream>
#include <map>
#include <queue>
#include <string>
#include <algorithm>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <random>
#include <fstream>
#include <unordered_map>
#include <shared_mutex>   // std::shared_mutex, std::shared_lock, std::unique_lock
#include <mutex>
#include <thread>
#include <atomic>
#include <vector>
//g++ -std=c++17 -O2 -pthread -o orderbook_mt orderbook_mt.cpp
using namespace std;

// ── Data structures ──────────────────────────────────────────────────────────

struct orders {
    int    id;
    int    quantity;
    string timestamp;
};

struct OrderInfo {
    bool   isBuy;
    int    price;
    int    quantity;
    string status; // "OPEN", "PARTIAL", "FILLED", "CANCELLED"
};

// ── Order Book ───────────────────────────────────────────────────────────────

class orderbook {
private:
    map<int, queue<orders>, greater<int>> buybook;
    map<int, queue<orders>>               sellbook;
    unordered_map<int, OrderInfo>         orderLookup;

    // Single readers-writer lock:
    //   shared_lock  → read-only methods (print*, lookup*)
    //   unique_lock  → all mutating methods (add*, cancel*, modify*)
    mutable shared_mutex bookMutex;

    // ── Internal (lock-free) helpers ─────────────────────────────────────────
    // Called only when the caller already holds a unique_lock.

    string _getTimeNow() const {
        using namespace chrono;
        auto    now   = system_clock::now();
        time_t  now_t = system_clock::to_time_t(now);
        tm     *ltm   = localtime(&now_t);
        stringstream ss;
        ss << put_time(ltm, "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }

    // Returns nullptr if id not found. Caller must hold at least a shared lock.
    const OrderInfo* _lookupOrder_nolock(int id) const {
        auto it = orderLookup.find(id);
        if (it == orderLookup.end()) return nullptr;
        return &it->second;
    }

    // Cancel without acquiring any lock. Used internally by modify_order.
    bool _cancel_order_nolock(int id) {
        bool found = false;

        for (auto& [price, q] : buybook) {
            queue<orders> tmp;
            while (!q.empty()) {
                orders ord = q.front(); q.pop();
                if (ord.id != id) tmp.push(ord);
                else              found = true;
            }
            q = tmp;
        }
        for (auto& [price, q] : sellbook) {
            queue<orders> tmp;
            while (!q.empty()) {
                orders ord = q.front(); q.pop();
                if (ord.id != id) tmp.push(ord);
                else              found = true;
            }
            q = tmp;
        }

        if (found) {
            orderLookup[id].status   = "CANCELLED";
            orderLookup[id].quantity = 0;
        }
        return found;
    }

    // Limit-buy matching (no lock acquired; caller holds unique_lock).
    void _addbuy_nolock(int id, int price, int quantity) {
        cout << "BUY ORDER: ID=" << id
             << " PRICE=" << price
             << " QTY="   << quantity << "\n";

        orderLookup[id] = { true, price, quantity, "OPEN" };
        int originalQty = quantity;

        while (quantity > 0 && !sellbook.empty()) {
            auto bestsell = sellbook.begin();
            if (price < bestsell->first) break;

            while (quantity > 0 && !bestsell->second.empty()) {
                orders& sellorder = bestsell->second.front();
                int traded = min(quantity, sellorder.quantity);

                cout << "TRADE: " << traded
                     << " @ "    << bestsell->first
                     << " | BUY_ID="  << id
                     << " | SELL_ID=" << sellorder.id
                     << " | SELL_TS=" << sellorder.timestamp << "\n";

                quantity                           -= traded;
                sellorder.quantity                 -= traded;
                orderLookup[id].quantity           -= traded;
                orderLookup[sellorder.id].quantity -= traded;

                orderLookup[sellorder.id].status =
                    (orderLookup[sellorder.id].quantity == 0) ? "FILLED" : "PARTIAL";

                if (sellorder.quantity == 0)
                    bestsell->second.pop();
            }
            if (bestsell->second.empty())
                sellbook.erase(bestsell);
        }

        if (quantity == 0) {
            orderLookup[id].status = "FILLED";
        } else {
            orderLookup[id].status   = (quantity < originalQty) ? "PARTIAL" : "OPEN";
            orderLookup[id].quantity = quantity;
            buybook[price].push({ id, quantity, _getTimeNow() });
        }
    }

    // Limit-sell matching (no lock acquired; caller holds unique_lock).
    void _addsell_nolock(int id, int price, int quantity) {
        cout << "SELL ORDER: ID=" << id
             << " PRICE=" << price
             << " QTY="   << quantity << "\n";

        orderLookup[id] = { false, price, quantity, "OPEN" };
        int originalQty = quantity;

        while (quantity > 0 && !buybook.empty()) {
            auto bestbuy = buybook.begin();
            if (price > bestbuy->first) break;

            while (quantity > 0 && !bestbuy->second.empty()) {
                orders& buyorder = bestbuy->second.front();
                int traded = min(quantity, buyorder.quantity);

                cout << "TRADE: " << traded
                     << " @ "    << bestbuy->first
                     << " | SELL_ID=" << id
                     << " | BUY_ID="  << buyorder.id
                     << " | BUY_TS="  << buyorder.timestamp << "\n";

                quantity                          -= traded;
                buyorder.quantity                 -= traded;
                orderLookup[id].quantity          -= traded;
                orderLookup[buyorder.id].quantity -= traded;

                orderLookup[buyorder.id].status =
                    (orderLookup[buyorder.id].quantity == 0) ? "FILLED" : "PARTIAL";

                if (buyorder.quantity == 0)
                    bestbuy->second.pop();
            }
            if (bestbuy->second.empty())
                buybook.erase(bestbuy);
        }

        if (quantity == 0) {
            orderLookup[id].status = "FILLED";
        } else {
            orderLookup[id].status   = (quantity < originalQty) ? "PARTIAL" : "OPEN";
            orderLookup[id].quantity = quantity;
            sellbook[price].push({ id, quantity, _getTimeNow() });
        }
    }

public:
    // ── Thread-safe public API ───────────────────────────────────────────────

    // Lookup (read-only): shared lock → concurrent reads allowed.
    const OrderInfo* lookupOrder(int id) const {
        shared_lock<shared_mutex> lk(bookMutex);
        return _lookupOrder_nolock(id);
    }

    void printLookup(int id) const {
        shared_lock<shared_mutex> lk(bookMutex);
        const OrderInfo* info = _lookupOrder_nolock(id);
        if (!info) {
            cout << "LOOKUP: ID=" << id << " NOT FOUND\n";
            return;
        }
        cout << "LOOKUP: ID="  << id
             << " SIDE="       << (info->isBuy ? "BUY" : "SELL")
             << " PRICE="      << info->price
             << " QTY="        << info->quantity
             << " STATUS="     << info->status << "\n";
    }

    void printAllLookup() const {
        shared_lock<shared_mutex> lk(bookMutex);
        cout << "\n===== ORDER LOOKUP TABLE =====\n";
        if (orderLookup.empty()) { cout << "(empty)\n"; }
        for (auto& [id, info] : orderLookup) {
            cout << " ID="     << id
                 << " SIDE="   << (info.isBuy ? "BUY" : "SELL")
                 << " PRICE="  << info.price
                 << " QTY="    << info.quantity
                 << " STATUS=" << info.status << "\n";
        }
        cout << "==============================\n";
    }

    void printBestBidAsk() const {
        shared_lock<shared_mutex> lk(bookMutex);
        cout << "\n===== BEST BID / ASK =====\n";
        if (!buybook.empty())
            cout << "Best Bid (Buy) : " << buybook.begin()->first  << "\n";
        else
            cout << "Best Bid (Buy) : NONE\n";

        if (!sellbook.empty())
            cout << "Best Ask (Sell): " << sellbook.begin()->first << "\n";
        else
            cout << "Best Ask (Sell): NONE\n";
        cout << "==========================\n";
    }

    void printbook() const {
        shared_lock<shared_mutex> lk(bookMutex);
        cout << "\n-------------------------------------\n";
        cout << "ORDER BOOK\n";
        cout << "-------------------------------------\n";

        cout << "== SELL BOOK (ascending price) ==\n";
        for (auto& [price, q] : sellbook) {
            queue<orders> tmp = q;
            int vol = 0;
            while (!tmp.empty()) { vol += tmp.front().quantity; tmp.pop(); }
            cout << "  Price: " << price << " | Volume: " << vol << "\n";
            tmp = q;
            while (!tmp.empty()) {
                cout << "    ID=" << tmp.front().id
                     << " QTY="  << tmp.front().quantity
                     << " TS="   << tmp.front().timestamp << "\n";
                tmp.pop();
            }
        }

        cout << "== BUY BOOK (descending price) ==\n";
        for (auto& [price, q] : buybook) {
            queue<orders> tmp = q;
            int vol = 0;
            while (!tmp.empty()) { vol += tmp.front().quantity; tmp.pop(); }
            cout << "  Price: " << price << " | Volume: " << vol << "\n";
            tmp = q;
            while (!tmp.empty()) {
                cout << "    ID=" << tmp.front().id
                     << " QTY="  << tmp.front().quantity
                     << " TS="   << tmp.front().timestamp << "\n";
                tmp.pop();
            }
        }
    }

    // ── Mutating operations: exclusive lock ──────────────────────────────────

    void addbuy(int id, int price, int quantity) {
        unique_lock<shared_mutex> lk(bookMutex);
        _addbuy_nolock(id, price, quantity);
    }

    void addsell(int id, int price, int quantity) {
        unique_lock<shared_mutex> lk(bookMutex);
        _addsell_nolock(id, price, quantity);
    }

    void addmarketbuy(int id, int quantity) {
        unique_lock<shared_mutex> lk(bookMutex);
        string market_ts = _getTimeNow();
        cout << "\nMARKET BUY: ID=" << id
             << " QTY=" << quantity
             << " TS="  << market_ts << "\n";

        orderLookup[id] = { true, 0, quantity, "OPEN" };
        int originalQty = quantity;

        while (quantity > 0 && !sellbook.empty()) {
            auto bestsell = sellbook.begin();

            while (quantity > 0 && !bestsell->second.empty()) {
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

                orderLookup[sellorder.id].status =
                    (orderLookup[sellorder.id].quantity == 0) ? "FILLED" : "PARTIAL";

                if (sellorder.quantity == 0)
                    bestsell->second.pop();
            }
            if (bestsell->second.empty()) {
                sellbook.erase(bestsell);
                continue;
            }
        }

        if (quantity == 0)
            orderLookup[id].status = "FILLED";
        else {
            orderLookup[id].status = (quantity < originalQty) ? "PARTIAL" : "OPEN";
            cout << "UNFILLED MARKET BUY QTY=" << quantity
                 << " TS=" << market_ts << "\n";
        }
    }

    void addmarketsell(int id, int quantity) {
        unique_lock<shared_mutex> lk(bookMutex);
        string market_ts = _getTimeNow();
        cout << "\nMARKET SELL: ID=" << id
             << " QTY=" << quantity
             << " TS="  << market_ts << "\n";

        orderLookup[id] = { false, 0, quantity, "OPEN" };
        int originalQty = quantity;

        while (quantity > 0 && !buybook.empty()) {
            auto bestbuy = buybook.begin();

            while (quantity > 0 && !bestbuy->second.empty()) {
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

                orderLookup[buyorder.id].status =
                    (orderLookup[buyorder.id].quantity == 0) ? "FILLED" : "PARTIAL";

                if (buyorder.quantity == 0)
                    bestbuy->second.pop();
            }
            if (bestbuy->second.empty()) {
                buybook.erase(bestbuy);
                continue;
            }
        }

        if (quantity == 0)
            orderLookup[id].status = "FILLED";
        else {
            orderLookup[id].status = (quantity < originalQty) ? "PARTIAL" : "OPEN";
            cout << "UNFILLED MARKET SELL QTY=" << quantity
                 << " TS=" << market_ts << "\n";
        }
    }

    void cancel_order(int id) {
        unique_lock<shared_mutex> lk(bookMutex);
        cout << "\nCANCEL ORDER ID=" << id << "\n";
        bool found = _cancel_order_nolock(id);
        if (found)
            cout << "ORDER CANCELLED: ID=" << id << "\n";
        else
            cout << "CANCEL FAILED: ID=" << id << " not found in book\n";
    }

    void modify_order(int id, int new_price, int new_quantity) {
        unique_lock<shared_mutex> lk(bookMutex);
        cout << "\nMODIFY ORDER: ID=" << id
             << " NEW_PRICE=" << new_price
             << " NEW_QTY="   << new_quantity << "\n";

        const OrderInfo* info = _lookupOrder_nolock(id);
        if (!info) {
            cout << "MODIFY FAILED: ID=" << id << " NOT FOUND\n";
            return;
        }
        if (info->status == "FILLED" || info->status == "CANCELLED") {
            cout << "MODIFY FAILED: ID=" << id << " is already " << info->status << "\n";
            return;
        }

        bool isBuy = info->isBuy;
        _cancel_order_nolock(id);   // resets status to CANCELLED

        // Re-insert with same ID (lock already held)
        if (isBuy) _addbuy_nolock (id, new_price, new_quantity);
        else        _addsell_nolock(id, new_price, new_quantity);

        cout << "ORDER MODIFIED SUCCESSFULLY: ID=" << id << "\n";
    }
};

// ── Atomic ID generator ──────────────────────────────────────────────────────
// Shared across threads so each order gets a unique ID.

static atomic<int> nextOrderId{ 1 };

inline int newId() { return nextOrderId.fetch_add(1, memory_order_relaxed); }

// ── CSV loader ───────────────────────────────────────────────────────────────

void loadOrders(orderbook& ob, const string& filename) {
    ifstream file(filename);
    if (!file.is_open()) { cout << "Failed to open " << filename << "\n"; return; }
    string line;
    getline(file, line); // skip header
    while (getline(file, line)) {
        stringstream ss(line);
        string idStr, side, priceStr, qtyStr;
        getline(ss, idStr,    ',');
        getline(ss, side,     ',');
        getline(ss, priceStr, ',');
        getline(ss, qtyStr,   ',');
        int id = stoi(idStr), price = stoi(priceStr), qty = stoi(qtyStr);
        if      (side == "BUY")  ob.addbuy (id, price, qty);
        else if (side == "SELL") ob.addsell(id, price, qty);
    }
}

// ── Main: multithreaded demo ─────────────────────────────────────────────────

int main() {
    orderbook ob;

    // ── Mutex to keep cout output from interleaving ──────────────────────────
    // (the order book itself is already protected; this is just for legible output)
    mutex coutMtx;

    // ── Thread worker: randomly buys or sells ────────────────────────────────
    auto worker = [&](int threadId, int ordersPerThread) {
        mt19937 rng(threadId * 12345ULL + time(nullptr));
        uniform_int_distribution<int> sideDist (0,   1);
        uniform_int_distribution<int> priceDist(90, 110);
        uniform_int_distribution<int> qtyDist  (1,  100);

        for (int i = 0; i < ordersPerThread; ++i) {
            int id    = newId();
            int price = priceDist(rng);
            int qty   = qtyDist(rng);

            if (sideDist(rng) == 0) ob.addbuy (id, price, qty);
            else                    ob.addsell(id, price, qty);
        }
    };

    // ── Launch N threads, each placing M orders ───────────────────────────────
    const int NUM_THREADS        = 4;
    const int ORDERS_PER_THREAD  = 10;

    cout << "=== Spawning " << NUM_THREADS << " threads ("
         << ORDERS_PER_THREAD << " orders each) ===\n\n";

    vector<thread> threads;
    threads.reserve(NUM_THREADS);
    for (int t = 0; t < NUM_THREADS; ++t)
        threads.emplace_back(worker, t, ORDERS_PER_THREAD);

    for (auto& th : threads) th.join();

    cout << "\n=== All threads finished ===\n";

    // ── Sequential demo ops after threads complete ────────────────────────────

    // Stable order for modify/cancel demos
    int safeId = newId();
    ob.addbuy(safeId, 10, 100);    // far below market → won't match
    ob.printLookup(safeId);
    ob.modify_order(safeId, 95, 150);
    ob.printLookup(safeId);

    int cancelId = newId();
    ob.addsell(cancelId, 999, 50); // far above market → won't match
    ob.cancel_order(cancelId);
    ob.printLookup(cancelId);

    int mbId = newId(), msId = newId();
    ob.addmarketbuy (mbId,  50);
    ob.addmarketsell(msId,  50);

    ob.printBestBidAsk();
    ob.printbook();
    ob.printAllLookup();

    return 0;
}
