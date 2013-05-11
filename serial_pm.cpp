/*
ImageSlicer.cpp designed to reproduce the results of
ImageSlicer.py and slice up a target image into sections
used in image matching for creating photo mosaics
*/

#include <math.h>
#include <climits>
#include <list>
#include "mongo/client/dbclient.h"
#include "CycleTimer.h"
#include "imageSlicer.h"

using namespace mongo;

int square(int x) {
  return x * x;
}

int RGBdistance(RGB t1, RGB t2) {
  int red = square(t1.red - t2.red);
  int green = square(t1.green - t2.green);
  int blue = square(t1.blue - t2.blue);
  return (int)sqrt(red + green + blue);
}

int totalDistance(vector<RGB> a1, vector<RGB> a2) {
  if (a2.size() == 0) {
    return INT_MAX;
  }
  int dist = 0;
  for (size_t i = 0; i < a1.size(); i++) {
    dist += RGBdistance(a1[i], a2[i]);
  }
  return dist;
}

int main(int argc, char **argv) {
  InitializeMagick(0);
  if (argc < 4) {
    cout << "usage: " << argv[0] << " <image path> <save path> <cutSize>\n";
    return 1;
  }
  int cutSize = atoi(argv[3]);
  ImageSlicer slicer(argv[1], 102, cutSize);
  string savepath = argv[2];
  DBClientConnection c;
  c.connect("localhost");
  vector <vector <RGB> > dbImageColors;
  vector <string> dbImageSources;
  auto_ptr<DBClientCursor> cursor = c.query("instagram_photomosaic.image_pool_cpp", BSONObj());
  // load all the stuff from the database to check against.
  double dbstart = CycleTimer::currentSeconds();
  while (cursor->more()) {
    BSONObj obj = cursor->next();
    dbImageSources.push_back(obj.getStringField("srcsmall"));
    BSONObjIterator fields (obj.getObjectField("averages"));
    vector <RGB> curRGBs;
    while (fields.more()) {
      vector<BSONElement> elems = fields.next().Array();
      int red = elems[0].Int();
      int green = elems[2].Int();
      int blue = elems[1].Int();
      RGB rgb;
      rgb.red = red;
      rgb.green = green;
      rgb.blue = blue;
      curRGBs.push_back(rgb);
    }
    // add vector of 9 rgbs to large vector

    dbImageColors.push_back(curRGBs);
  }
  double dbend = CycleTimer::currentSeconds();
  printf("Time to read in DB %f\n", (dbend - dbstart));
  // average values of the input image. This is an array of a bunch of RGBs
  // where they are grouped in 9s in order.
  double avgstart = CycleTimer::currentSeconds();
  vector<RGB> averages = slicer.getAverages();
  double avgend = CycleTimer::currentSeconds();
  printf("Time to find averages of input image %f\n", (avgend - avgstart));
  vector<string> finalImages;
  double imgstart = CycleTimer::currentSeconds();
  for (size_t i = 0; i < averages.size(); i += (cutSize * cutSize)) {
    // current value of the minimum distance and it's index.
    int minIndex = 0;
    int minVal = INT_MAX;
    vector<RGB> current;
    // grab the next 9 values (corresponds to a subimage)
    for (int j = 0; j < (cutSize * cutSize); j++) {
      current.push_back(averages[i + j]);
    }
    for (size_t k = 0; k < dbImageColors.size(); k++) {
      // get distance from current subimage to current image from db.
      int dist = totalDistance(current, dbImageColors[k]);
      if (dist < minVal) {
        minIndex = k;
        minVal = dist;
      }
    }
    finalImages.push_back(dbImageSources[minIndex]);
    // replace minIndex with empty vector to remove values and avoid duplicates.
    // vector<RGB> empty;
    // dbImageColors[minIndex] = empty;
  }
  dbImageColors.resize(0);
  averages.resize(0);
  double imgend = CycleTimer::currentSeconds();
  printf("Time to compute image matches %f\n", (imgend - imgstart));
  list <Image> finalMontage;
  Montage montage;
  montage.tile("51x51");
  montage.geometry("48x48");
  vector<Image> images;
  double montagestart = CycleTimer::currentSeconds();
  for (int n = 0; n < (int)finalImages.size(); n++) {
    string filename = finalImages[n];
    if (n % 100 == 0) {
      printf("Opening image %d at path %s\n", n, filename.c_str());
    }
    Image mosaicImage(filename);
    mosaicImage.resize("6x6");
    images.push_back(mosaicImage);
  }
  montageImages(&finalMontage, images.begin(), images.end(), montage);
  writeImages(finalMontage.begin(), finalMontage.end(), savepath);
  double montageend = CycleTimer::currentSeconds();
  printf("Time to create and write montage to file %f\n", (montageend - montagestart));
}



