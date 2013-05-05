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
#include "mongo/client/dbclient.h"
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
  vector <RGB> reordered;
  int cutSize2 = cutSize * cutSize;
  int block_size = numSlices * cutSize2;
  int h = 0;
  while (h != numSlices) {
    for (int p = block_size * h; p < block_size*(h+1); p++) {
      if (p % cutSize2 == 0 || p % cutSize2 == 1 || p % cutSize2 == 2) {
        reordered.push_back(averages[p]);
      }
    } 

    for (int q = block_size * h; q < block_size*(h+1); q++) {
      if (q % cutSize2 == 3 || q % cutSize2 == 4 || q % cutSize2 == 5) {
        reordered.push_back(averages[q]);
      }
    } 

    for (int r = block_size * h; r < block_size*(h+1); r++) {
      if (r % cutSize2 == 0 || r % cutSize2 == 1 || r % cutSize2 == 2) {
        reordered.push_back(averages[r]);
      }
    }
    h++;
  }
  averages = reordered;
}

vector< vector< Image > > ImageSlicer::getSlices() {
  slice();
  return slices;
}

vector<RGB> ImageSlicer::getAverages() {
  calculateRGBValues();
  return averages;
}

int main(int argc, char **argv) {
  InitializeMagick(*argv);
  ImageSlicer slicer("images/235246727104344123_202091.jpg", 51);
  DBClientConnection c;
  c.connect("localhost");
  vector <vector <RGB> > dbImageColors;
  vector <string> dbImageSources;
  auto_ptr<DBClientCursor> cursor = c.query("instagram_photomosaic.image_pool", BSONObj());
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
    dbImageColors.push_back(curRGBs);
  }
  cout << dbImageColors.size() << endl;
  cout << dbImageSources.size() << endl;
}



