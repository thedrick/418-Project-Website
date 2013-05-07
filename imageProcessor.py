import os
import sys 
import Image
from imageSlicer import ImageSlicer
from pymongo import MongoClient
import glob

def processImages():
  client = MongoClient()
  db = client.instagram_photomosaic
  image_pool = db.image_pool
  imgs = glob.glob("images/*.jpg")
  for img in imgs:
    if (len(list(image_pool.find({"imgsrc" : img}))) != 0):
      continue
    slicer = ImageSlicer(img, 1)
    averages = slicer.get_averages()
    mapper = lambda x: (x[0][0], x[0][1], x[0][2])
    averages = map(mapper, averages)
    print "Adding img %s to db with averages %s" % (img, str(averages))
    imgsmall = img.replace("images", "smallImages").replace(".jpg", "small.jpg")
    dbitem = {
      "imgsrc" : img,
      "srcsmall" : imgsmall,
      "averages" : averages
    }
    image_pool.insert(dbitem)

processImages()
