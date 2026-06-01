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
using namespace std;

struct orders{
    int id;
    int quantity;
    string timestamp;
};

class orderbook{
    private:
        map<int,queue<orders>,greater<int> > buybook;
        map<int,queue<orders> > sellbook;

    public:
        string getTimeNow(){
        using namespace chrono;

        auto now = system_clock::now();
        time_t now_time = system_clock::to_time_t(now);

        tm *ltm = std::localtime(&now_time);

        stringstream ss;
        ss << put_time(ltm, "%Y-%m-%d %H:%M:%S");

        return ss.str();
    }

        void addbuy(int id,int price,int quantity){
        cout << "BUY ORDER: "
            << "ID=" << id
            << " PRICE=" << price
            << " QTY=" << quantity
            << endl;
        //match against sell order
        while(quantity>0&&!sellbook.empty()){
            auto bestsell=sellbook.begin();
            if(price< bestsell->first){
                break;
            }
            while(quantity>0&&!bestsell->second.empty()){
                orders& sellorder=bestsell->second.front();
                int traded=min(quantity,sellorder.quantity);
                cout << "TRADE: "
                    << traded
                    << " @ " << bestsell->first
                    << " | SELL_TS=" << sellorder.timestamp
                    << endl;
                quantity-=traded;
                sellorder.quantity-=traded;
                if(sellorder.quantity==0){
                    bestsell->second.pop();
                }
            }
            if(bestsell->second.empty()){
                        sellbook.erase(bestsell);

        }
    }
        //remaining to book
                if(quantity>0){
                    buybook[price].push({id, quantity, getTimeNow()});
                }


        }
        void addsell(int id,int price,int quantity){
            cout << "SELL ORDER: "
                    << "ID=" << id
                    << " PRICE=" << price
                    << " QTY=" << quantity
                    << endl;
            //buy price >=sell price in this case price is seller reject case price<buy price
            while(quantity>0&&!buybook.empty()){
                auto buy=buybook.begin();
                
                if(price > buy->first){
                    break;
                }
                while(quantity>0&&!buy->second.empty()){
                    orders& buyorder=buy->second.front();
                    int traded=min(quantity,buyorder.quantity);
                    cout << "TRADE: "
                        << traded
                        << " @ " << buy->first
                        << " | BUY_TS=" << buyorder.timestamp
                        << endl;
                    quantity-=traded;
                    buyorder.quantity-=traded;
                    
                    if(buyorder.quantity==0){
                        buy->second.pop();
                    }
                    
                    }
                    if(buy->second.empty()){
                        buybook.erase(buy);
                    }
            }
            if(quantity>0){
                sellbook[price].push({id, quantity, getTimeNow()});
            }
        }
        void printbook(){
            cout<<"-------------------------------------"<<endl;
            cout<<"order book"<<endl;
            cout<<"-------------------------------------"<<endl;

            cout<<"Sell book"<<endl;
            for(auto &p:sellbook){
                cout<<"price"<<" "<<p.first;
                queue<orders>q=p.second;
                
                int totalVolume=0;
                queue<orders>temp=q;
                while(!temp.empty()){
                    totalVolume+=temp.front().quantity;
                    temp.pop();
                }
                cout<<"Price:"<<p.first
                    <<"Volume:"<<totalVolume;

                while(!q.empty()){
                    cout<<"quantity"<<q.front().quantity
                        <<"TS= "<<q.front().timestamp;
                    q.pop();
                }
                cout<<endl;
            }
            cout<<"Buy book"<<endl;
            for(auto &p:buybook){
                cout<<"price"<<" "<<p.first;
                queue<orders>q=p.second;

                int totalVolume=0;
                queue<orders>temp=q;
                while(!temp.empty()){
                    totalVolume+=temp.front().quantity;
                    temp.pop();
                }
                cout<<"Price:"<<p.first
                    <<"Volume:"<<totalVolume;

                while(!q.empty()){
                    cout<<"qauntity"<<q.front().quantity
                        <<"TS= "<<q.front().timestamp<<endl;

                    q.pop();
                }
                cout<<endl;
            }
            
        }
        void cancel_order(int id){
            cout << "\nCANCEL ORDER ID=" << id << endl;
            //looking on buybook side
            for(auto &p:buybook){
                queue<orders>q;
                while(!p.second.empty()){
                    orders ord=p.second.front();
                    p.second.pop();

                    if(ord.id!=id){
                        q.push(ord);
                    }else{
                        cout << "ORDER CANCELLED: ID=" << ord.id << endl;
                    }
                }
                p.second=q;

            }
            //looking on sellbook
            for(auto &p:sellbook){
                queue<orders>q;
                while(!p.second.empty()){
                    orders ord=p.second.front();
                    p.second.pop();

                    if(ord.id!=id){
                        q.push(ord);
                    }else{
                        cout << "ORDER CANCELLED: ID=" << ord.id << endl;
                    }
                }
                p.second=q;
            }
        }
        void addmarketbuy(int id,int quantity){
    string market_ts = getTimeNow();

    cout << "\nMARKET BUY: "
         << "ID=" << id
         << " QTY=" << quantity
         << " TS=" << market_ts
         << endl;

    while(quantity>0 && !sellbook.empty()){
        auto bestsell=sellbook.begin();

        while(quantity>0 && !bestsell->second.empty()){
            orders &sellorder=bestsell->second.front();

            int traded=min(quantity,sellorder.quantity);

            cout << "TRADE: "
                 << traded
                 << " @ " << bestsell->first
                 << " | MARKET_BUY_TS=" << market_ts
                 << " | SELL_TS=" << sellorder.timestamp
                 << endl;

            quantity-=traded;
            sellorder.quantity-=traded;

            if(sellorder.quantity==0){
                bestsell->second.pop();
            }
        }

        if(bestsell->second.empty()){
            sellbook.erase(bestsell);
            continue;
        }
    }

    if(quantity>0){
        cout << "UNFILLED MARKET BUY QTY="
             << quantity
             << " TS=" << market_ts
             << endl;
    }
}
        void addmarketsell(int id,int quantity){
    string market_ts = getTimeNow();

    cout << "\nMARKET SELL: "
         << "ID=" << id
         << " QTY=" << quantity
         << " TS=" << market_ts
         << endl;

    while(quantity>0 && !buybook.empty()){
        auto bestbuy=buybook.begin();

        while(quantity>0 && !bestbuy->second.empty()){
            orders &buyorder=bestbuy->second.front();

            int traded=min(quantity,buyorder.quantity);

            cout << "TRADE: "
                 << traded
                 << " @ " << bestbuy->first
                 << " | MARKET_SELL_TS=" << market_ts
                 << " | BUY_TS=" << buyorder.timestamp
                 << endl;

            quantity-=traded;
            buyorder.quantity-=traded;

            if(buyorder.quantity==0){
                bestbuy->second.pop();
            }
        }

        if(bestbuy->second.empty()){
            buybook.erase(bestbuy);
            continue;
        }
    }

    if(quantity>0){
        cout << "UNFILLED MARKET SELL QTY="
             << quantity
             << " TS=" << market_ts
             << endl;
    }
}
        
};

#include <random>

int main() {

    orderbook ob;

    mt19937 rng(time(nullptr));

    uniform_int_distribution<int> sideDist(0, 1);      // 0=buy, 1=sell
    uniform_int_distribution<int> priceDist(90, 110);  // prices
    uniform_int_distribution<int> qtyDist(1, 100);     // quantities

    const int N = 100;

    auto start = chrono::high_resolution_clock::now();

    for (int i = 1; i <= N; i++) {

        int side = sideDist(rng);
        int price = priceDist(rng);
        int qty = qtyDist(rng);

        if (side == 0) {
            ob.addbuy(i, price, qty);
        } else {
            ob.addsell(i, price, qty);
        }
    }

    auto end = chrono::high_resolution_clock::now();

    auto duration =
        chrono::duration_cast<chrono::milliseconds>(end - start);

    cout << "\n====================================\n";
    cout << "Processed " << N << " orders\n";
    cout << "Time: " << duration.count() << " ms\n";
    cout << "Orders/sec: "
         << (N * 1000.0 / duration.count())
         << "\n";
    cout << "====================================\n";

    ob.printbook();

    return 0;
}
