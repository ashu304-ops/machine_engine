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
        cout<<"buyers side"<<" "<<"id"<<" "<<id<<" "<<"price"<<" "<<price<<"quantity"<<" "<<quantity<<endl;
        //match against sell order
        while(quantity>0&&!sellbook.empty()){
            auto bestsell=sellbook.begin();
            if(price< bestsell->first){
                break;
            }
            while(quantity>0&&!bestsell->second.empty()){
                orders& sellorder=bestsell->second.front();
                int traded=min(quantity,sellorder.quantity);
                cout<<"Trade qauntity"<<traded<<"@"<<bestsell->first;
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
            cout<<"seller side"<<" "<<"id"<<" "<<id<<" "<<"price"<<" "<<price<<"quantity"<<" "<<quantity<<endl;
            //buy price >=sell price in this case price is seller reject case price<buy price
            while(quantity>0&&!buybook.empty()){
                auto buy=buybook.begin();
                
                if(price > buy->first){
                    break;
                }
                while(quantity>0&&!buy->second.empty()){
                    orders& buyorder=buy->second.front();
                    int traded=min(quantity,buyorder.quantity);
                    cout<<"Trade qauntity"<<traded<<"@"<<buy->first;
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
            cout<<"order book"<<endl;
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
            cout<<"buy book"<<endl;
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
            cout<<"cancelling order with order id"<<id;
            //looking on buybook side
            for(auto &p:buybook){
                queue<orders>q;
                while(!p.second.empty()){
                    orders ord=p.second.front();
                    p.second.pop();

                    if(ord.id!=id){
                        q.push(ord);
                    }else{
                        cout<<"cancelled the order"<<"@ ID"<<ord.id<<endl;
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
                        cout<<"cancelling orders"<<"@"<<ord.id<<endl;
                    }
                }
                p.second=q;
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

    ob.printbook();



}



