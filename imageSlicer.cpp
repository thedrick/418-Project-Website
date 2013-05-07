/*
ImageSlicer.cpp designed to reproduce the results of
ImageSlicer.py and slice up a target image into sections
used in image matching for creating photo mosaics
*/

#include <iostream>
#include <string>
#include <vector>
#include <string>
#include <sstream>
#include <Magick++.h>
#include <math.h>
#include <climits>
#include <list>
#include "mongo/client/dbclient.h"
#include "CycleTimer.h"

using namespace std;
using namespace Magick;
using namespace mongo;

struct RGB {
  int red;
  int green;
  int blue;
};

class ImageSlicer {
  string imgsrc; // source of the input image
  int numSlices; // number of slices in x and y direction
  int cutSize; // number of sub slices to make from each image 
  vector< vector< RGB > > rgbs; // vector to store RGB values of pieces
  vector<RGB> averages; // vector to store average rgbs
  vector< vector< Image > > slices; // image slices.
  Image sourceImage;

public:
  ImageSlicer(string imgsrc, int numSlices);
  vector<RGB> getAverages();
  vector< vector< Image > > getSlices();

private:
  void slice();
  void calculateRGBValues();
};

ImageSlicer::ImageSlicer (string src, int n) {
  imgsrc = src;
  numSlices = n;
  cutSize = 3;
  slices = vector< vector<Image> > (51);
  for (int x = 0; x < n; x++) {
    slices[x] = vector<Image> (51);
  }
  sourceImage = Image(src);
}

void ImageSlicer::slice() {
  Image img = sourceImage;
  int width = img.columns();
  int height = img.rows(); // size of instagram photo

  int subwidth = width / numSlices;
  int subheight = height / numSlices;
  int count = 0;
  for (int x = 0; x < numSlices; x++) {
    for (int y = 0; y < numSlices; y++) {
      Image piece = img;
      piece.crop(Geometry(subwidth, subheight, x * subwidth, y * subheight));
      // piece.write(sstr.str());
      slices[y][x] = piece;
      count++;
    }
  }
  return;
}

void ImageSlicer::calculateRGBValues() {
  int subwidth = 612 / numSlices;
  int subheight = 612 / numSlices;

  int average_subwidth = subwidth / cutSize;
  int average_subheight = subheight / cutSize;

  for (int x = 0; x < numSlices; x++) {
    for (int y = 0; y < numSlices; y++) {
      Image currentSlice = slices[x][y];
      Pixels view(currentSlice);
      PixelPacket *pixels = view.get(0, 0, currentSlice.columns(), currentSlice.rows());
      for (int i = 0; i < cutSize; i++) {
        for (int j = 0; j < cutSize; j++) {
          int red = 0;
          int green = 0;
          int blue = 0;
          for (int k = 0; k < average_subwidth; k++) {
            for (int l = 0; l < average_subheight; l++) {
              int curX = (i * average_subwidth) + k;
              int curY = (j * average_subheight) + l;
              int pixelLoc = curX + (curY * subwidth);
              ColorRGB pixel = ColorRGB(pixels[pixelLoc]);
              red += pixel.red() * 255;
              green += pixel.green() * 255;
              blue += pixel.blue() * 255;
            }
          }
          int numIters = average_subwidth * average_subheight;
          red /= numIters;
          blue /= numIters;
          green /= numIters;
          RGB rgb;
          rgb.red = red;
          rgb.green = green;
          rgb.blue = blue;
          averages.push_back(rgb);
        }
      }
    }
  }
}

vector< vector< Image > > ImageSlicer::getSlices() {
  slice();
  return slices;
}

vector<RGB> ImageSlicer::getAverages() {
  slice();
  calculateRGBValues();
  return averages;
}

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
  if (a2.size() != 9) {
    return 255 * 255;
  }
  int dist = 0;
  for (size_t i = 0; i < a1.size(); i++) {
    dist += RGBdistance(a1[i], a2[i]);
  }
  return dist;
}

int main(int argc, char **argv) {
  InitializeMagick(*argv);
  if (argc < 3) {
    cout << "usage: " << argv[0] << " <image path> <save path>\n";
    return 1;
  }
  ImageSlicer slicer(argv[1], 51);
  string savepath = argv[2];
  DBClientConnection c;
  c.connect("localhost");
  vector <vector <RGB> > dbImageColors;
  vector <string> dbImageSources;
  auto_ptr<DBClientCursor> cursor = c.query("instagram_photomosaic.image_pool", BSONObj());
  // load all the stuff from the database to check against.
  double dbstart = CycleTimer::currentSeconds();
  while (cursor->more()) {
    BSONObj obj = cursor->next();
    dbImageSources.push_back(obj.getStringField("imgsrc"));
    BSONObjIterator fields (obj.getObjectField("averages"));
    vector <RGB> curRGBs;
    while (fields.more()) {
      vector<BSONElement> elems = fields.next().Array();
      int red = elems[0].Int();
      int green = elems[1].Int();
      int blue = elems[2].Int();
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
  for (size_t i = 0; i < averages.size(); i += 9) {
    // current value of the minimum distance and it's index.
    int minIndex = 0;
    int minVal = INT_MAX;
    vector<RGB> current;
    // grab the next 9 values (corresponds to a subimage)
    for (int j = 0; j < 9; j++) {
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
    vector<RGB> empty;
    dbImageColors[minIndex] = empty;
  }
  double imgend = CycleTimer::currentSeconds();
  printf("Time to compute image matches %f\n", (imgend - imgstart));
  list <Image> finalMontage;
  Montage montage;
  montage.tile("51x51");
  montage.geometry("48x48");
  vector<Image> images;
  double montagestart = CycleTimer::currentSeconds();
  for (size_t n = 0; n < finalImages.size(); n++) {
    string filename = finalImages[n];
    Image mosaicImage(filename);
    mosaicImage.resize("48x48");
    if (n % 50 == 0) {
      printf("Opening image %zu\n", n);
    }
    images.push_back(mosaicImage);
  }
  printf("Attempting to montage the image\n");
  montageImages(&finalMontage, images.begin(), images.end(), montage);
  writeImages(finalMontage.begin(), finalMontage.end(), savepath);
  double montageend = CycleTimer::currentSeconds();
  printf("Time to create and write montage to file %f\n", (montageend - montagestart));
}



