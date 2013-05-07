# resisze images
import os
import sys
import glob
from PIL import Image
from argparse import ArgumentParser
from pymongo import MongoClient


def resize_images():
  imgs = glob.glob("images/*.jpg")
  for img in imgs:
    large = Image.open(img)
    small = large.resize((48, 48))
    smallimg = img.replace("images/", "").replace(".jpg", "small.jpg")
    small.save("smallImages/" + smallimg)

#resize_images()

def add_smallpath_to_db():
  client = MongoClient();
  db = client.instagram_photomosaic
  image_pool = db.image_pool
  pool = image_pool.find()
  count = 0
  for img in pool:
    #img["smallsrc"] = img["imgsrc"].replace("images", "smallImages").replace(".jpg", "small.jpg")
    print img
    count += 1
  print count

add_smallpath_to_db()
