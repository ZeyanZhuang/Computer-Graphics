import matplotlib.pyplot as plt
import numpy as np
import cv2 as cv
import matplotlib as mpl
img = cv.imread("image.ppm")
cv.imwrite('result.jpg', img)


mpl.use('TKAgg')
img = plt.imread('result.jpg')
plt.imshow(img)
plt.show()
