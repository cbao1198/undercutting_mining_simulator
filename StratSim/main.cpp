//
//  main.cpp
//  BlockSim
//
//  Created by Harry Kalodner on 6/6/16.
//  Copyright © 2016 Harry Kalodner. All rights reserved.
//


#include "BlockSim/block.hpp"
#include "BlockSim/blockchain.hpp"
#include "BlockSim/minerStrategies.h"
#include "BlockSim/logging.h"
#include "BlockSim/game.hpp"
#include "BlockSim/minerGroup.hpp"
#include "BlockSim/miner_result.hpp"
#include "BlockSim/game_result.hpp"
#include "BlockSim/typeDefs.hpp"
#include "BlockSim/miner.hpp"

#include "multiplicative_weights_learning_model.hpp"
#include "exp3_learning_model.hpp"
#include "learning_strategy.hpp"

#include <iostream>
#include <math.h>

//--more representative of smaller miners, where the chance that you mine the
//next block is ~0 (not to be confused with the strategy selfish mining)

#define ATOMIC false     //not realistic, but do not force miners to mine on top of their own blocks
#define NOISE_IN_TRANSACTIONS false //miners don't put the max value they can into a block (simulate tx latency)

#define NETWORK_DELAY BlockTime(0)         //network delay in seconds for network to hear about your block
#define EXPECTED_NUMBER_OF_BLOCKS BlockCount(10000)

#define LAMBERT_COEFF 0.13533528323661//coeff for lambert func equil  must be in [0,.2]
//0.13533528323661 = 1/(e^2)

#define SEC_PER_BLOCK BlockRate(600)     //mean time in seconds to find a block

#define B BlockValue(0 * SATOSHI_PER_BITCOIN) // Block reward
//#define A BlockValue(50 * SATOSHI_PER_BITCOIN)/SEC_PER_BLOCK  //rate transactions come in

#define A BlockValue(50 * SATOSHI_PER_BITCOIN) // transactions that come in every round

struct RunSettings {
    unsigned int numberOfGames;
    MinerCount totalMiners;
    MinerCount fixedDefault;
    GameSettings gameSettings;
    std::string folderPrefix;
    Alpha alpha = -1;
};

void runStratGame(RunSettings settings, std::vector<std::unique_ptr<LearningStrategy>> &learningStrategies, std::unique_ptr<Strategy> defaultStrategy);

void runSingleStratGame(RunSettings settings);

void runStratGame(RunSettings settings, std::vector<std::unique_ptr<LearningStrategy>> &learningStrategies, std::unique_ptr<Strategy> defaultStrategy) {
    
    //start running games
    BlockCount totalBlocksMined(0);
    BlockCount blocksInLongestChain(0);
    
    std::string resultFolder = "../miningResults/";
    
    if (settings.folderPrefix.length() > 0) {
        resultFolder += settings.folderPrefix + "-";
    }
    
    resultFolder += std::to_string(rawCount(settings.fixedDefault))+std::to_string(settings.alpha);
    
    std::vector<std::unique_ptr<Miner>> miners;
    std::vector<Miner *> learningMiners;
    
    HashRate hashRate(1.0/rawCount(settings.totalMiners));
    MinerCount numberRandomMiners(settings.totalMiners - settings.fixedDefault);
    
    for (MinerCount i(0); i < settings.totalMiners; i++) {
        auto minerName = std::to_string(rawCount(i));
        MinerParameters parameters {rawCount(i), minerName, hashRate, NETWORK_DELAY, COST_PER_SEC_TO_MINE};
        miners.push_back(std::make_unique<Miner>(parameters, *defaultStrategy));
        if (i < numberRandomMiners) {
            learningMiners.push_back(miners.back().get());
        }
    }
    
//    LearningModel *learningModel = new MultiplicativeWeightsLearningModel(learningStrategies, learningMiners.size(), resultFolder);
    LearningModel *learningModel = new Exp3LearningModel(learningStrategies, learningMiners.size(), resultFolder);

    MinerGroup minerGroup(std::move(miners));
    
//    double phi = std::sqrt(strategies.size() * std::log(strategies.size())) / std::sqrt(settings.numberOfGames);
//    double phi = std::sqrt(strategies.size() * std::log(strategies.size())) / std::sqrt(settings.numberOfGames / 100);
    
    double phi = .05;
    Value totalVariance = Value(0);
    Value totalValue =Value(0);
    auto blockchain = std::make_unique<Blockchain>(settings.gameSettings.blockchainSettings);
    for (unsigned int gameNum = 0; gameNum < settings.numberOfGames; gameNum++) {
//        double n = gameNum;
//        double nMax = settings.numberOfGames;
//        double phi = std::pow(.9, (n / nMax) * 30.0);
        
        blockchain->reset(settings.gameSettings.blockchainSettings);
        
        learningModel->writeWeights(gameNum);
        minerGroup.reset(*blockchain);
        learningModel->pickNewStrategies(phi, learningMiners, *blockchain);
        minerGroup.resetOrder();
        
        GAMEINFO("\n\nGame#: " << gameNum << " The board is set, the pieces are in motion..." << std::endl);
        
        auto result = runGame(minerGroup, *blockchain, settings.gameSettings, settings.alpha);
        
        GAMEINFO("The game is complete. Calculate the scores:" << std::endl);
        
        Value maxProfit = (A * (EXPECTED_NUMBER_OF_BLOCKS * settings.gameSettings.blockchainSettings.secondsPerBlock) - result.moneyLeftAtEnd) / Value(rawCount(settings.totalMiners) / 4);
        //Value maxProfit = (29*A/20*(EXPECTED_NUMBER_OF_BLOCKS) - result.moneyLeftAtEnd) / Value(rawCount(settings.totalMiners) / 4);
        // Steps 3, 4, 5
        learningModel->updateWeights(result, maxProfit, phi);
        
        totalBlocksMined += result.totalBlocksMined;
        blocksInLongestChain += result.blocksInLongestChain;
        totalVariance += result.totalVariance;
        totalValue += result.moneyInLongestChain;

//        BlockCount staleBlocks(result.totalBlocksMined - result.blocksInLongestChain);
//        std::cout << result.moneyInLongestChain << " in chain and " <<
//        result.moneyLeftAtEnd << " left with " << 100 * double(rawCount(staleBlocks)) / double(rawCount(result.totalBlocksMined)) << "% orphan rate" <<  std::endl;
        
//        std::cout << totalBlocksMined << " total blocks mined" << std::endl;
    }
    learningModel->writeWeights(settings.numberOfGames);
    learningModel->writeAvgWeights(settings.numberOfGames);

    learningModel->printWeights();
    learningModel->writeWeightSpreadAndVariance(1.0*totalVariance/(double(totalValue)));
    
    delete learningModel;
    
}

