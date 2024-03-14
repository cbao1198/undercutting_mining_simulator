//
//  blockchain.hpp
//  BlockSim
//
//  Created by Harry Kalodner on 5/25/16.
//  Copyright © 2016 Harry Kalodner. All rights reserved.
//

#ifndef blockchain_hpp
#define blockchain_hpp

#include "typeDefs.hpp"

#include <queue>
#include <stddef.h>
#include <memory>
#include <map>
#include <iostream>
class Block;
class GenesisBlock;
class Miner;
struct BlockchainSettings;

class Blockchain {    
    std::map<int,ValueRate,std::greater<int>> availableTransactions;
    BlockTime timeInSecs;
    BlockRate secondsPerBlock;
    std::map<int,ValueRate,std::greater<int>> transactionFeeRate;
    Alpha alpha;
    Value totalFeesInput;
    int cycle;
    
    
    BlockHeight _maxHeightPub;
    BlockHeight _tempMaxHeightPub;
    std::vector<std::vector<size_t>> _blocksIndex;
    std::vector<std::unique_ptr<Block>> _blocks;
    std::vector<std::vector<Block *>> _smallestBlocks; // cache smallest blocks of a given height
    std::map<int, Block *> _oldestBlocks; // cache smallest blocks of a given height
    
    std::vector<std::unique_ptr<Block>> _oldBlocks;
    
    Block *blockByMinerAtHeight(BlockHeight height, const Miner &miner) const;
    
public:
    Blockchain(BlockchainSettings blockchainSettings);
    
    std::unique_ptr<Block> createBlock(const Block *parent, const Miner *miner, std::map<int,Value,std::greater<int>> value, Value profitWeight);
    void reset(BlockchainSettings blockchainSettings);

    void publishBlock(std::unique_ptr<Block> block);
    
    const std::vector<const Block *> getHeads() const;
    void printBlockchain() const;
    void printHeads() const;
    
    const Block &winningHead() const;
    
    BlockCount blocksOfHeight(BlockHeight height) const;
    
    const std::vector<Block *> oldestBlocks(BlockHeight height) const;
    void setOldest(BlockHeight height);
    Block &oldest(BlockHeight height) const;
    Block &most(BlockHeight age) const;
    
    void advanceToTime(BlockTime time);
    inline BlockTime getSecondsPerBlock() const {
	return secondsPerBlock;
    }	
    inline BlockHeight getMaxHeightPub() const {
        return _maxHeightPub;
    }
    void updateMaxHeightPub(); 
    inline BlockTime getTime() const {
        return timeInSecs;
    }
    
    inline Value getTotalTransactions() const {
        Value valueNetworkTotal = 0;
        for(auto transaction : availableTransactions)
        {
            valueNetworkTotal += transaction.first*transaction.second;
        }
        return valueNetworkTotal;
    }

    inline std::map<int,Value, std::greater<int> > getAvailableTransactions() {
        return availableTransactions;
    }
    
    
    BlockValue expectedBlockSize() const;
    TimeRate chanceToWin(HashRate hashRate) const;
    
    Value gap(BlockHeight i) const;
    Value remFees(const Block &block,const Alpha alphaCap) const;
    Value remAlphaValCapFees(const Block &block, const Value valCap, const Alpha alpha) const;
    
    std::map<int,ValueRate,std::greater<int>> remAlpha(const Block &block,const Alpha alphaCap) const;
    std::map<int,ValueRate,std::greater<int>> remAlphaValCap(const Block &block, const Value valCap, const Alpha alpha) const;
    
    Block &most(BlockHeight age, const Miner &miner) const;
    Block &oldest(BlockHeight age, const Miner &miner) const;
};

#endif /* blockchain_hpp */
