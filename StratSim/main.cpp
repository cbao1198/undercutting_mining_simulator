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
#include <fstream>
#include <sstream>
#include <cassert>

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
#define A BlockValue(50 * SATOSHI_PER_BITCOIN)

struct RunSettings {
    unsigned int numberOfGames;
    MinerCount totalMiners;
    MinerCount fixedDefault;
    MinerCount fixedPetty;
    MinerCount fixedFractional;
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
    
    resultFolder += std::to_string(rawCount(settings.fixedDefault))+"-"+std::to_string(rawCount(settings.fixedPetty))+"-"+std::to_string(rawCount(settings.fixedFractional))+"-"+std::to_string(settings.alpha);
    
    std::vector<std::unique_ptr<Miner>> miners;
    std::vector<Miner *> learningMiners;
    
    HashRate hashRate(1.0/rawCount(settings.totalMiners));
    MinerCount numberRandomMiners(settings.totalMiners - settings.fixedDefault - settings.fixedPetty);
    auto pettyStrategy = createPettyStrategy(ATOMIC, NOISE_IN_TRANSACTIONS);
    auto fractionalStrategy = createFractionalHonestStrategy(ATOMIC);
    for (MinerCount i(0); i < settings.totalMiners; i++) {
        auto minerName = std::to_string(rawCount(i));
        MinerParameters parameters {rawCount(i), minerName, hashRate, NETWORK_DELAY, COST_PER_SEC_TO_MINE};
        //miners.push_back(std::make_unique<Miner>(parameters, *defaultStrategy));
	//std::cout<<"fixed fractional: "<<settings.fixedFractional<<std::endl;
	if(i >= numberRandomMiners && i < numberRandomMiners + settings.fixedPetty){
	   miners.push_back(std::make_unique<Miner>(parameters, *pettyStrategy));
	}
	else {
	   miners.push_back(std::make_unique<Miner>(parameters, *defaultStrategy));
	}
        if (i < numberRandomMiners) {
	    if(i < settings.fixedFractional) {
		miners.pop_back();
		miners.push_back(std::make_unique<Miner>(parameters,*fractionalStrategy));
	    }
	    else{
            learningMiners.push_back(miners.back().get());
	    }
        }
    }
    
//    LearningModel *learningModel = new MultiplicativeWeightsLearningModel(learningStrategies, learningMiners.size(), resultFolder);
    LearningModel *learningModel = new Exp3LearningModel(learningStrategies, learningMiners.size(), resultFolder);
    MinerGroup minerGroup(std::move(miners));
    
//    double phi = std::sqrt(strategies.size() * std::log(strategies.size())) / std::sqrt(settings.numberOfGames);
//    double phi = std::sqrt(strategies.size() * std::log(strategies.size())) / std::sqrt(settings.numberOfGames / 100);
    
    double phi = .15;
    Value totalVariance = Value(0);
    Value totalValue =Value(0);
    std::stringstream ss;
        ss << resultFolder << "/chain_results.csv";
   auto	    overallOutputStream = std::ofstream(ss.str());
    auto blockchain = std::make_unique<Blockchain>(settings.gameSettings.blockchainSettings);
    auto undercutPercentage = 0.0;
    auto forkAvgPercentage = 0.0;
    auto defaultAvgPercentage = 0.0;
    auto pettyAvgPercentage = 0.0;
    auto fractionalAvgPercentage = 0.0;
    auto avgChainLength = 0.0;
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
        // Steps 3, 4, 5
        //learningModel->updateWeights(result, maxProfit, phi);
        
