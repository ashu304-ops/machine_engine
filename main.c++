#include<iostream>
#include<map>
#include<queue>
#include<string>
#include<cmath>
#include<algorithm>
using namespace std;

struct orders{
    int id;
    int quantity;
};

class orderbook{
    private:
        map<int,queue<orders>,greater<int> > buybook;
        map<int,queue<orders> > sellbook;

    public:
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
                        << " @ "
                        << bestsell->first
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
                    buybook[price].push({id,quantity});
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
                        << " @ "
                        << buy->first
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
                sellbook[price].push({id,quantity});
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
                while(!q.empty()){
                    cout<<"quantity"<<q.front().quantity<<" ";
                    q.pop();
                }
                cout<<endl;
            }
            cout<<"Buy book"<<endl;
            for(auto &p:buybook){
                cout<<"price"<<" "<<p.first;
                queue<orders>q=p.second;
                while(!q.empty()){
                    cout<<"qauntity"<<q.front().quantity<<" ";
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
            cout << "\nMARKET BUY: "
                    << "ID=" << id
                    << " QTY=" << quantity
                    << endl;
            while(quantity>0&& !sellbook.empty()){
                auto bestsell=sellbook.begin();
                while(quantity>0&&!bestsell->second.empty()){
                    orders &sellorder=bestsell->second.front();
                    int traded=min(quantity,sellorder.quantity);
                    cout << "TRADE: "
                            << traded
                            << " @ "
                            << bestsell->first
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
                            << endl;
                }
        }
        void addmarketsell(int id,int quantity){
            cout << "\nMARKET SELL: "
                << "ID=" << id
                << " QTY=" << quantity
                << endl;
            while(quantity>0&&!buybook.empty()){
                auto bestbuy=buybook.begin();
                while(quantity>0&&!bestbuy->second.empty()){
                    orders &buyorder=bestbuy->second.front();
                    int traded=min(quantity,buyorder.quantity);
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
                        << endl;
                }
        }
        
};

int main(){

    orderbook ob;
    ob.addbuy(1,102,5);
    ob.addbuy(2,104,3);
    ob.addbuy(3,108,8);

    ob.cancel_order(2);
    ob.addsell(8,109,4);
    ob.addsell(6,108,8);
    ob.addsell(2,106,5);

    ob.addmarketbuy(7,9);
    ob.addmarketsell(6,5);

    ob.printbook();



}



