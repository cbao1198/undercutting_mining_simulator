//
//  game_result.hpp
//  BlockSim
//
//  Created by Harry Kalodner on 7/1/16.
//  Copyright © 2016 Harry Kalodner. All rights reserved.
//

#ifndef game_result_hpp
#define game_result_hpp

#include "typeDefs.hpp"
#include "block.hpp"

#include <vector>

struct MinerResult;
class Miner;

struct GameResult {
    std::vector<MinerResult> minerResults;
    std::vector< const Block*> winningChain;
    BlockCount totalBlocksMined;
    BlockCount blocksInLongestChain;
    Value moneyLeftAtEnd;
    Value moneyInLongestChain;
    Value totalVariance;
    
    GameResult(std::vector<MinerResult> minerResults, std::vector< const Block*> winningChain, BlockCount totalBlocksMined, BlockCount blocksInLongestChain, Value moneyLeftAtEnd, Value moneyInLongestChain, Value totalVariance);
};

#endif /* game_result_hpp */
