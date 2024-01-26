import pandas as pd
import numpy as np
import matplotlib

'''
Plot graphs of strategy weights on y axis and game num on x axis for each strategy
'''

prefix = "weighted-"
#prefix = "backlog-"
prefix  = "../miningResults/clean-atomic-"
for j in range(2,19):
    #i = 2000000001
    i = 100000000*j
    petty_honest = pd.read_csv(prefix+"test-1"+str(i)+"/index-0-petty-honest.csv")
    fork_4 = pd.read_csv(prefix+"test-1"+str(i)+"/index-1-function-fork-4.csv")
    fork_8 = pd.read_csv(prefix+"test-1"+str(i)+"/index-2-function-fork-8.csv")
    fork_16 = pd.read_csv(prefix+"test-1"+str(i)+"/index-3-function-fork-16.csv")
    fork_32 = pd.read_csv(prefix+"test-1"+str(i)+"/index-4-function-fork-32.csv")
    fork_64 = pd.read_csv(prefix+"test-1"+str(i)+"/index-5-function-fork-64.csv")
    fork_128 = pd.read_csv(prefix+"test-1"+str(i)+"/index-6-function-fork-128.csv")
    fork_256 = pd.read_csv(prefix+"test-1"+str(i)+"/index-7-function-fork-256.csv")
    lazy_fork = pd.read_csv(prefix+"test-1"+str(i)+"/index-8-lazy-fork.csv")

    df = pd.DataFrame({
        "petty honest": petty_honest["weight"],
        "fork 8": fork_8["weight"], 
        "fork 16": fork_16["weight"],
        "fork 32": fork_32["weight"],
        "fork 64": fork_64["weight"], 
        "fork 128": fork_128["weight"], 
        "fork 256": fork_256["weight"], 
        "lazy_fork": lazy_fork["weight"],
        "game_num": fork_8["game_num"]
        }
    )
    df.plot(x ='game_num', y=['petty honest','fork 8','fork 16', 'fork 32','fork 64','fork 128','fork 256','lazy_fork'], yticks=np.arange(0, 1, 0.1),kind='line').get_figure().savefig("../miningResults/with-default-10-A20-1-19A20-every 10-alpha-"+str(i)+".png")
    break