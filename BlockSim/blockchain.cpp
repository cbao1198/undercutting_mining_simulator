//
//  blockchain.cpp
//  BlockSim
//
//  Created by Harry Kalodner on 5/25/16.
//  Copyright © 2016 Harry Kalodner. All rights reserved.
//

#include "blockchain.hpp"
#include "utils.hpp"
#include "block.hpp"
#include "blockchain_settings.hpp"

#include <cassert>
#include <unordered_set>
#include <iostream>
#include <limits>
#include <random>

Blockchain::Blockchain(BlockchainSettings blockchainSettings) :
    timeInSecs(0),
    secondsPerBlock(blockchainSettings.secondsPerBlock),
    transactionFeeRate(blockchainSettings.transactionFeeRate),
    _maxHeightPub(0),
    alpha(Alpha(-1))
{
    _blocks.reserve(rawCount(blockchainSettings.numberOfBlocks) * 2);
    _blocksIndex.resize(rawCount(blockchainSettings.numberOfBlocks) * 2);
    _smallestBlocks.resize(rawCount(blockchainSettings.numberOfBlocks) * 2);
    alpha = blockchainSettings.alpha;
    totalFeesInput = 0;
    for (auto transaction : transactionFeeRate)
    {
        totalFeesInput += transaction.first * transaction.second;
    }
    reset(blockchainSettings);
}

std::unique_ptr<Block> Blockchain::createBlock(const Block *parent, const Miner *miner, std::map<int,Value,std::greater<int>> value, bool wasUndercut) {
    
    if (_oldBlocks.size() == 0) {
        
        return std::make_unique<Block>(parent, miner, getTime(), value, wasUndercut);
    }
    
    auto block = std::move(_oldBlocks.back());
    _oldBlocks.pop_back();
    block->reset(parent, miner, getTime(), value);
    return block;
}

void Blockchain::reset(BlockchainSettings blockchainSettings) {
    //valueNetworkTotal = 0;
    availableTransactions.clear();
    timeInSecs = BlockTime(0);
    secondsPerBlock = blockchainSettings.secondsPerBlock;
    transactionFeeRate = blockchainSettings.transactionFeeRate;
    _maxHeightPub = BlockHeight(0);
    _oldBlocks.reserve(_oldBlocks.size() + _blocks.size());
    for (auto &block : _blocks) {
        _oldBlocks.push_back(std::move(block));
    }
    _blocks.clear();
    _smallestBlocks[0].clear();
    _blocksIndex[0].clear();
    auto genesis = std::make_unique<Block>(blockchainSettings.blockReward);
    _smallestBlocks[0].push_back(genesis.get());
    _blocksIndex[0].push_back(_blocks.size());
    _blocks.push_back(std::move(genesis));
}

void Blockchain::publishBlock(std::unique_ptr<Block> block) {
    assert(block);
    assert(block->height <= _maxHeightPub + BlockHeight(1));
    
    HeightType height = rawHeight(block->height);
    
    if (block->height > _maxHeightPub) {
        _blocksIndex[height].clear();
        _smallestBlocks[height].clear();
        _smallestBlocks[height].push_back(block.get());
        _maxHeightPub = block->height;
    } else {
        std::vector<Block *> &smallestVector = _smallestBlocks[height];
        Value currFront;
        if(alpha != -1){
            auto pos_alpha = Value(static_cast<ValueType>(alpha));
            currFront = std::min(smallestVector.front()->value,pos_alpha);
        }
        else{
            currFront = smallestVector.front()->value;
        }
        
        Value blockVal;
        
        if(alpha != -1){
            auto pos_alpha = Value(static_cast<ValueType>(alpha));
            blockVal = std::min(block->value,pos_alpha);
        }
        else{
            blockVal = block->value;
        }

        if (blockVal < currFront) {
            smallestVector.clear();
            smallestVector.push_back(block.get());
        } else if (blockVal == currFront) {
            smallestVector.push_back(block.get());
        }
    }

    _blocksIndex[height].push_back(_blocks.size());
    _blocks.push_back(std::move(block));
}

