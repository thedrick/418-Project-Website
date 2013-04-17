import os, sys
import Image
import urllib
from StringIO import StringIO
from instagram.client import InstagramAPI

api = InstagramAPI(client_id="4aa0a8ef77b34a0d8e3ddf2d97523f22", client_secret="da88550123c74ddb9de9d7a3e0bb088d")
popular_media, pagin = api.tag_recent_media(count=20, tag_name="blue")
for media in popular_media:
  url = media.images['standard_resolution'].url
  print "Saving image at url " + url
  img = Image.open(StringIO(urllib.urlopen(url).read())).save("images/" + str(media.id) + ".jpg")