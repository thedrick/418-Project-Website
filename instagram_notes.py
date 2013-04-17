#### Instagram API notes ####

# stuff to import
import os, sys, urllib, Image
from StringIO import StringIO
from instagram.client import InstagramAPI

# set up the api 
api = InstagramAPI(client_id="4aa0a8ef77b34a0d8e3ddf2d97523f22", client_secret="da88550123c74ddb9de9d7a3e0bb088d")

# get the most popular media
popular_media = api.media_popular(count=20)
media = popular_media[0]

# fields
media.caption
media.comments
media.filter
media.id
media.like_count
media.link # instagram website link
media.user # user object
media.comment_count
media.created_time
media.get_standard_resolution_url()
media.images
media.likes
media.object_from_dictionary # no clue what this does.

# get the image
img = media.images['standard_resolution']

# fields on images
img.width
img.height
img.url

