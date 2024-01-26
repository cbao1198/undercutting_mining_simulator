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
    bool wasUndercut;
    if(parent.height < blockchain.getMaxHeightPub())
    {
         wasUndercut = true;
    }
    else
    {
         wasUndercut = false;
    }
    
    auto value = blockValueFunc(blockchain, parent, alpha);
    
    auto newBlock = blockchain.createBlock(&parent, &miner, value, wasUndercut);
    return newBlock;
}
