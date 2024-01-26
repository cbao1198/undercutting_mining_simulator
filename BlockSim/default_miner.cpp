//
//  default_miner.cpp
//  BlockSim
//
//  Created by Harry Kalodner on 5/25/16.
//  Copyright © 2016 Harry Kalodner. All rights reserved.
//

#include "default_miner.hpp"
#include "block.hpp"
#include "blockchain.hpp"
#include "utils.hpp"
#include "miner.hpp"
#include "publishing_strategy.hpp"
#include "strategy.hpp"
#include <iostream>

#include <cmath>
#include <cassert>

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;


std::unique_ptr<Strategy> createDefaultStrategy(bool atomic, bool noiseInTransactions) {
    ParentSelectorFunc mineFunc;
    
    if (atomic) {
        mineFunc = defaultBlockToMineOnAtomic;
    } else {
        mineFunc = defaultBlockToMineOnNonAtomic;
    }
    
    auto valueFunc = std::bind(defaultValueInMinedChild, _1, _2, noiseInTransactions, _3);
    
    return std::make_unique<Strategy>("default-honest", mineFunc, valueFunc);
}

Block &defaultBlockToMineOnAtomic(const Miner &me, const Blockchain &chain, const Alpha alpha) {
    return chain.oldest(chain.getMaxHeightPub(), me);
}

Block &defaultBlockToMineOnNonAtomic(const Miner &, const Blockchain &chain, const Alpha alpha) {
    return chain.oldest(chain.getMaxHeightPub());
}

std::map<int,Value,std::greater<int>> defaultValueInMinedChild(const Blockchain &chain, const Block &mineHere, bool noiseInTransactions, Alpha alpha) {
    return chain.remAlpha(mineHere,alpha);
}
