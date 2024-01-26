//
//  function_fork_miner.hpp
//  BlockSim
//
//  Created by Harry Kalodner on 5/25/16.
//  Copyright © 2016 Harry Kalodner. All rights reserved.
//

#ifndef function_fork_miner_hpp
#define function_fork_miner_hpp

#include "typeDefs.hpp"

#include <functional>
#include <memory>
#include <map>

class Strategy;
class Blockchain;
class Block;

using ForkFunc = std::function<Value(const Blockchain &, const Block&,  Value)>;

std::unique_ptr<Strategy> createFunctionForkStrategy(bool atomic, ForkFunc f, std::string type);

std::map<int,Value,std::greater<int>> functionForkValueInMinedChild(const Blockchain &blockchain, const Block &block, ForkFunc f, Alpha alpha);

Value functionForkValPercentage(const Blockchain &blockchain, const Block &block, Alpha maxVal, double funcCoeff);
Value functionForkNumPercentage(const Blockchain &blockchain, const Block &block, Alpha maxVal, double funcCoeff);

//Value functionForkLambert(const Blockchain &blockchain, Value maxVal, double lambertCoeff);

#endif /* function_fork_miner_hpp */
