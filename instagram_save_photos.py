import os, sys
import Image
import urllib
from StringIO import StringIO
from instagram.client import InstagramAPI
from time import sleep

words = [
  "penis",
  "cock",
  "dick",
  "porn",
  "gaysex"
]

api = InstagramAPI(client_id="4aa0a8ef77b34a0d8e3ddf2d97523f22", client_secret="da88550123c74ddb9de9d7a3e0bb088d")

for tag in words:
  sleep(5)
  popular_media, pagin = api.tag_recent_media(tag_name=tag)
  for media in popular_media:
    url = media.images['standard_resolution'].url
    try:
      print "Saving image at with caption " + media.caption.text
    except:
      print "No caption"
    img = Image.open(StringIO(urllib.urlopen(url).read())).save("test/" + str(media.id) + ".jpg")
  
  
  
  
