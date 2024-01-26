//
//  block.cpp
//  BlockSim
//
//  Created by Harry Kalodner on 5/25/16.
//  Copyright © 2016 Harry Kalodner. All rights reserved.
//

#include "block.hpp"
#include "miner.hpp"

#include <cassert>
#include <iostream>
#include <limits>
#include <map>

constexpr auto timeMax = std::numeric_limits<TimeType>::max();

Block::Block(BlockValue blockReward_) : Block(nullptr, nullptr, BlockTime(0), std::map<int,Value,std::greater<int>>(), BlockHeight(0), std::map<int,Value,std::greater<int>>(), Value(0), Value(rawValue(blockReward_)), Value(1)) {}

Block::Block(const Block *parent_, const Miner *miner_, BlockTime timeSeconds_, std::map<int,Value,std::greater<int>> txFees ,BlockHeight height_, std::map<int,Value,std::greater<int>> txFeesInChain_, Value valueInChain_, Value blockReward_, bool wasUndercut_) : timeBroadcast(timeMax), parent(parent_), miner(miner_), height(height_), timeMined(timeSeconds_), value(getValue(txFees) + blockReward_), txFeesInChain(txFeesInChain_), valueInChain(valueInChain_), blockReward(blockReward_), wasUndercut(wasUndercut_) {}

Block::Block(const Block *parent_, const Miner *miner_, BlockTime timeSeconds_, std::map<int,Value,std::greater<int>> txFees, bool wasUndercut) :
    Block(parent_, miner_, timeSeconds_, txFees, parent_->height + BlockHeight(1), combineFees(txFees,parent_->txFeesInChain), parent_->valueInChain + parent_->blockReward + getValue(txFees), parent_->nextBlockReward(), wasUndercut) {}

void Block::reset(const Block *parent_, const Miner *miner_, BlockTime timeSeconds_, std::map<int,Value,std::greater<int>> txFees) {
    height = parent_->height + BlockHeight(1);
    timeMined = timeSeconds_;
    timeBroadcast = timeMax;
    auto txFeesValue = getValue(txFees);
    value = txFeesValue + parent_->nextBlockReward();
    txFeesInChain = combineFees(txFees, parent_->txFeesInChain);
    valueInChain = txFeesValue + parent_->valueInChain + parent_->nextBlockReward();
    blockReward = parent_->nextBlockReward();
    parent = parent_;
    miner = miner_;
}

Value Block::nextBlockReward() const {
    return blockReward;
}

void Block::broadcast(BlockTime timePub) {
    timeBroadcast = timePub;
}

bool Block::isBroadcast() const {
    return timeBroadcast < timeMax;
}

BlockTime Block::getTimeBroadcast() const {
    return timeBroadcast;
}

std::ostream& operator<< (std::ostream& out, const Block& mc) {
    mc.print(out, true);
    return out;
}

std::vector<const Block *> Block::getChain() const {
    std::vector<const Block *> chain;
    const Block *current = this;
    while (current) {
        chain.push_back(current);
        current = current->parent;
    }
    return chain;
}

Value Block::getValue(std::map<int,Value,std::greater<int>> txFees)
{
    //get total value of a transaction map
    Value val = 0;
    for (auto transaction: txFees)
    {
        val += transaction.first * transaction.second;
    }
    return val;
}


std::map<int,Value,std::greater<int>> Block::combineFees(std::map<int,Value,std::greater<int>> map1, std::map<int,Value,std::greater<int>> map2)
{
    //combine two transaction maps
    std::map<int, Value,std::greater<int>> ans;
    for(auto tx : map1)
    {
        if(map2.find(tx.first) == map2.end())
        {
            ans[tx.first] = tx.second;
        }
        else
        {
            ans[tx.first] = tx.second + map2[tx.first];
        }
    }
    return ans;
}

void Block::print(std::ostream& os, bool isPublished) const {
    if (height == BlockHeight(0)) {
        os << "[h:0, m:gen]";
        return;
    }
    if (isPublished) {
        os << "{";
    }
    else {
        os << "[";
    }
    
    os << "h:" << height << ", m:" << miner->params.name << ", v:" << value << ", t:" << timeMined;
    
    if (isPublished) {
        os << "}->";
    }
    else {
        os << "]->";
    }
}

bool Block::minedBy(const Miner *miner_) const {
    return miner == miner_;
}
