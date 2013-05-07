import os, sys
import Image
import urllib
from StringIO import StringIO
from instagram.client import InstagramAPI
from time import sleep

#words = ['sopretty', 'spring', 'summer', 'flowerstagram', 'flowersofinstagram', 'flowerstyles_gf', 'flowerslovers', 'flowerporn', 'botanical', 'floral', 'florals', 'insta_pick_blossom', 'flowermagic', 'instablooms', 'bloom', 'blooms', 'botanical', 'floweroftheday']
words = ['pages', 'paper', 'kindle', 'nook', 'library', 'author', 'bestoftheday', 'bookworm', 'readinglist', 'love', 'photooftheday', 'imagine', 'plot', 'climax', 'story', 'literature', 'literate', 'stories', 'words', 'text']

api = InstagramAPI(client_id="4aa0a8ef77b34a0d8e3ddf2d97523f22", client_secret="da88550123c74ddb9de9d7a3e0bb088d")
total = 0
maxid = 0
try:
  for tag in words:
    maxid = 0
    for x in xrange(5):
      sleep(5)
      if (maxid == 0):
        popular_media, pagin = api.tag_recent_media(count=100, tag_name=tag)
        print pagin
      else:
        print "trying page %d for tag %s" % (x,tag)
        api.tag_recent_media(count=100, max_id=maxid, tag_name=tag)
      for media in popular_media:
        url = media.images['standard_resolution'].url
        maxid = media.id
        try:
          Image.open(StringIO(urllib.urlopen(url).read())).save("images/" + str(media.id) + ".jpg")
          Image.open(StringIO(urllib.urlopen(url).read())).resize((48,48)).save("smallImages/" + str(media.id) + "small.jpg")
          total += 1
        except Exception as e:
          "Handling exception while saving photos ",
          print e
          continue
except Exception as e:
  print "Handling error and stopping",
  print e

print "number of images %d" % total
  
  
  
  
