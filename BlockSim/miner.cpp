//
//  miner.cpp
//  BlockSim
//
//  Created by Harry Kalodner on 5/25/16.
//  Copyright © 2016 Harry Kalodner. All rights reserved.
//

#include "miner.hpp"
#include "block.hpp"
#include "blockchain.hpp"
#include "logging.h"
#include "minerParameters.h"
#include "mining_style.hpp"
#include "simple_mining_style.hpp"
#include "picky_mining_style.hpp"
#include "publishing_strategy.hpp"
#include "simple_publisher.hpp"
#include "utils.hpp"
#include "strategy.hpp"

#include <assert.h>
#include <string>
#include <utility>
#include <iostream>
#include <cmath>
#include <map>
#include <sstream>
#include <limits>

Miner::Miner(MinerParameters params_, const Strategy &strategy_) : strategy(strategy_), params(params_) { }

Miner::~Miner() = default;

void Miner::reset(const Blockchain &blockchain) {
    blocksMinedTotal = BlockCount(0);
    unbroadcastBlocks.clear();
    _nextPublishTime = BlockTime(std::numeric_limits<TimeType>::max());
    _lastCostUpdate = BlockTime(0);
    _nextMiningTime = strategy.get().miningStyle->nextMiningTime(blockchain, *this);
    totalMiningCost = 0;
    waitingForBroadcast = false;
}

void Miner::changeStrategy(const Strategy &strategy_, const Blockchain &chain) {
    totalMiningCost += strategy.get().miningStyle->resetMiningCost(*this, chain, _lastCostUpdate);
    strategy = strategy_;
    _lastCostUpdate = chain.getTime();
    _nextMiningTime = strategy.get().miningStyle->nextMiningTime(chain, *this);
}

void Miner::finalize(Blockchain &blockchain) {
    totalMiningCost += strategy.get().miningStyle->resetMiningCost(*this, blockchain, _lastCostUpdate);
    _lastCostUpdate = blockchain.getTime();
    
    for (auto &block : unbroadcastBlocks) {
        blockchain.publishBlock(std::move(block));
    }
}

std::unique_ptr<Block> Miner::miningPhase(Blockchain &chain, Alpha alpha) {
    assert(chain.getTime() == nextMiningTime());
    COMMENTARY("\tMiner " << params.name << "'s turn. " << strategy.get().name << ". ");
    
    auto miningPair = strategy.get().miningStyle->attemptToMine(chain, this, _lastCostUpdate, alpha);
    _lastCostUpdate = chain.getTime();
    auto &block = miningPair.first;
    
    totalMiningCost += miningPair.second;
    
    assert(nextMiningTime() == chain.getTime());
    
    COMMENTARYBLOCK (
        if (block) {
            COMMENTARY("\n### Miner " << params.name << " found a block: " << *block << " on top of " << *block->parent << " with value " << block->value << " ###\n");
        }
    )
    
    _nextMiningTime = strategy.get().miningStyle->nextMiningTime(chain, *this);
    if (block) {
        blocksMinedTotal++;
        if (strategy.get().publisher->withholdsBlocks()) {
            waitingForBroadcast = true;
            unbroadcastBlocks.push_back(std::move(block));
        } else {
            block->broadcast(chain.getTime() + params.networkDelay);
            return std::move(block);
        }
    }
    
    return nullptr;
}

std::vector<std::unique_ptr<Block>> Miner::broadcastPhase(const Blockchain &chain) {
    assert(unbroadcastBlocks.size() > 0);
    std::vector<std::unique_ptr<Block>> blocks;
    auto &publisher = strategy.get().publisher;
    auto blocksToPublish = publisher->publishBlocks(chain, *this, unbroadcastBlocks);
    waitingForBroadcast = !unbroadcastBlocks.empty();
    for (auto &block : blocksToPublish) {
        block->broadcast(chain.getTime() + params.networkDelay);
        COMMENTARY("Miner " << params.name << " publishes " << *block << "\n");
    }
    return blocksToPublish;
}

Block *Miner::newestUnpublishedBlock() const {
    if (unbroadcastBlocks.empty()) {
        return nullptr;
    }
    
    Block *block = unbroadcastBlocks[0].get();
    for (auto &unpublishedBlock : unbroadcastBlocks) {
        if (unpublishedBlock->height > block->height) {
            block = unpublishedBlock.get();
        }
    }
    return block;
}

void Miner::updateNextPublishTime(BlockTime newTime, const Blockchain &chain) {
    assert(newTime >= chain.getTime());
    _nextPublishTime = newTime;
}

void Miner::printStrategy() const {
    std::cout<< "[" << strategy.get().name  << "] miner " << params.name<<std::endl;
}

std::ostream& operator<<(std::ostream& os, const Miner& miner) {
    miner.print(os);
    return os;
}

void Miner::print(std::ostream& os) const {
    os << "[" << strategy.get().name  << "] miner " << params.name;
}

bool Miner::isFork() const{
    return strategy.get().name.find("fork") != std::string::npos;
}
bool Miner::isFractional() const{
    return strategy.get().name.find("fractional") != std::string::npos;
}

bool Miner::isPetty() const{
    return strategy.get().name.find("petty") != std::string::npos;
}

bool ownBlock(const Miner *miner, const Block *block) {
    return block->minedBy(miner);
}

bool Miner::publishesNextRound() const {
    return unbroadcastBlocks.size() > 0;
}
