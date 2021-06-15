import cv2 as cv
import numpy as np
import matplotlib.pyplot as plt;

T = 5
dt = 0.00005
n = int(T / dt)
x = np.linspace(0, T, n)
f = np.zeros(n)
f[0] = 1;
for i in range(n - 1):
    f[i + 1] = f[i] + f[i] * dt;
    f[i + 1] = f[i] + f[i + 1] * dt;


real = np.exp(x)
plt.figure('haha')
plt.plot(x, f)
plt.plot(x, real)
plt.show()

plt.figure('res')
plt.plot(x, np.abs(real - f))
plt.show()
