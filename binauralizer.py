# -*- coding: utf-8 -*-
"""
Created on Thu Sep 17 22:51:07 2020

@author: Alexander Müller
"""

import numpy as np
import scipy.fft
import matplotlib.pyplot as plt
from scipy.io import wavfile


# data block length
n = 512

# load IR file
samplerate, ir_data = wavfile.read('G:/CSN/VAE/IRs/KU100/KU100_0.wav')
samplerate, ir_data_90 = wavfile.read('G:/CSN/VAE/IRs/KU100/KU100_90.wav')

# use same scaling as Juce framework
ir_data_scaled = ir_data / 32768
ir_data_scaled_90 = ir_data_90 / 32768

# impulse length
m = len(ir_data)

# use a power of 2 (or 3, 5) as FFT size
p = int(np.log2(m+n-1))
k = int(np.power(2,(p+1)))

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

delay_left = np.zeros(k)
delay_right = np.zeros(k)
padded_ir_data_left_90 = np.zeros(k)
padded_ir_data_right_90 = np.zeros(k)

# store ir inside padded data
padded_ir_data_left[0:128] = ir_data_scaled[0:128, 0]
padded_ir_data_right[0:128] = ir_data_scaled[0:128, 1]

padded_ir_data_left_90[0:128] = ir_data_scaled_90[0:128, 0]
padded_ir_data_right_90[0:128] = ir_data_scaled_90[0:128, 1]


delay_left[0] = 1
delay_right[50] = 1


# use sine wave as input data
f = 375         # set f to n * 93.75 to have a complete wave inside sine

for i in range (0,n): 
    sine[i] = 0.473 * np.cos(2*np.pi*f * i/samplerate)
    t_vec[i] = i

#plt.plot(t_vec, sine)

# alt input: noise

noise = np.random.normal(0,1,n)

# store padded input data (sine) inside data
#data1[0:n] = sine[0:n]
#data2[0:n] = sine[0:n]
data1[0:n] = noise[0:n]
data2[0:n] = noise[0:n]


# get spectres
hrtf_left = scipy.fft.fft(padded_ir_data_left)
hrtf_right = scipy.fft.fft(padded_ir_data_right)

hrtf_left_90 = scipy.fft.fft(padded_ir_data_left_90)
hrtf_right_90 = scipy.fft.fft(padded_ir_data_right_90)

delay_spec_left = scipy.fft.fft(delay_left)
delay_spec_right = scipy.fft.fft(delay_right)

tmp1 = scipy.fft.fft(data1)
tmp2 = scipy.fft.fft(data2)

f_vec = scipy.fft.fftshift(scipy.fft.fftfreq(data1.shape[-1]))

Fs = 48000
xf = np.linspace(0, Fs//2, k // 2)



for m in range(0,2):  
    conv_left = tmp1 * hrtf_left
    conv_right = tmp2 * hrtf_right
    
    conv_left_90 = tmp1 * hrtf_left_90
    conv_right_90 = tmp2 * hrtf_right_90
    
    #conv_left = tmp1 * delay_spec_left
    #conv_right = tmp2 * delay_spec_right
    
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


''''
figb, axb = plt.subplots()
plt.plot(t_vec, current_left[0:n])
plt.plot(t_vec, current_right[0:n])
plt.title("current result 0 to n")
plt.grid()
plt.show()

fig2, ax2 = plt.subplots()
plt.plot(t_vec[0:k-n], current_left[0:k-n])
plt.plot(t_vec[0:k-n], current_right[0:k-n])
plt.title("current result 0 to k-n")
plt.grid()
plt.show()

fig3, ax3 = plt.subplots()
plt.plot(t_vec[0:k-n], previous_left[n:k])
plt.plot(t_vec[0:k-n], previous_right[n:k])
plt.title("previous result 0 to k-n")
plt.grid()
plt.show()

fig4, ax4 = plt.subplots()
plt.plot(t_vec[0:k-n], channelLeft[0:k-n])
plt.plot(t_vec[0:k-n], channelRight[0:k-n])
plt.title("output 0 to k-n")
plt.grid()
plt.show()

fig, ax = plt.subplots()
plt.plot(t_vec, channelLeft)
plt.plot(t_vec, channelRight)
plt.title("output 0 to n")
plt.grid()
plt.show()

fig5, ax5 = plt.subplots()
plt.plot(t_vec, noise)
plt.plot(t_vec, channelLeft)
plt.title("input vs output (left)")
plt.grid()
plt.show()

fig6, ax6 = plt.subplots()
plt.plot(t_vec, sine)
plt.plot(t_vec, channelRight)
plt.title("input vs output (right)")
plt.grid()
plt.show()

'''


fig7, ax7 = plt.subplots()
plt.plot(xf, 2.0/k * np.abs(tmp1[:k//2]))
plt.plot(xf, 2.0/k * np.abs(conv_left[:k//2]))
plt.plot(xf, 2.0/k * np.abs(conv_right[:k//2]))
plt.title("conv(noise, IR0) - spectre")
plt.grid()
plt.show()

fig10, ax10 = plt.subplots()
plt.plot(xf, 2.0/k * np.abs(conv_left_90[:k//2]))
plt.plot(xf, 2.0/k * np.abs(conv_right_90[:k//2]))
plt.title("conv(noise, IR90) - spectre")
plt.grid()
plt.show()

fig8, ax8 = plt.subplots()
plt.plot(xf, 2.0/k * np.abs(hrtf_left[:k//2]))
plt.plot(xf, 2.0/k * np.abs(hrtf_right[:k//2]))
plt.title("HRTF left / right")
plt.grid()
plt.show()

fig9, ax9 = plt.subplots()
plt.plot(xf, 2.0/k * np.abs(hrtf_left_90[:k//2]))
plt.plot(xf, 2.0/k * np.abs(hrtf_right_90[:k//2]))
plt.title("HRTF left / right")
plt.grid()
plt.show()



