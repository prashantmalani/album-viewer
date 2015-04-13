#!/usr/bin/python

import gdata.photos.service
import gdata.media
import gdata.geo

# This is the email and password to which the album will be shared
# It is advised to create a dummy account from which to access this data
# rather than using ones real email ID.
EMAIL = 'albumviwertrial123@gmail.com'
PASSWD = 'helloworld123'

# This is the ID where the album is stored. It is advisable to use a
# dummy ID here as well.
ALBUMID = 'p.malani'

def getClient(email, passwd):
  """Obtain a Google Data client for accessing Picasa Web Album data

  Args:
    email: Email ID of the client account
    pass : Password of the client account

  Returns:
    a Google Data client instance.
  """
  gd_client = gdata.photos.service.PhotosService()
  gd_client.email = 'albumviewertrial123@gmail.com'
  gd_client.password = 'helloworld123'
  gd_client.source = 'Album Viewer'
  gd_client.ProgrammaticLogin()
  return gd_client

if __name__ == "__main__":
  print 'Hello World'
  gd_client = getClient(EMAIL, PASSWD)

  albums = gd_client.GetUserFeed(user=ALBUMID)
  for album in albums.entry:
    print 'title: %s, number of photos: %s, id: %s' % (album.title.text,
          album.numphotos.text, album.gphoto_id.text)
