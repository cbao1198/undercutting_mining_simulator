//
//  function_fork_miner.cpp
//  BlockSim
//
//  Created by Harry Kalodner on 5/25/16.
//  Copyright © 2016 Harry Kalodner. All rights reserved.
//

#include "function_fork_miner.hpp"
#include "block.hpp"
#include "blockchain.hpp"
#include "miner.hpp"
#include "publishing_strategy.hpp"
#include "strategy.hpp"

//#include <gsl/gsl_sf_lambert.h>

#include <cmath>
#include <cassert>

#include <iostream>
#include <map>

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

Block &blockToMineOnNonAtomic(const Miner &, const Blockchain &chain, ForkFunc func, const Alpha alpha);
Block &blockToMineOnAtomic(const Miner &me, const Blockchain &chain, ForkFunc f, const Alpha alpha);

Value valCont(const Blockchain &chain, ForkFunc f, const Block &block, const Alpha alpha);
std::map<int,Value,std::greater<int>> txFeesCont(const Blockchain &chain, ForkFunc f, const Block &block, const Alpha alpha);
Value valUnder(const Blockchain &chain, ForkFunc f, const Block &block, const Alpha alpha);
std::map<int,Value,std::greater<int>> txFeesUnder(const Blockchain &chain, ForkFunc f, const Block &block, const Alpha alpha);

std::unique_ptr<Strategy> createFunctionForkStrategy(bool atomic, ForkFunc f, std::string type) {
    ParentSelectorFunc mineFunc;
    
    if (atomic) {
        mineFunc = std::bind(blockToMineOnAtomic, _1, _2, f, _3);
    } else {
        mineFunc = std::bind(blockToMineOnNonAtomic, _1, _2, f, _3);
    }
    auto valueFunc = std::bind(functionForkValueInMinedChild, _1, _2, f, _3);
    
    return std::make_unique<Strategy>("function-fork-" + type, mineFunc, valueFunc);
}

Block &blockToMineOnNonAtomic(const Miner &, const Blockchain &chain, ForkFunc f, const Alpha alpha) {
    Block &contBlock = chain.most(chain.getMaxHeightPub());
    if (chain.getMaxHeightPub() == BlockHeight(0)) {
        return contBlock;
    }
    Block &underBlock = chain.most(chain.getMaxHeightPub() - BlockHeight(1));
    
    if (valCont(chain, f, contBlock, alpha) >= valUnder(chain, f, underBlock, alpha)) {
        return contBlock;
    } else {
        return underBlock;
    }
}

Block &blockToMineOnAtomic(const Miner &me, const Blockchain &chain, ForkFunc f, const Alpha alpha) {
    Block &contBlock = chain.most(chain.getMaxHeightPub(), me);
    if (chain.getMaxHeightPub() == BlockHeight(0)) {
        return contBlock;
    }
    
    Block &underBlock = chain.most(chain.getMaxHeightPub() - BlockHeight(1), me);
    if (contBlock.minedBy(&me) || valCont(chain, f, contBlock, alpha) >= valUnder(chain, f, underBlock, alpha)) {
        return contBlock;
    } else {
        return underBlock;
    }
}

std::map<int,Value,std::greater<int>> functionForkValueInMinedChild(const Blockchain &chain, const Block &block, ForkFunc f, const Alpha alpha) {
    //int tmp;
    //std::cin>>tmp;
    if (block.height == chain.getMaxHeightPub()) {
        return txFeesCont(chain, f, block, alpha);
    } else {
        return txFeesUnder(chain, f, block, alpha);
    }
}

Value valCont(const Blockchain &chain, ForkFunc f, const Block &contBlock, Alpha alpha) {
    auto val = f(chain, contBlock,alpha);
    return val;
}

Value valUnder(const Blockchain &chain, ForkFunc f, const Block &underBlock, Alpha alpha) {
    auto underVal = f(chain, underBlock, alpha);
    auto val = std::min(underVal, chain.gap(chain.getMaxHeightPub()));
    return val;
}
std::map<int,Value,std::greater<int>> txFeesCont(const Blockchain &chain, ForkFunc f, const Block &block, const Alpha alpha)
{
    return chain.remAlphaValCap(block,f(chain ,block, alpha),alpha);
}

std::map<int,Value,std::greater<int>> txFeesUnder(const Blockchain &chain, ForkFunc f, const Block &block, const Alpha alpha)
{
    return chain.remAlphaValCap(block,valUnder(chain,f,block,alpha),alpha);
}

Value functionForkValPercentage(const Blockchain &chain, const Block &block, Alpha alpha, double funcCoeff) {
    auto maxVal = chain.remFees(block, Alpha(-1));
    /*for(auto tx: chain.remAlpha(block,-1))
        {
            std::cout<<tx.first<<"->"<<tx.second<<std::endl;
        }*/
    //std::cout<<"func coeff:"<<funcCoeff<<std::endl;
    double coeff = 1.0 / funcCoeff;
    double newValue = rawValue(maxVal) * coeff;
    //std::cout<<"new value: "<<Value(newValue)<<std::endl;
    return chain.remAlphaValCapFees(block,newValue,alpha);
}
Value functionForkNumPercentage(const Blockchain &chain, const Block &block, Alpha alpha, double funcCoeff) {
    
    double coeff = 1.0 / funcCoeff;
    //std::cout<<"func coeff:"<<funcCoeff<<std::endl;
    double newValue = chain.getTotalTransactions() * coeff;
    //std::cout<<"new value: "<<Value(newValue)<<std::endl;
    return chain.remFees(block,std::min(Alpha(newValue),alpha));
}

/*Value functionForkLambert(const Blockchain &blockchain, Value maxVal, double lambertCoeff) {
    //don't include B-- this is about the expected portion form tx fees
    auto expectedBlockSize = blockchain.expectedBlockSize();
    auto expectedSizeRaw = rawValue(expectedBlockSize);
    auto blockRatio = valuePercentage(maxVal, Value(expectedBlockSize));
    if (blockRatio <= lambertCoeff) {
        return maxVal;
    } else if (blockRatio < 2*lambertCoeff - std::log(lambertCoeff) - 1) {
        
        double argToLambertFunct0 = -lambertCoeff*std::exp(blockRatio-2*lambertCoeff);
        double lambertRes = gsl_sf_lambert_W0(argToLambertFunct0);
        
        double newValue = -expectedSizeRaw * lambertRes;
        return Value(static_cast<ValueType>(newValue));
    } else {
        return Value(expectedBlockSize);
    }
}*/
