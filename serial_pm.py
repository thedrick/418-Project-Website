import os
import sys 
import Image
from argparse import ArgumentParser
from imageSlicer import ImageSlicer


if __name__ == '__main__':
	desc = """Serial version of a photo mosaic creation program
	using Instagram photos as inputs and outputs."""
	parser = ArgumentParser(description=desc)
	parser.add_argument("image_path", metavar="PATH", type=str, nargs=1,
						help="path to the input image")
	args = parser.parse_args()
	print args.image_path