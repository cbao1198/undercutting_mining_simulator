//
//  blockchain_settings.hpp
//  BlockSim
//
//  Created by Harry Kalodner on 10/26/16.
//  Copyright © 2016 Harry Kalodner. All rights reserved.
//

#ifndef blockchain_settings_hpp
#define blockchain_settings_hpp

#include "typeDefs.hpp"
#include <vector>
#include <map>

struct BlockchainSettings {
    BlockRate secondsPerBlock;
    std::map<int,ValueRate,std::greater<int>> transactionFeeRate;
    BlockValue blockReward;
    BlockCount numberOfBlocks;
    Alpha alpha;
    int cycle;
};

#endif /* blockchain_settings_hpp */
