#Author: Anant Gupta, Abhinav Gupta
#Filename: code.py
#Functions: bfs, get_background, detect_component, get_histogram, process_image
#Global Variables: NONE

from __future__ import print_function
from collections import deque
import math
import cv2

import config


class Processor(object):
    def __init__(self, image):
        self.width = config.HEIGHT
        self.height = config.WIDTH
        self.image = image
        self.num_divs = 5

	#Function Name: bfs
	#Inputs: 	self -> The class object pointer
	#			x -> x coordinate of the starting point
	#			y -> y coordinate of the starting point
	#			components -> array storing the component id of each point in the image
	#			comp -> current component id
	#			bgorno -> boolean array indicating whether each point is background or foreground
	#			count -> Number of points visited by the bfs's
	#Outputs:	count -> Updated number of points visited by the bfs's
	#			components -> updated components array
	#Logic:		This function performs a breadth-first search on the image points and assigns a component id to the
	#			connected component discovered by the search
    def bfs(self, x, y, components, comp, bgorno, count):
        d = deque()
        d.append((x, y))
        components[x][y] = comp
        while len(d) > 0:
            curr_x, curr_y = d.popleft()
            count += 1
            for new_x, new_y in [(curr_x + 1, curr_y), (curr_x, curr_y + 1), (curr_x - 1, curr_y),
                                 (curr_x, curr_y - 1)]:
                if 0 <= new_x < self.width and 0 <= new_y < self.height and bgorno[new_x][new_y] and components[new_x][new_y] is None:
                    components[new_x][new_y] = comp
                    d.append((new_x, new_y))
        return count, components

	#Function Name: get_background
	#Input: self -> The class object pointer
	#Output: bgorno -> boolean array indicating whether each point is background or foreground
	#Logic: This function checks whether the RGB values of a point lie within certain thresholds characterizing background pixels,
	#		and assigns this in the array bgorno
    def get_background(self):
        bgorno = [[False] * self.height for _ in xrange(self.width)]

        for i in xrange(self.width):
            for j in xrange(self.height):
                if self.image[i][j][0] > 1.5 * self.image[i][j][1] and self.image[i][j][0] > 1.5 * self.image[i][j][2] \
                        and self.image[i][j][0] > 20:
                    bgorno[i][j] = False
                else:
                    bgorno[i][j] = True

        return bgorno

	#Function Name: detect_component
	#Inputs: 	self -> The class object pointer
	#			bgorno -> boolean array indicating whether each point is background or foreground
	#Outputs:	components -> array storing the component id of each point in the image
	#			max_comp -> The component id of the largest component
	#			max_count -> The number of points in the largest component
	#Logic: 	This function finds all connected components in the image (a connected component is a set of neighbouring
	#			non-background points) and identifies the largest component. This largest component most likely represents
	#			the interior of the chip.
    def detect_component(self, bgorno):
        components = [[None] * self.height for _ in xrange(self.width)]
        comp = 0
        count = 0
        max_comp = 0
        max_count = 0

        for i in xrange(self.width):
            for j in xrange(self.height):
                if bgorno[i][j] and components[i][j] is None:
                    count, components = self.bfs(i, j,
                                                 components=components,
                                                 comp=comp,
                                                 bgorno=bgorno,
                                                 count=count)
                    if count > max_count:
                        max_count = count
                        max_comp = comp
                    comp += 1
                    count = 0

        return components, max_comp, max_count

	#Function Name: get_histogram
	#Inputs: 	self -> The class object pointer
	#			components -> array storing the component id of each point in the image
	#			max_comp -> The component id of the largest component
	#			max_count -> The number of points in the largest component
	#Output:	hist -> Histogram of color values of points of the largest component
	#Logic:		This function assigns a fraction of each point to its 8 nearest color buckets (2 brackets for RGB each),
	#			and returns the normalized histogram
    def get_histogram(self, components, max_comp, max_count):
        MAX_COL = 255
        hist = [[[0] * self.num_divs for _ in xrange(self.num_divs)] for __ in xrange(self.num_divs)]
        for i in xrange(self.width):
            for j in xrange(self.height):
                if components[i][j] == max_comp:
                    buckets = [float(self.image[i][j][k] * self.num_divs) / (MAX_COL + 1) for k in xrange(3)]
                    bn = []
                    br = []
                    for bucket in buckets:
                        num = int(math.floor(bucket))
                        if bucket - num < 0.5:
                            if bucket > 0.5:
                                nums = (num - 1, num)
                                ratios = (0.5 - bucket + num, 0.5 + bucket - num)
                            else:
                                nums = (0, 0)
                                ratios = (1, 0)
                        else:
                            if bucket < self.num_divs - 0.5:
                                nums = (num, num + 1)
                                ratios = (1.5 - bucket + num, bucket - num - 0.5)
                            else:
                                nums = (self.num_divs - 1, self.num_divs - 1)
                                ratios = (1, 0)
                        bn.append(nums)
                        br.append(ratios)
                    for p in xrange(2):
                        for q in xrange(2):
                            for r in xrange(2):
                                hist[bn[0][p]][bn[1][q]][bn[2][r]] += br[0][p] * br[1][q] * br[2][r]

                    self.image[i][j][0] = 0
                    self.image[i][j][1] = 0
                    self.image[i][j][2] = 0
                else:
                    self.image[i][j][0] = 255
                    self.image[i][j][1] = 255
                    self.image[i][j][2] = 255

        for i in xrange(self.num_divs):
            for j in xrange(self.num_divs):
                for k in xrange(self.num_divs):
                    hist[i][j][k] /= max_count

        return hist

	#Function Name: process_image
	#Input: self -> The class object pointer
	#Output: good_chip -> Boolean indicating whether the chip is good or bad
	#Logic: This function does the following:
	#		Identify non-background points in the image
	#		Detect the largest connected component (this is supposed to represent the chip)
	#		Compute the histogram of color values from the component
	#		Calculate the error between the histogram and the expected histogram of a good chip
	#		If the error lies below a certain threshold, return true, else return false
    def process_image(self):

        print("Detecting Background...")
        bgorno = self.get_background()

        print("Detecting Components...")
        components, max_comp, max_count = self.detect_component(bgorno)

        if max_count < 500:
            return True

        print("Creating Histogram...")

        hist = self.get_histogram(components, max_comp, max_count)

        cv2.imwrite('component.png', self.image)

        good_hist = config.GOOD_HIST

        error = 0
        for i in xrange(self.num_divs):
            for j in xrange(self.num_divs):
                for k in xrange(self.num_divs):
                    error += (hist[i][j][k] - good_hist[i][j][k]) ** 2
        print(error)

        error_threshold = 0.1

        good_chip = True

        if error < error_threshold:
            print("Good chip")
        else:
            print("Bad chip")
            good_chip = False

        return good_chip
