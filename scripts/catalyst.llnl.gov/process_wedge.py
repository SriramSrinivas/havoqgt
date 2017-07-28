#!/usr/bin/python

st=-1
en=-1
cnt=0
cnt2=0
ans=[]
fout=open("/p/lscratchf/zhang50/wedge_out_2", "w")
with open("/p/lscratchf/zhang50/wedge", "r") as f:
	for line in f:
		w=line.split()
		if long(w[0])!=st:
			#ans.append(str(st)+" "+str(cnt))	
			fout.write(str(st)+" "+str(cnt)+"\n")
			st=long(w[0])
			cnt=1
		else:
			cnt=cnt+1
		if long(w[0])!=st or long(w[1])!=en:
			en=long(w[1])
			cnt2=cnt2+1
						
fout.write(str(cnt2)+"\n")
