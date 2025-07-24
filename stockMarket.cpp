//done
#include "main.hpp"
#include <stdio.h>
#include <iostream>
#include <list>
#include <string>
#include <typeinfo>
#include <sstream>
#include<algorithm>
#include <cmath>
#include<map>
#include<unordered_map>
using namespace std;

int clockTime=0;
struct PairHash {
    size_t operator()(const std::pair<char, std::string>& p) const {
        return std::hash<char>()(p.first) ^ (std::hash<std::string>()(p.second) << 1);
    }
};

enum Side{
    SELL, BUY
};

Side oppositeSide(Side s)
{
    return s==Side::SELL ? Side::BUY : Side::SELL;
}

Side charToSide(char c) { return c == 'B' ? Side::BUY : Side::SELL; }
char SideToChar(Side c) { return c == Side::BUY ? 'B' : 'S'; }

struct NodeOrder{
    int id;
    Side side;      // either S or B
    string symbol;    // either A or T (or W)
    int volume;
    float priceInteger;
    int timeTurn;
    NodeOrder()
    {}
          
    NodeOrder(string id_, string symbol_, string price_, string volume_, std::string side_, int time_)
        : id(stoi(id_)), symbol(symbol_), priceInteger(stof(price_)), volume(stoi(volume_)), side(charToSide(side_[0])),
          timeTurn(time_) {}

    void copyNode(const NodeOrder& other) {
        id = other.id;
        side = other.side;
        symbol = other.symbol;
        volume = other.volume;
        priceInteger = other.priceInteger;
        timeTurn = other.timeTurn;
    }
    void setTimeTurn(int v){
        timeTurn = v;
    }
    void setPrice(string v){
        priceInteger = stof(v);
    }
    void setVolume(int v){
        volume = v;
    }
    void setVolume(string v){
        volume = stoi(v);
    }

    void dump()
    {
        cout<<" id"<<id<<" side"<<SideToChar(side)<<" symbol"<<symbol<<" volume"<<volume<<" priceInteger"<<priceInteger<<" timeTurn"<<timeTurn<<endl;
    }
};
// sort based on id - regarding remove / edit , OPTIMIZE lookup
unordered_map<int, list<NodeOrder>::iterator> idMap; 
// map side and symbol => order book
// sort based on price, regarding find a match , OPTIMIZE keeping sorted
unordered_map<pair<Side,string> , map<float, list<NodeOrder>>, PairHash> orderBooks;

NodeOrder copyFromStockByid(string argid){
    int argid_ = stoi(argid);
    NodeOrder temp;
    if(idMap.contains(argid_))
    {
        temp.copyNode(*idMap[argid_]);
        return temp;
    }
    return temp;
}
void pull(int argid)
{
    if(idMap.contains(argid))
    {
        orderBooks[{(idMap[argid])->side, (idMap[argid])->symbol}][(idMap[argid])->priceInteger].erase(idMap[argid]);
        idMap.erase(argid);    
    }
}

std::string floatToStr(float val) {
    
    string str(to_string((int)(val * 10000) / 1));
    str.insert(str.size()-4, ".");
    str.erase(str.find_last_not_of('0') + 1, string::npos);
    if (str.back() == '.') str.pop_back();
    return str;
}
// vector<int> collectPotentialRecords(NodeOrder aggressiveOffer)
pair<bool, map<float, list<NodeOrder>>::iterator> collectPotentialRecords(NodeOrder& aggressiveOffer)
{
    Side opposeSide = oppositeSide(aggressiveOffer.side);
    bool success=true;
    auto& book = orderBooks[{ opposeSide, aggressiveOffer.symbol }]; // map<float, list<NodeOrder>>
    map<float, list<NodeOrder>>::iterator bestOffer = book.end();
    if (!book.empty()) {
        if(opposeSide==Side::BUY)
        {
            if( prev(book.end())->first >= aggressiveOffer.priceInteger)
                bestOffer = prev(book.end());  // Highest price
        }
        else 
        {
            if( book.begin()->first <= aggressiveOffer.priceInteger)
                bestOffer = book.begin();  // Lowest price
        }
    }
    else success=false;
    if( bestOffer == book.end()) success=false;
    
    pair<bool, map<float, list<NodeOrder>>::iterator> returnvalue = {success, bestOffer};
    return returnvalue;
}

