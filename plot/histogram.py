#!/usr/bin/python2.7
# -*- encoding: utf-8 -*-

import matplotlib.pyplot as plt
import numpy as np
import sys

if len(sys.argv) != 3:
    print 'usage: %s pcie_fifo_output lpt_fifo_output' % sys.argv[0]
    sys.exit(1)

a = np.fromfile(sys.argv[1], dtype=np.int64)
b = np.fromfile(sys.argv[2], dtype=np.int64)

l = min(len(a), len(b))
a, b = tuple(np.array(x[:l], dtype=np.float)/1.e3 for x in (a, b))

ticks = np.arange(0, 23, .5)
plt.hist(a, bins=ticks, log=True, color='g', alpha=.5, label='PCIe')
plt.hist(b, bins=ticks, log=True, color='r', alpha=.5, label='LPT')

plt.plot(2*[a.min()],[0,1e7],'g--',lw=2,alpha=.5)
plt.plot(2*[a.max()],[0,1e7],'g--',lw=2,alpha=.5)

plt.plot(2*[b.min()],[0,1e7],'r--',lw=2,alpha=.5)
plt.plot(2*[b.max()],[0,1e7],'r--',lw=2,alpha=.5)

plt.text(a.min()-1.6, 5e6, r'$%.1f \mu s$'%a.min(), color='g')
plt.text(b.min()+.1, 5e6, r'$%.1f \mu s$'%b.min(), color='r')

plt.text(a.max()+.1, 5e6, r'$%.1f \mu s$'%a.max(), color='g')
plt.text(b.max()-1.8, 5e6, r'$%.1f \mu s$'%b.max(), color='r')

plt.legend(loc='lower left')
plt.xlabel(r'Latency ($\mu s$)')
plt.ylabel(r'Number of occurrences')

plt.tight_layout()
plt.savefig('latency-hist.svg', transparent=True)
