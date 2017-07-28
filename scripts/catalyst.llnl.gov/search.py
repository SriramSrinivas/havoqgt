#!/usr/bin/python
import os
import sys
import glob
import subprocess

#for file in glob.iglob('/p/lscratchf/havoqgtu/WebDataCommons/2012/nodes/index-000*'):
fout = open(sys.argv[1], 'w')
print sys.argv[1]
flag=0
query='\''
for arg in sys.argv[2:]:
	if flag==1:
		query=query+"|"
	else:
		flag=1
	query = query +arg+"$"
query = query + "\'" 
ans=['' for i in range(len(sys.argv)-1)]
for i in range(83):
	file = '/p/lscratchf/havoqgtu/WebDataCommons/2012/nodes/index-000'+'{0:02d}'.format(i)
	#p = subprocess.Popen(['wc', '-l', file], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
	#p = subprocess.Popen(['tail', '-n','1',  file], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
	print file + " " + query
	p = subprocess.Popen(['egrep', query, file], stdout=subprocess.PIPE, stderr=subprocess.PIPE)

	result, err = p.communicate()
	print result
	if result=='':
		 continue
	for st in result.splitlines():
		print st
		w=st.split('\t')
		print w[1]
		cnt=0
		for i in sys.argv[2:]:
			cnt=cnt+1
			if long(i)==long(w[1]):
				print str(cnt)
				ans[cnt-1]=w[0]
				 
for i in range(len(sys.argv)-1):
	fout.write(ans[i]+"\n")
fout.close()
#	with open(file, 'r') as f:
#		print file
#		for line in f:
#			w = line.split("\t")
#			for arg in sys.argv[2:]:
#				if long(w[1])==long(arg):
#					print line