BlockCount Blockchain::blocksOfHeight(BlockHeight height) const {
    return BlockCount(_blocksIndex[rawHeight(height)].size());
}

const std::vector<Block *> Blockchain::oldestBlocks(BlockHeight height) const {
    BlockTime minTimePublished(std::numeric_limits<BlockTime>::max());
    for (size_t index : _blocksIndex[rawHeight(height)]) {
        minTimePublished = std::min(_blocks[index]->getTimeBroadcast(), minTimePublished);
    }
    
    std::vector<Block *> possiblities;
    for (size_t index : _blocksIndex[rawHeight(height)]) {
        if (_blocks[index]->getTimeBroadcast() == minTimePublished) {
            possiblities.push_back(_blocks[index].get());
        }
    }
    assert(possiblities.size() > 0);
    return possiblities;
}

Block &Blockchain::oldest(BlockHeight height) const {
    auto possiblities = oldestBlocks(height);
    return *possiblities[selectRandomIndex(possiblities.size())];
}

Block &Blockchain::most(BlockHeight height) const {
    auto &smallestBlocks = _smallestBlocks[rawHeight(height)];
    size_t index = selectRandomIndex(smallestBlocks.size());
    Block *block = smallestBlocks[index];
    return *block;
}

const Block &Blockchain::winningHead() const {
    Value largestValue(0);
    for (size_t index : _blocksIndex[rawHeight(getMaxHeightPub())]) {
        largestValue = std::max(largestValue, _blocks[index]->valueInChain);
    }
    
    std::vector<Block *> possiblities;
    for (size_t index : _blocksIndex[rawHeight(getMaxHeightPub())]) {
        if (_blocks[index]->valueInChain == largestValue) {
            possiblities.push_back(_blocks[index].get());
        }
    }
    std::uniform_int_distribution<std::size_t> vectorDis(0, possiblities.size() - 1);
    return *possiblities[selectRandomIndex(possiblities.size())];
}

void Blockchain::advanceToTime(BlockTime time) {
    assert(time >= timeInSecs);
    for (auto transaction: transactionFeeRate)
    {
        
        /*
        //used to simulate high fees coming in every cycleLength rounds; can be uncommented if needed
        cycleLength = 2
        highFeeValue = 2
        if(transaction.first > 1 && time % cycleLength == 0)
        {
            availableTransactions[transaction.first] = transactionFeeRate[transaction.first] * time / cycleLength;
            availableTransactions[1] = 0;
        }
        else if (transaction.first == 1)
        {
            availableTransactions[transaction.first] += transactionFeeRate[highFeeValue] * time;
        }
        else
        {
            availableTransactions[1] = transactionFeeRate[transaction.first] * time;
            long long numHigh = time / cycleLength;
            availableTransactions[1] -= transactionFeeRate[transaction.first] * numHigh;
        }*/
        availableTransactions[transaction.first] = transactionFeeRate[transaction.first] * time;
    }
    timeInSecs = time;
}

BlockValue Blockchain::expectedBlockSize() const {
    return totalFeesInput * secondsPerBlock + BlockValue(_smallestBlocks[rawHeight(getMaxHeightPub())].front()->nextBlockReward());
}

TimeRate Blockchain::chanceToWin(HashRate hashRate) const {
    return hashRate / secondsPerBlock;
}

Block *Blockchain::blockByMinerAtHeight(BlockHeight height, const Miner &miner) const {
    for (size_t index : _blocksIndex[rawHeight(height)]) {
        if (_blocks[index]->minedBy(&miner)) {
            return _blocks[index].get();
        }
    }
    return nullptr;
}

Block &Blockchain::most(BlockHeight height, const Miner &miner) const {
    Block *block = blockByMinerAtHeight(height, miner);
    if (block) {
        return *block;
    }
    
    return most(height);
}

Block &Blockchain::oldest(BlockHeight height, const Miner &miner) const {
    Block *block = blockByMinerAtHeight(height, miner);
    if (block) {
        return *block;
    }
    
    return oldest(height);
}

Value Blockchain::gap(BlockHeight height) const {
    return remFees(most(height - BlockHeight(1)),-1) - remFees(most(height),-1);
}

