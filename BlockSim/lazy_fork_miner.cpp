//
//  lazy_miner.cpp
//  BlockSim
//
//  Created by Harry Kalodner on 5/25/16.
//  Copyright © 2016 Harry Kalodner. All rights reserved.
//

#include "lazy_fork_miner.hpp"

#include "block.hpp"
#include "blockchain.hpp"
#include "miner.hpp"
#include "default_miner.hpp"
#include "publishing_strategy.hpp"
#include "strategy.hpp"
#include <iostream>

#include <cassert>

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

std::map<int,Value,std::greater<int>> lazyValueInMinedChild(const Blockchain &blockchain, const Block &mineHere, const Alpha alpha);
Block &lazyBlockToMineOnAtomic(const Miner &me, const Blockchain &chain, const Alpha alpha);
Block &lazyBlockToMineOnNonAtomic(const Miner &me, const Blockchain &chain, const Alpha alpha);

std::unique_ptr<Strategy> createLazyForkStrategy(bool atomic) {
    
    ParentSelectorFunc mineFunc;
    
    if (atomic) {
        mineFunc = lazyBlockToMineOnAtomic;
    } else {
        mineFunc = lazyBlockToMineOnNonAtomic;
    }
    auto valueFunc = lazyValueInMinedChild;
    
    return std::make_unique<Strategy>("lazy-fork", mineFunc, valueFunc);
}

Block &lazyBlockToMineOnAtomic(const Miner &me, const Blockchain &chain, const Alpha alpha) {
    if (chain.getMaxHeightPub() == BlockHeight(0)) {
        return chain.most(chain.getMaxHeightPub(), me);
    }
    
    if (chain.remFees(chain.most(chain.getMaxHeightPub(), me),alpha) >= chain.gap(chain.getMaxHeightPub())) {
        return chain.most(chain.getMaxHeightPub(), me);
    } else {
        return chain.most(chain.getMaxHeightPub() - BlockHeight(1), me);
    }
}

Block &lazyBlockToMineOnNonAtomic(const Miner &, const Blockchain &chain, const Alpha alpha) {
    if (chain.getMaxHeightPub() == BlockHeight(0)) {
        return chain.most(chain.getMaxHeightPub());
    }
    if (std::min(chain.remFees(chain.most(chain.getMaxHeightPub()),alpha),Value(chain.remFees(chain.most(chain.getMaxHeightPub()),-1)*0.5)) >= chain.gap(chain.getMaxHeightPub())) {
       return chain.most(chain.getMaxHeightPub());
    } else {
       return chain.most(chain.getMaxHeightPub() - BlockHeight(1));
    }
}

std::map<int,Value,std::greater<int>> lazyValueInMinedChild(const Blockchain &chain, const Block &mineHere, const Alpha alpha) {
    Value valCap = chain.remFees(mineHere,-1)*0.5;
    if(mineHere.height < chain.getMaxHeightPub()){
    	valCap = std::min(valCap, chain.gap(chain.getMaxHeightPub()));
    }
    auto val = chain.remAlphaValCap(mineHere, valCap, alpha);
    return val;
}



