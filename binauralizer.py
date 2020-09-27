# -*- coding: utf-8 -*-
"""
Created on Thu Sep 17 22:51:07 2020

@author: cocot
"""

import numpy as np
import scipy.fft
import matplotlib.pyplot as plt
from scipy.io import wavfile

# block length
n = 512

# load IR file
samplerate, ir_data = wavfile.read('G:/CSN/VAE/IRs/KU100/KU100_0.wav')

# use same scaling as Juce framewokr
ir_data_scaled = ir_data / 32768

# impulse length
m = len(ir_data)

if (m+n-1) % 2:
    k = m+n
else:
    k = m+n-1


# buffers (according to Binauralization code)
channelLeft = np.zeros(n)
channelRight = np.zeros(n)
padded_ir_data_left = np.zeros(k)
padded_ir_data_right = np.zeros(k)
sine = np.zeros(n)
t_vec = np.zeros(n)
data1 = np.zeros(k)
data2 = np.zeros(k)
previous_left = np.zeros(k)
previous_right = np.zeros(k)


# store ir inside padded data
padded_ir_data_left[0:128] = ir_data_scaled[0:128, 0]
padded_ir_data_right[0:128] = ir_data_scaled[0:128, 1]


# use sine wave as input data
f = 375         # set f to n * 93.75 to have a complete wave inside sine

for i in range (0,n): 
    sine[i] = 0.473 * np.cos(2*np.pi*f * i/samplerate)
    t_vec[i] = i

#plt.plot(t_vec, sine)

# store padded input data (sine) inside data
data1[0:n] = sine[0:n]
data2[0:n] = sine[0:n]

# get spectres
ir_left = scipy.fft.fft(padded_ir_data_left)
ir_right = scipy.fft.fft(padded_ir_data_right)


tmp1 = scipy.fft.fft(data1)
tmp2 = scipy.fft.fft(data2)


for m in range(0,2):  
    conv_left = tmp1 * ir_left
    conv_right = tmp2 * ir_right
    
    current_left = np.real(scipy.fft.ifft(conv_left))
    current_right = np.real(scipy.fft.ifft(conv_right))
      
    # normalization after ifft is not required here!

    channelLeft[0:n] = current_left[0:n]
    channelRight[0:n] = current_right[0:n]
    
    for i in range (0, k-n):
        channelLeft[i] += previous_left[i+n]
        channelRight[i] += previous_right[i+n]

    previous_left = current_left
    previous_right = current_right


fig, ax = plt.subplots()
plt.plot(t_vec, channelLeft)
plt.plot(t_vec, channelRight)
plt.grid()
plt.show()

fig2, ax2 = plt.subplots()
plt.plot(t_vec, current_left[0:n])
plt.plot(t_vec, current_right[0:n])
plt.grid()
plt.show()

fig3, ax3 = plt.subplots()
plt.plot(t_vec[0:k-n], previous_left[n:k])
plt.plot(t_vec[0:k-n], previous_right[n:k])
plt.grid()
plt.show()

fig4, ax4 = plt.subplots()
plt.plot(t_vec[0:k-n], channelLeft[0:k-n])
plt.plot(t_vec[0:k-n], channelRight[0:k-n])
plt.title("output 0 to k-n")
plt.grid()
plt.show()


#N = 320
#T = 1 / 48000
#pf = np.linspace(0.0, 1.0/(2.0*T), N/2)

# use numpy conv



#fig, ax = plt.subplots()
#ax.plot(plt_freq, 2.0/(N+1) * np.abs(current_left))
#plt.show()