void runSingleStratGame(RunSettings settings) {
    using std::placeholders::_1;
    using std::placeholders::_2;
    using std::placeholders::_3;
    
    std::vector<std::unique_ptr<LearningStrategy>> learningStrategies;
    
    StratWeight defaultWeight(1);
    
    std::unique_ptr<Strategy> defaultStrategy(createDefaultStrategy(ATOMIC, NOISE_IN_TRANSACTIONS));
    
    learningStrategies.push_back(std::make_unique<LearningStrategy>(createPettyStrategy(ATOMIC, NOISE_IN_TRANSACTIONS), defaultWeight));
    for (int i = 1; i < 8; i++) {
        int funcCoeff = static_cast<int>(pow(2, (i + 1)));
        std::function<Value(const Blockchain &, const Block&, Alpha)> forkFunc(std::bind(functionForkValPercentage, _1, _2, _3, funcCoeff));
        learningStrategies.push_back(std::make_unique<LearningStrategy>(createFunctionForkStrategy(ATOMIC, forkFunc, std::to_string(funcCoeff)), defaultWeight));
    }
//  strategies.push_back(createFunctionForkStrategy(NO_SELF_MINING, std::bind(functionForkLambert, _1, _2, LAMBERT_COEFF), "lambert"));
    learningStrategies.push_back(std::make_unique<LearningStrategy>(createLazyForkStrategy(ATOMIC), defaultWeight));
    
    
    runStratGame(settings, learningStrategies, std::move(defaultStrategy));
}

int main(int, const char * []) {

    //alpha = block size limit
    Alpha alpha = Alpha(8000000000000);
    alpha = Alpha(-1);
    std::map<int,Value,std::greater<int>> inputRates;
    inputRates[1] = A;
    BlockchainSettings blockchainSettings = {SEC_PER_BLOCK, inputRates, B, EXPECTED_NUMBER_OF_BLOCKS, alpha};
    GameSettings gameSettings = {blockchainSettings};
    for(int j = 0; j <= 20; j++){
    // vary number of default
        for (long long i = 10; i <= 80; i=i+5){
            //vary alpha
            alpha = Value(100000000*i);
            std::cout<<"WITH ALPHA: "<<alpha<<std::endl;
            RunSettings runSettings = {80000, MinerCount(20), MinerCount(j), gameSettings, "clean-atomic-test", alpha};
            runSingleStratGame(runSettings);
        }
    }
}