        totalBlocksMined += result.totalBlocksMined;
        blocksInLongestChain += result.blocksInLongestChain;
        totalVariance += result.totalVariance;
        totalValue += result.moneyInLongestChain;
	 auto minerResults = result.minerResults;
        maxProfit = A*EXPECTED_NUMBER_OF_BLOCKS*settings.gameSettings.blockchainSettings.secondsPerBlock/rawCount(settings.totalMiners);
   auto totalUndercutPercentage = 0.0;
  auto totalPettyPercentage = 0.0;
  auto totalFractionalPercentage = 0.0;
 auto totalBlocks = 0.0; 
    for (size_t i = 0; i < minerGroup.miners.size(); i++) {
        const auto &miner = minerGroup.miners[i];

        std::cout<<*miner << " earned:" << 1.0*minerResults[i].totalProfit/result.moneyInLongestChain << " mined " << miner->getBlocksMinedTotal() <<" total, of which " << minerResults[i].blocksInWinningChain << " made it into the final chain" << std::endl;
        overallOutputStream<<*miner << " earned:" <<1.0*minerResults[i].totalProfit/result.moneyInLongestChain<< " mined " << miner->getBlocksMinedTotal() <<" total, of which " << minerResults[i].blocksInWinningChain << " made it into the final chain" << std::endl;
	if(minerResults[i].totalProfit > maxProfit){
		maxProfit = minerResults[i].totalProfit;
		//winner = miner;
	}
	//std::cout<<miner->isFork()<<std::endl;
	if(miner->isFork()){
	   totalUndercutPercentage += 1.0*minerResults[i].totalProfit/result.moneyInLongestChain;
	}
	if(miner->isPetty()){
	   totalPettyPercentage += 1.0*minerResults[i].totalProfit/result.moneyInLongestChain;
	}
	if(miner->isFractional()){
	   totalFractionalPercentage += 1.0*minerResults[i].totalProfit/result.moneyInLongestChain;
	}
	totalBlocks += minerResults[i].blocksInWinningChain;
    }
    auto winningChain = result.winningChain;
    int numUndercut = 0;
    for (auto mined : winningChain) {
	    auto& miner = mined->miner;
    if(mined->profitWeight == 1){
	                overallOutputStream<<"undercut at height "<<mined->height<<std::endl;
			numUndercut += 1;
			std::cout<<"undercut at height: "<<mined->height<<std::endl;
		}    
    if(mined->height > BlockHeight(1000)){
		
			continue;
			}
	if (mined->height == BlockHeight(0)) {
            break;
        }
    	overallOutputStream<<"block "<<mined->height<<": ";
	overallOutputStream<<"mined by "<<*miner<<" with value "<<mined->value<<std::endl;
    }
    overallOutputStream<<"total lazy fork percentage: "<<totalUndercutPercentage<<std::endl;
    overallOutputStream<<"total petty percentage: "<<totalPettyPercentage<<std::endl;
    overallOutputStream<<"total fractional percentage: "<<totalFractionalPercentage<<std::endl;
    overallOutputStream<<"total default honest percentage: "<<1.0-totalUndercutPercentage-totalPettyPercentage-totalFractionalPercentage<<std::endl;
    std::cout<<"total lazy fork percentage: "<<totalUndercutPercentage<<std::endl;
    std::cout<<"num undercut: "<<numUndercut<<std::endl;
    std::cout<<"total petty percentage: "<<totalPettyPercentage<<std::endl;
    std::cout<<"total fractional percentage: "<<totalFractionalPercentage<<std::endl;
    std::cout<<"total default honest percentage: "<<1.0-totalUndercutPercentage-totalPettyPercentage-totalFractionalPercentage<<std::endl;
    overallOutputStream<<"percent undercut: "<<1.0*numUndercut/EXPECTED_NUMBER_OF_BLOCKS<<std::endl;
    overallOutputStream<<"num undercut: "<<numUndercut<<std::endl;
    std::cout<<numUndercut<<" "<<EXPECTED_NUMBER_OF_BLOCKS<<std::endl;
    overallOutputStream<<"longest chain length: "<<totalBlocks<<std::endl;
    //assert(result.moneyInLongestChain+result.moneyLeftAtEnd == (EXPECTED_NUMBER_OF_BLOCKS*3*A/5 + 3*A));
    overallOutputStream<<"value fraction: "<<1.0*result.moneyInLongestChain/(result.moneyInLongestChain+result.moneyLeftAtEnd)<<std::endl;
    overallOutputStream<<"left over: "<<result.moneyLeftAtEnd<<std::endl;
    std::cout<<"longest chain length: "<<totalBlocks<<std::endl;
    std::cout<<"percent undercut: "<<1.0*numUndercut/EXPECTED_NUMBER_OF_BLOCKS<<std::endl;
    undercutPercentage += 1.0*numUndercut/EXPECTED_NUMBER_OF_BLOCKS;
    forkAvgPercentage += totalUndercutPercentage/(settings.totalMiners-settings.fixedPetty-settings.fixedDefault-settings.fixedFractional);
    if(settings.fixedPetty > 0){
    pettyAvgPercentage += totalPettyPercentage/(settings.fixedPetty);
    }
    if(settings.fixedDefault > 0){
    defaultAvgPercentage += (1.0-totalUndercutPercentage-totalPettyPercentage-totalFractionalPercentage)/settings.fixedDefault;
    }
    if(settings.fixedFractional > 0){
    fractionalAvgPercentage += totalFractionalPercentage/settings.fixedFractional;
    }

