# -*- coding: utf-8 -*-
"""
Created on Thu Sep 17 22:51:07 2020

@author: cocot
"""

import numpy as np
from scipy.fftpack import fft
import matplotlib.pyplot as plt
from scipy.io import wavfile
import pyaudio     #sudo apt-get install python-pyaudio


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
#t_vec = np.zeros(n)
data1 = np.zeros(k)
data2 = np.zeros(k)
previous_left = np.zeros(k)
previous_right = np.zeros(k)


# store ir inside padded data
padded_ir_data_left[0:128] = ir_data_scaled[0:128, 0]
padded_ir_data_right[0:128] = ir_data_scaled[0:128, 1]


# use sine wave as input data
f = 93.75 * 4

for i in range (0,n): 
    sine[i] = 0.473 * np.cos(2*np.pi*f * i/samplerate)
    #t_vec[i] = i

#plt.plot(t_vec, sine)

# store padded input data (sine) inside data
data1[0:n] = sine[0:n]
data2[0:n] = sine[0:n]

# get spectres
ir_left = np.fft.rfft(padded_ir_data_left)
ir_right = np.fft.rfft(padded_ir_data_right) # ,k)

tmp1 = np.fft.rfft(data1)
tmp2 = np.fft.rfft(data2)


for m in range(0,2):
    conv_left = tmp1 * ir_left
    conv_right = tmp2 * ir_right
    
    current_left = np.fft.irfft(conv_left)
    current_right = np.fft.irfft(conv_right)
    
    #current_left /= k
    #current_right /= k
    
    channelLeft[0:n] = current_left[0:n]
    channelRight[0:n] = previous_left[0:n]
    channelLeft[0:k-n] = current_left[n:k]
    channelRight[0:k-n] = previous_right[n:k]
    
    previous_left = current_left
    previous_right = current_right


# play audio
#PyAudio = pyaudio.PyAudio     #initialize pyaudio

#channelOut = np.zero

#p = PyAudio()
#stream = p.open(format = p.get_format_from_width(1), 
#                channels = 1, 
#                rate = samplerate, 
#                output = True)

#stream.write(channelLeft)
#stream.stop_stream()
#stream.close()
#p.terminate()