Value Blockchain::remFees(const Block &block, const Alpha alphaCap) const {
    auto remAlphaMap = remAlpha(block, alphaCap);
    Value totalFees = 0;
    for(auto tx : remAlphaMap)
    {
        totalFees += tx.first*tx.second;
    }
    return totalFees;
}

std::map<int,ValueRate,std::greater<int>> Blockchain::remAlpha(const Block &block,const Alpha alphaCap) const {
    std::map<int, Value,std::greater<int>> ans(availableTransactions);
    Value totalFees = 0;
    for(auto tx: availableTransactions)
    {
        if(block.txFeesInChain.find(tx.first) != block.txFeesInChain.end())
        {
            ans[tx.first] -= block.txFeesInChain.at(tx.first);
        }
        
        if(alphaCap > 0 && totalFees + ans[tx.first] > alphaCap)
        {
            ans[tx.first] = alphaCap - totalFees;
            return ans;
        }
        totalFees += ans[tx.first];
    }
    return ans;
}
std::map<int,ValueRate,std::greater<int>> Blockchain::remAlphaValCap(const Block &block, const Value valCap, const Alpha alpha) const {
    std::map<int, Value,std::greater<int>> ans;
    Value totalFees = 0;
    Value totalVal = 0;
    for(auto tx: availableTransactions)
    {
        if(block.txFeesInChain.find(tx.first) != block.txFeesInChain.end())
        {
            ans[tx.first] = tx.second - block.txFeesInChain.at(tx.first);
        }
        else
        {
            ans[tx.first] = tx.second;
        } 
        if(totalFees + ans[tx.first] > alpha)
        {
            ans[tx.first] = alpha - totalFees;
        }
        if(totalVal + ans[tx.first]*tx.first > valCap)
        {
            ans[tx.first] = (valCap-totalVal)/tx.first;
        }
        totalFees += ans[tx.first];
        totalVal += ans[tx.first]*tx.first;
    }
    return ans;
}
Value Blockchain::remAlphaValCapFees(const Block &block, const Value valCap, const Alpha alpha) const {
    std::map<int, Value,std::greater<int>> ans(availableTransactions);
    Value totalFees = 0;
    Value totalVal = 0;
    for(auto tx: availableTransactions)
    {
        if(block.txFeesInChain.find(tx.first) != block.txFeesInChain.end())
        {
            ans[tx.first] -= block.txFeesInChain.at(tx.first);
        }
        if(totalFees + ans[tx.first] > alpha)
        {
            ans[tx.first] = alpha - totalFees;
        }
        if(totalVal + ans[tx.first]*tx.first > valCap)
        {
            ans[tx.first] = (valCap-totalVal)/tx.first;
        }
        totalFees += ans[tx.first];
        totalVal += ans[tx.first]*tx.first;
    }
    return totalVal;
}

const std::vector<const Block *> Blockchain::getHeads() const {
    std::unordered_set<const Block *> nonHeadBlocks;
    std::unordered_set<const Block *> possibleHeadBlocks;
    for (auto &block : _blocks) {
        if (block->parent != nullptr) {
            nonHeadBlocks.insert(block->parent);
            possibleHeadBlocks.erase(block->parent);
        }
        if (nonHeadBlocks.find(block.get()) == end(nonHeadBlocks)) {
            possibleHeadBlocks.insert(block.get());
        }
    }
    return std::vector<const Block *>(std::begin(possibleHeadBlocks), std::end(possibleHeadBlocks));
}

void Blockchain::printBlockchain() const {
    std::unordered_set<const Block *> printedBlocks;
    for (auto &current : getHeads()) {
        auto chain = current->getChain();
        for (auto block : chain) {
            if (printedBlocks.find(block) == end(printedBlocks)) {
                std::cout << *block;
                printedBlocks.insert(block);
            } else {
                break;
            }
        }
        std::cout << std::endl;
    }
}

void Blockchain::printHeads() const {
    std::cout << "heads:" << std::endl;
    for (auto current : getHeads()) {
        std::cout << *current << std::endl;
    }
    std::cout << "end heads." << std::endl;
}


