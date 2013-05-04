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
using namespace std;
using namespace Magick;

struct RGB {
  int red;
  int green;
  int blue;
};

class ImageSlicer {
  string imgsrc;
  string saveloc;
  int numSlices;
  vector< vector< RGB > > rgbs;
  vector< vector< RGB > > averages;
  vector< vector< Image > > slices;

public:
  ImageSlicer(string imgsrc, int numSlices, string saveloc);
  vector< vector< RGB > > getAverages();
  vector< vector< Image > > getSlices();

private:
  void slice();
};

ImageSlicer::ImageSlicer (string src, int n, string save) {
  imgsrc = src;
  numSlices = n;
  saveloc = save;
  slices = vector< vector<Image> > (51);
  for (int x = 0; x < n; x++) {
    slices[x] = vector<Image> (51);
  }
}

void ImageSlicer::slice() {
  Image img;
  img.read(imgsrc);
  int width = img.columns();
  int height = img.rows(); // size of instagram photo

  int subwidth = width / numSlices;
  int subheight = height / numSlices;
  int count = 0;
  for (int x = 0; x < numSlices; x++) {
    for (int y = 0; y < numSlices; y++) {
      Image piece = img;
      printf("Saving slice for x=%d, and y=%d\n", x, y);
      piece.crop(Geometry(subwidth, subheight, x * subwidth, y * subheight));
      stringstream sstr;
      sstr << "test/piece" << count << ".jpg";
      // piece.write(sstr.str());
      slices[y][x] = piece;
      count++;
    }
  }
  return;
}

vector< vector< Image > > ImageSlicer::getSlices() {
  slice();
  return slices;
}

int main() {
  ImageSlicer slicer("images/136310984593675760_6658108.jpg", 51, "out.jpg");
  cout << "slices: " << slicer.getSlices().size();
}



