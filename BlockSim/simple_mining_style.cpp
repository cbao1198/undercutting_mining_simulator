//
//  simple_miner.cpp
//  BlockSim
//
//  Created by Harry Kalodner on 6/11/16.
//  Copyright © 2016 Harry Kalodner. All rights reserved.
//

#include "simple_mining_style.hpp"
#include "blockchain.hpp"
#include "utils.hpp"
#include "block.hpp"
#include "miner.hpp"

#include <assert.h>
#include <cmath>
#include <iostream>

SimpleMiningStyle::SimpleMiningStyle(ParentSelectorFunc parentSelectorFunc_, BlockValueFunc blockValueFunc_) : MiningStyle(parentSelectorFunc_, blockValueFunc_) {}

std::pair<std::unique_ptr<Block>, Value> SimpleMiningStyle::attemptToMine(Blockchain &blockchain, Miner *miner, BlockTime lastTimePaid, Alpha alpha) {
    auto block = createBlock(blockchain, *miner, alpha);
    if(miner->isFork() == false && block->profitWeight == 100)
    {
	    std::cout<<*miner<<std::endl;
	    std::cout<<block->profitWeight<<std::endl;
    } 
    Value cost = resetMiningCost(*miner, blockchain, lastTimePaid);
    
    return std::make_pair(std::move(block), cost);
}

BlockTime SimpleMiningStyle::nextMiningTime(const Blockchain &chain, const Miner &miner) const {
    auto offset = selectMiningOffset(chain.getSecondsPerBlock());
    long long height = chain.getTime()/chain.getSecondsPerBlock();
    if(chain.getTime() > 0)
    {
        height += 1;
    }
    return (height)*chain.getSecondsPerBlock() + offset;
}

Value SimpleMiningStyle::resetMiningCost(const Miner &miner, const Blockchain &chain, BlockTime lastTimePaid) {
    return miner.params.costPerSecond * (chain.getTime() - lastTimePaid);
}
