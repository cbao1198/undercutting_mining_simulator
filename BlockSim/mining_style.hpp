//
//  mining.hpp
//  BlockSim
//
//  Created by Harry Kalodner on 6/11/16.
//  Copyright © 2016 Harry Kalodner. All rights reserved.
//

#ifndef mining_hpp
#define mining_hpp

#include "typeDefs.hpp"

#include <functional>
#include <memory>
#include <map>

class Blockchain;
class Miner;
class Block;

using ParentSelectorFunc = std::function<Block &(const Miner &, const Blockchain &, const Alpha)>;
//using txStruct = std::map<int,Value,std::greater<int>>;
using BlockValueFunc = std::function<std::map<int,Value,std::greater<int>>(const Blockchain &, const Block &, const Alpha)>;

class MiningStyle {

private:
    const ParentSelectorFunc parentSelectorFunc;
    const BlockValueFunc blockValueFunc;
    
protected:
    MiningStyle(ParentSelectorFunc parentSelectorFunc, BlockValueFunc blockValueFunc);
    
    std::unique_ptr<Block> createBlock(Blockchain &blockchain, const Miner &me, const Alpha alpha);
    
public:
    
    virtual ~MiningStyle();
    
    virtual std::pair<std::unique_ptr<Block>, Value> attemptToMine(Blockchain &blockchain, Miner *miner, BlockTime lastTimePaid, Alpha alpha) = 0;
    virtual BlockTime nextMiningTime(const Blockchain &chain, const Miner &miner) const = 0;
    virtual Value resetMiningCost(const Miner &miner, const Blockchain &chain, BlockTime lastTimePaid) = 0;
};

#endif /* mining_hpp */
