//
//  mining.cpp
//  BlockSim
//
//  Created by Harry Kalodner on 6/11/16.
//  Copyright © 2016 Harry Kalodner. All rights reserved.
//

#include "mining_style.hpp"

#include "blockchain.hpp"
#include "miner.hpp"
#include "block.hpp"

#include <assert.h>
#include <map>
#include <iostream>

MiningStyle::MiningStyle(ParentSelectorFunc parentSelectorFunc_, BlockValueFunc blockValueFunc_) :
parentSelectorFunc(parentSelectorFunc_), blockValueFunc(blockValueFunc_) {}

MiningStyle::~MiningStyle() = default;

std::unique_ptr<Block> MiningStyle::createBlock(Blockchain &blockchain, const Miner &miner, const Alpha alpha) {
    auto &parent = parentSelectorFunc(miner, blockchain, alpha);
    Value weight;
    if(parent.height < blockchain.getMaxHeightPub())
    {
         weight = 1;
    }
    else
    {
         weight = 0;
    }
    //auto weight = parent.height;
    
    auto value = blockValueFunc(blockchain, parent, alpha);
    /*long long txTotalVal = 0;
    for(auto tx:value)
    {
        txTotalVal += tx.second;
    }
    if(txTotalVal == alpha)
    {
         weight = 1;
    }
    else
    {
         weight = 0;
    }*/
    //assert(value >= parent.nextBlockReward());
    //assert(value <= parent.nextBlockReward() + blockchain.rem(parent));
    /*if(blockchain.getTime()%10 == 1)
    {
        std::cout<<"making block:\n";
    for(auto tx:value)
    {
        std::cout<<tx.first<<"->"<<tx.second<<std::endl;
    }
    }*/
    
    
    auto newBlock = blockchain.createBlock(&parent, &miner, value, weight);
    /*std::cout<<"block tx vals parent :\n";
    for(auto tx: newBlock->parent->txFeesInChain)
    {
        std::cout<<tx.first<<"->"<<tx.second<<std::endl;
    }
    std::cout<<"block tx vals  :\n";
    for(auto tx: newBlock->txFeesInChain)
    {
        std::cout<<tx.first<<"->"<<tx.second<<std::endl;
    }
    int tmp;*/
    //std::cin>>tmp;
    return newBlock;
}
