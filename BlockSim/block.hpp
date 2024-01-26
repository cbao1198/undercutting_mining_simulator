//
//  block.hpp
//  BlockSim
//
//  Created by Harry Kalodner on 5/25/16.
//  Copyright © 2016 Harry Kalodner. All rights reserved.
//

#ifndef block_hpp
#define block_hpp

#include "typeDefs.hpp"

#include <vector>
#include <iterator>
#include <map>

class Miner;

class Block {
protected:
    BlockTime timeBroadcast;
public:
    const Block *parent;
    const Miner *miner;
    BlockHeight height;
    BlockTime timeMined;
    Value value;
    std::map<int,Value,std::greater<int>> txFeesInChain;
    Value valueInChain;
    Value blockReward;
    bool wasUndercut;
    
    Block(const Block *parent_, const Miner *miner_, BlockTime timeSeconds, std::map<int,Value,std::greater<int>> txFees, BlockHeight height, std::map<int,Value,std::greater<int>> txFeesInChain_,Value valueInChain, Value blockReward, bool wasUndercut);
    
    Block(BlockValue blockReward);
    Block(const Block *parent_, const Miner *miner_, BlockTime timeSeconds_, std::map<int,Value,std::greater<int>> txFees, bool wasUndercut);
    
    void reset(const Block *parent, const Miner *miner, BlockTime timeSeconds, std::map<int,Value,std::greater<int>> txFees);
    
    void broadcast(BlockTime timePub);
    BlockTime getTimeBroadcast() const;
    bool isBroadcast() const;
    
    Value nextBlockReward() const;
    
    bool minedBy(const Miner *miner) const;
    void print(std::ostream& where, bool isPublished) const;
    std::vector<const Block *> getChain() const;
    Value getValue(std::map<int,Value,std::greater<int>> txFees);
    std::map<int,Value,std::greater<int>> combineFees(std::map<int,Value,std::greater<int>> map1, std::map<int,Value,std::greater<int>> map2);
};

std::ostream& operator<< (std::ostream& out, const Block& mc);

#endif /* block_hpp */
