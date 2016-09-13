"""
Module for computing PPR scores based on end-point algo
"""
import pickle as pck
import glob

OUT_BASE = '/g/g17/osimpson/evolving/havoqgt/build/catalyst.llnl.gov/'
    
'''output format
end vx - start vx - rw id - time (X4) - _ - _ - _ - rw length
'''

out_files = glob.glob(OUT_BASE+'output*')
keys = {}
for f in out_files:
    if f.split('_')[-1] in keys:
        keys[f.split('_')[-1]].append(f)
    else:
        keys[f.split('_')[-1]] = [f]

def main():
    for k in keys:
        scores = {}
        time = None
        for fname in keys[k]:
            with open(fname) as f:
                for line in f:
                    rw_summary = line.strip().split(' ')
                    if not time:
                        time = rw_summary[3]
                    else:
                        if rw_summary[3] != time:
                            print("Wrong time in output", fname)
                    if rw_summary[0] in scores:
                        scores[rw_summary[0]] += 1
                    else:
                        scores[rw_summary[0]] = 1
        total = sum([scores[x] for x in scores])*1.0
        for x in scores:
            scores[x] = scores[x]/total
    
        if time:
            pck.dump( scores, open(OUT_BASE+'scores/scores_'+time+'.pck', 'wb') )

if __name__ == "main":
    main() 