    avgChainLength += totalBlocks;
    }
    learningModel->writeWeights(settings.numberOfGames);

    learningModel->printWeights();
    std::cout<<totalVariance<<" "<<totalValue<<std::endl;
    learningModel->writeWeightSpreadAndVariance(1.0*totalVariance/(double(totalValue)));
    std::cout<<"variance: "<< 1.0*totalVariance/(double(totalValue))<<std::endl;  
    std::cout<<"raw variance: "<<totalVariance<<" and total val: "<<totalValue<<std::endl;
    overallOutputStream<<"variance: "<< 1.0*totalVariance/(double(totalValue))<<std::endl;
    std::cout<<"avg percentage undercut: "<<undercutPercentage/settings.numberOfGames<<std::endl;
    std::cout<<"avg fork profit per miner: "<<forkAvgPercentage/settings.numberOfGames<<std::endl;
    std::cout<<"avg petty honest profit per miner: "<<pettyAvgPercentage/settings.numberOfGames<<std::endl;
    std::cout<<"avg default profit per miner: "<<defaultAvgPercentage/settings.numberOfGames<<std::endl;
    std::cout<<"avg fractional profit per miner: "<<fractionalAvgPercentage/settings.numberOfGames<<std::endl;
    std::cout<<"avg chain length: "<<avgChainLength/settings.numberOfGames<<std::endl;
    overallOutputStream<<"avg percentage undercut: "<<undercutPercentage/settings.numberOfGames<<std::endl;
    overallOutputStream<<"avg fork profit per miner: "<<forkAvgPercentage/settings.numberOfGames<<std::endl;
    overallOutputStream<<"avg petty honest profit per miner: "<<pettyAvgPercentage/settings.numberOfGames<<std::endl;
    overallOutputStream<<"avg fractional profit per miner: "<<fractionalAvgPercentage/settings.numberOfGames<<std::endl;
    overallOutputStream<<"avg default profit per miner: "<<defaultAvgPercentage/settings.numberOfGames<<std::endl;
    overallOutputStream<<"avg chain length: "<<avgChainLength/settings.numberOfGames<<std::endl;
    delete learningModel;
    
    /*GAMEINFOBLOCK(
                  GAMEINFO("Games over. Final strategy weights:\n");
                  learningModel->printWeights();
                  )*/
    
}

void runSingleStratGame(RunSettings settings) {
    using std::placeholders::_1;
    using std::placeholders::_2;
    using std::placeholders::_3;
    
    std::vector<std::unique_ptr<LearningStrategy>> learningStrategies;
    
    StratWeight defaultWeight(1);
    
    std::unique_ptr<Strategy> defaultStrategy(createDefaultStrategy(ATOMIC, NOISE_IN_TRANSACTIONS));
    
    /*learningStrategies.push_back(std::make_unique<LearningStrategy>(createPettyStrategy(ATOMIC, NOISE_IN_TRANSACTIONS), defaultWeight));
    for (int i = 1; i < 8; i++) {
        int funcCoeff = static_cast<int>(pow(2, (i + 1)));
        std::function<Value(const Blockchain &, const Block&, Alpha)> forkFunc(std::bind(functionForkValPercentage, _1, _2, _3, funcCoeff));
        learningStrategies.push_back(std::make_unique<LearningStrategy>(createFunctionForkStrategy(ATOMIC, forkFunc, std::to_string(funcCoeff)), defaultWeight));
    }
//  strategies.push_back(createFunctionForkStrategy(NO_SELF_MINING, std::bind(functionForkLambert, _1, _2, LAMBERT_COEFF), "lambert"));*/
    learningStrategies.push_back(std::make_unique<LearningStrategy>(createLazyForkStrategy(ATOMIC), defaultWeight));
    //learningStrategies.push_back(std::make_unique<LearningStrategy>(createFractionalHonestStrategy(ATOMIC),defaultWeight)); 
    
    runStratGame(settings, learningStrategies, std::move(defaultStrategy));
}