void checkMatching(NodeOrder &aggressiveOffer, vector<string> &output)
{
    Side opposeSide = oppositeSide(aggressiveOffer.side);
    
    pair<bool, map<float, list<NodeOrder>>::iterator> pos = collectPotentialRecords(aggressiveOffer);
    while((aggressiveOffer.volume != 0) && pos.first)
    {
        auto& searchBookIt = pos.second;
        auto& nodeList = searchBookIt->second;  // This is the list<NodeOrder>
        auto nodeIt = nodeList.begin();

        for (nodeIt = nodeList.begin(); (nodeIt != nodeList.end()) && (aggressiveOffer.volume != 0); ++nodeIt) 
        {   // from the newest to the oldest
            stringstream sstm;
            int id = nodeIt->id;
            
            int dealVolume=min(nodeIt->volume, aggressiveOffer.volume);
            nodeIt->volume = nodeIt->volume - dealVolume;
            aggressiveOffer.setVolume(aggressiveOffer.volume - dealVolume);
            
            sstm<<aggressiveOffer.symbol<<","<<nodeIt->priceInteger<<","<<dealVolume<<","<<aggressiveOffer.id<<","<<nodeIt->id;
            output.push_back(sstm.str());
            if(aggressiveOffer.volume ==0)break;
        }
        if( (aggressiveOffer.volume != 0) )
        {
            orderBooks[{opposeSide, aggressiveOffer.symbol}].erase(searchBookIt);
        }
        else 
        {
            while ( nodeList.front().id != nodeIt->id ) 
            {
                pull(nodeList.front().id);
            }
            if ( nodeIt->volume == 0 ) 
            {
                pull(nodeIt->id);
                if ( nodeList.empty() ) orderBooks[{opposeSide, aggressiveOffer.symbol}].erase(searchBookIt);
            }
            break;
        }
        pos = collectPotentialRecords(aggressiveOffer);
    }
    if(aggressiveOffer.volume !=0)
    {
        auto& orderList = orderBooks[{aggressiveOffer.side, aggressiveOffer.symbol}][aggressiveOffer.priceInteger];
        
        orderList.push_back(aggressiveOffer);
        idMap[aggressiveOffer.id] = prev(orderList.end());
    }
    else pull(aggressiveOffer.id);
}

vector<string> printingOffers(vector<string> &output)
{
    vector<string> output2;
    string symbArr[3] = {"AAPL","TSLA","WEBB"};
    for( string& symb : symbArr)
    {
        std::stringstream sstm;
        if(orderBooks[{ Side::SELL, symb }].size() || orderBooks[{ Side::BUY, symb }].size()){
            sstm<<"==="<<symb<<"==="; 
            output.push_back(sstm.str());
        }
        for (auto itSell = orderBooks[{ Side::SELL, symb }].begin(); itSell != orderBooks[{ Side::SELL, symb }].end(); ++itSell)
        {
            int volumes=0;
            for (auto itList = itSell->second.begin(); itList != itSell->second.end(); ++itList)
            {
                volumes+=itList->volume;
            }
            sstm.str("");
            sstm<<","<<","<<itSell->first<<","<<volumes; 
            output2.push_back(sstm.str());
        }
        int indVec=0;
        for (auto itSell = orderBooks[{ Side::BUY, symb }].rbegin(); itSell != orderBooks[{ Side::BUY, symb }].rend(); ++itSell)
        {
            int volumes=0;
            for (auto itList = itSell->second.begin(); itList != itSell->second.end(); ++itList)
            {
                volumes+=itList->volume;
            }
            string temp("");
            if(indVec<output2.size())
                temp = output2[indVec];
            else temp = ",,,";
            temp.insert(1, to_string(volumes));
            temp.insert(0, floatToStr(itSell->first));
            if(indVec<output2.size())
                output2[indVec] = temp;
            else output2.push_back(temp);
            indVec++;
        }
        output.insert(output.end(), output2.begin(), output2.end());
        output2.clear();
    }
    return output;
}

void insertFunction(vector<string> tokens, vector<string> &output)
{
    NodeOrder temp(tokens[1], tokens[2], tokens[4], tokens[5], tokens[3], clockTime++);
    checkMatching(temp, output);
}

void amendFunction(vector<string> tokens, vector<string> &output)
{
    NodeOrder node = copyFromStockByid(tokens[1]);
    
    if( stof(tokens[2]) != node.priceInteger)
    {
        node.setVolume(stoi(tokens[3]));
        node.setTimeTurn(clockTime++);
        node.setPrice(tokens[2]);
        pull(node.id); //?
        checkMatching(node, output);
    }else{
        if(node.volume >= stoi(tokens[3]))
        {  // no time penalty
            int argid = stoi(tokens[1]);
            auto& nodeList = orderBooks[{(idMap[argid])->side, (idMap[argid])->symbol}][(idMap[argid])->priceInteger];
            for (auto nodeIt = nodeList.begin(); (nodeIt != nodeList.end()); ++nodeIt) 
            {
                if(nodeIt->id == argid){
                    nodeIt->volume=stoi(tokens[3]);   
                }
            }
        }
        else{
            node.setVolume(stoi(tokens[3]));
            node.setTimeTurn(clockTime++);
            pull(node.id); //?
            checkMatching(node, output);
        }
    }
}

vector<string> tokenize(const string& line) {
    vector<string> tokens;
    stringstream ss(line);
    string token;
    while (getline(ss, token, ',')) {
        tokens.push_back(token);
    }
    return tokens;
}

vector<string> run(vector<string> const& input) {
    vector<string> output;
    orderBooks.clear();
    idMap.clear();
    for(const string& term : input)
    {
        auto tokens = tokenize(term);
        if( tokens[0] =="INSERT" )
        {
            insertFunction(tokens, output);
        }
        else if( tokens[0] == "PULL" )
        {
            pull(stoi(tokens[1]));
        }
        else
        {
            amendFunction(tokens, output);
        }
    }
    printingOffers(output);
    // for(auto el : output)cout<<el<<endl;
    return output;
}
