//
//  game.cpp
//  BlockSim
//
//  Created by Harry Kalodner on 6/6/16.
//  Copyright © 2016 Harry Kalodner. All rights reserved.
//

#include "game.hpp"
#include "blockchain.hpp"
#include "block.hpp"
#include "miner.hpp"
#include "logging.h"
#include "minerGroup.hpp"
#include "miner_result.hpp"
#include "game_result.hpp"

#include "minerStrategies.h"
#include "strategy.hpp"

#include <cassert>
#include <iostream>

GameResult runGame(MinerGroup &minerGroup, Blockchain &blockchain, GameSettings gameSettings, Alpha alpha) {
    
    GAMEINFO("Players:" << std::endl << minerGroup);
    
    //mining loop
    
    BlockTime totalSeconds = gameSettings.blockchainSettings.numberOfBlocks * gameSettings.blockchainSettings.secondsPerBlock;
    Value numUndercut = 0;
    //while (blockchain.getTime() < totalSeconds) 
     while (blockchain.getTime() < gameSettings.blockchainSettings.numberOfBlocks) {
        BlockTime nextTime = minerGroup.nextEventTime(blockchain);
        
        assert(blockchain.getTime() <= nextTime);
        
        blockchain.advanceToTime(nextTime);
        
        assert(blockchain.getTime() == nextTime);
        
        //steps through in second intervals
        //on each step each miner gets a turn
        COMMENTARY("Round " << blockchain.getTime() << " of the game..." << std::endl);
        
        minerGroup.nextMineRound(blockchain, alpha);
        
        minerGroup.nextBroadcastRound(blockchain);
        
        COMMENTARY("Publish phase:" << std::endl);
        
        minerGroup.nextPublishRound(blockchain);
        
        auto &winningBlock = blockchain.winningHead();
        COMMENTARY("Round " << blockchain.getTime() << " over. Current blockchain:" << std::endl);
        COMMENTARYBLOCK (
            blockchain.printBlockchain();
            blockchain.printHeads();
        )
    }
    
    minerGroup.finalize(blockchain);
    
    std::vector<MinerResult> minerResults;
    minerResults.resize(minerGroup.miners.size());
    
    auto &winningBlock = blockchain.winningHead();
    auto winningChain = winningBlock.getChain();
    int parentCount = 0;
    Value totalValue(0);
    Value totalVariance(0);
    auto ind = 0;
    for (auto mined : winningChain) {
        if (mined->height == BlockHeight(0)) {
            break;
        }
        if (mined->parent->minedBy(mined->miner)) {
            parentCount++;
        }
        auto miner = mined->miner;
        size_t minerIndex = minerGroup.miners.size();
        for (size_t ind = 0; ind < minerGroup.miners.size(); ind++) {
            if (minerGroup.miners[ind].get() == miner) {
                minerIndex = ind;
                break;
            }
        }
        if(mined->wasUndercut == 1)
        {
            //count number of times undercut
            numUndercut += 1;
        }
        minerResults[minerIndex].addBlock(mined);
        totalValue += mined->value;
        if(ind == 0){
            ind += 1;
        }
        else{
            //calculate interblock variance for measurement purposes
            auto prevBlock = winningChain[ind-1];
            if(prevBlock-> value > mined->value){
                totalVariance += prevBlock->value - mined->value;
            }
            else{
                totalVariance += mined->value -prevBlock->value;
            }
            ind += 1;
        }
        
    }
    
//    std::cout << parentCount << " block mined over parent" << std::endl;
    
    
    //calculate the score at the end
    BlockCount totalBlocks(0);
    BlockCount finalBlocks(0);
    
    for (size_t i = 0; i < minerGroup.miners.size(); i++) {
        const auto &miner = minerGroup.miners[i];
        //std::cout<<*miner << " earned:" << minerResults[i].totalProfit << " mined " << miner->getBlocksMinedTotal() <<" total, of which " << minerResults[i].blocksInWinningChain << " made it into the final chain" << std::endl;
        totalBlocks += miner->getBlocksMinedTotal();
        finalBlocks += minerResults[i].blocksInWinningChain;
    }
    Value moneyLeftAtEnd = blockchain.remFees(*winningChain[0],alpha);
    
    GameResult result(minerResults, totalBlocks, finalBlocks, moneyLeftAtEnd, totalValue,totalVariance);
    
    assert(winningBlock.valueInChain == totalValue);
    /*for (size_t i = 0; i < minerGroup.miners.size(); i++) {
        assert(minerResults[i].totalProfit <= totalValue);
    }*/
    
    
    GAMEINFO("Total blocks mined:" << totalBlocks << " with " << finalBlocks << " making it into the final chain" << std::endl);
    return result;
}