int main(int, const char * []) {

    Alpha alpha = Alpha(8000000000000);
    //alpha = Alpha(100);
    //Value alpha = Value(100000000000);
    alpha = Alpha(-1);
    std::map<int,Value,std::greater<int>> inputRates;
    //inputRates[1] = 19*A/20;
    //inputRates[10] = A/20;
    
    inputRates[1] = A;

    //inputRates[1] = 3*A/5;
    //inputRates[1] = 2*A/5;
    //inputRates[3] = A/5;
    //inputRates[5] = 3*A/25;
    //
    
    //actual tx from mempool; feb 27
    /*inputRates[10] = 300;
    inputRates[30] = 30;
    inputRates[50] = 20;
    inputRates[70] = 1;
    inputRates[90] = 2;
    inputRates[110] = 2;
    inputRates[130] = 2;*/
    for(int j = 0; j <= 19; j++){
    //for(int j = 19; j >= 0; j--){
    for(int k = 0; k <= 19-j; k++){
    //for(int j = 0; j <=20; j++){
    for (long long i = 10; i <= 80; i=i+5){
    //for (long long i = 70; i <= 80; i=i+10){
    BlockchainSettings blockchainSettings = {SEC_PER_BLOCK, inputRates, B, EXPECTED_NUMBER_OF_BLOCKS, alpha,2};
    GameSettings gameSettings = {blockchainSettings};
//    
//    for (MinerCount i(0); i < MinerCount(71); i += MinerCount(6)) {
//        RunSettings runSettings = {300000, MinerCount(100), i, gameSettings, "mult"};
//        runSingleStratGame(runSettings);
//    }
        alpha = Value(100000000*i);
        std::cout<<"WITH ALPHA: "<<alpha<<std::endl;
	std::cout<<"num default: "<<j<<std::endl;
	std::cout<<"num petty: "<<k<<std::endl;
	std::cout<<"num fractional: "<<numFractional<<std::endl;
	//RunSettings runSettings = {10, MinerCount(20), MinerCount(j), MinerCount(k), MinerCount(numFractional), gameSettings, "fractional-honest-ratio-50-transaction-5-5-test",alpha};
	//RunSettings runSettings = {10, MinerCount(20), MinerCount(j), MinerCount(0),MinerCount(numFractional),gameSettings, "different-ratio-50-fractional-45-num-blocks-10thousand-transaction-15-3-test",alpha};
	//RunSettings runSettings = {20000, MinerCount(20), MinerCount(j), MinerCount(0),MinerCount(numFractional),gameSettings, "learning-different-ratio-50-fractional-25-transaction-15-3-test",alpha};
	//RunSettings runSettings = {10, MinerCount(20), MinerCount(j), MinerCount(0),MinerCount(20),gameSettings, "different-ratio-90-transaction-15-3-10-blocks-test",alpha};
	//RunSettings runSettings = {1, MinerCount(20), MinerCount(j), MinerCount(0),MinerCount(10),gameSettings, "super-small-0010600000-vs-50-1000-block-test",alpha};

        //RunSettings runSettings = {10, MinerCount(20), MinerCount(j), MinerCount(k),gameSettings, "new-mining-default-petty-5-3-multiple-game-avg-test", alpha};
        RunSettings runSettings = {10, MinerCount(20), MinerCount(j), MinerCount(k),MinerCount(0),gameSettings, "new-mining-default-petty-5-5-try-2-test", alpha};
        //RunSettings runSettings = {10, MinerCount(20), MinerCount(j), MinerCount(k),gameSettings, "new-mining-default-petty-10-3-1-2-total-5-multiple-game-avg-test", alpha};
        //RunSettings runSettings = {1, MinerCount(20), MinerCount(j), MinerCount(1),MinerCount(10),gameSettings, "petty-honest-only-every-10-fractional-10-lazy-50-transaction-15-3-test", alpha};
        //RunSettings runSettings = {1, MinerCount(20), MinerCount(j), MinerCount(1),MinerCount(0),gameSettings, "tmp-test", alpha};
	//RunSettings runSettings = {10, MinerCount(20), MinerCount(j), MinerCount(k),MinerCount(0),gameSettings, "actual-data-test-feb-27-test",alpha};
        //RunSettings runSettings = {10, MinerCount(20), MinerCount(j), MinerCount(k), MinerCount(0), gameSettings, "new-mining-default-petty-every-2-gradient-3-1-1-2-total-5-start-5-1-multiple-game-avg-test", alpha};
        //RunSettings runSettings = {10, MinerCount(20), MinerCount(j), MinerCount(k),gameSettings, "new-mining-default-petty-5-3-1-2-total-5-multiple-game-avg-test", alpha};
	/*j = 0;
	k = 0;
        RunSettings runSettings = {1, MinerCount(20), MinerCount(j), MinerCount(k),gameSettings, "tmp-test", alpha};*/
        runSingleStratGame(runSettings);
//	break;
    }
    //break;
    }
    //break;
    }
}
