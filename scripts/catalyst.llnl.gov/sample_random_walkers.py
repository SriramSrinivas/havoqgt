"""
Module for seeding random walkers in time to detect big changes
in scores for nodes
"""
import sys
import numpy as np
#from subprocess import call
import os
import compute_scores

''' get params
delta is the amount of change of score (absolute)
start_time, end_time are Unix timestamps corresponding to [0,T]
tau is a length of time
eps is an error tolerance
'''
if len(sys.argv) == 5:
    delta, start_time, end_time, tau = map(float, sys.argv[1:])
    eps = 0.1
elif len(sys.argv) == 6:
    delta, start_time, end_time, tau, eps = map(float, sys.argv[1:])

# sample R2 times
T = end_time-start_time
r2 = 2.0*(T/tau)*(eps**(-2))*np.log(eps**(-1))
print("sampling", str(r2), "times")

# simulate R1 random walkers
r1 = int((12/(eps**2*delta))*np.log(eps**(-1)))
print("simulating", str(r1), "random walkers")

os.chdir(os.path.dirname('/g/g17/osimpson/evolving/havoqgt/build/catalyst.llnl.gov/'))
os.system( 'srun -N32 --ntasks-per-node=24 --distribution=block ./src/run_random_walk_simulation -i /l/ssd/output -m /l/ssd/metadata -c 10000 -n ' + str(r1) + ' -a brad_pitt_id.txt -s ' + str(start_time) + ' -e ' + str(end_time) + ' -r /p/lscratchf/osimpson/output -d 15 -t ' + str(r2) )

compute_scores.main()
