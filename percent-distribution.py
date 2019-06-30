import matplotlib.pyplot as plt
import numpy as np
f = open("con-stri.log",'r')
x = list()
percent = list()
path = list()
#path.append('b')
i = 1
for line in f.readlines():
	li = line.split(',')
	x.append(i)
	i = i+1
	percent.append(float(li[0]))
	if int(li[1])==1:
		path.append('b')
	else:
		path.append('r')
#path = path[:-1]
avg = np.mean(percent)
perlist = sorted(percent)
print(avg)
level = perlist[int((1-avg)*(len(percent)-1))]
print(level)
y1 = list()
for p in percent:
	y1.append(level)
#plt.title("I'm a scatter diagram.") 
plt.xticks(fontsize=13)
plt.yticks(fontsize=13)
plt.xlim(xmax=512,xmin=0)
plt.ylim(ymax=0.6,ymin=0.1)
plt.xlabel("Request Stream", fontsize=15)
plt.ylabel("Percentage", fontsize=15)
plt.scatter(x,percent,s=5,c=path,marker='o')
plt.scatter(x,y1,s=3,c='y',marker='o')
plt.show()