//
//  default_miner.hpp
//  BlockSim
//
//  Created by Harry Kalodner on 5/25/16.
//  Copyright © 2016 Harry Kalodner. All rights reserved.
//

#ifndef default_miner_hpp
#define default_miner_hpp

#include "typeDefs.hpp"

#include <memory>
#include <map>

class Miner;
class Block;
class Blockchain;
class Strategy;

std::unique_ptr<Strategy> createDefaultStrategy(bool atomic, bool noiseInTransactions);

std::map<int,Value,std::greater<int>> defaultValueInMinedChild(const Blockchain &blockchain, const Block &mineHere, bool noiseInTransactions, Alpha alpha);

Block &defaultBlockToMineOnAtomic(const Miner &me, const Blockchain &chain, const Alpha alpha);
Block &defaultBlockToMineOnNonAtomic(const Miner &me, const Blockchain &chain, const Alpha alpha);

#endif /* default_miner_hpp */
