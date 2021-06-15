main: main.cpp
	g++ -O3 -fopenmp main.cpp ./src/object.cpp ./src/_const.cpp ./src/_random.cpp -o main

test:
	# time ./main 50
	python3 show_image.py
