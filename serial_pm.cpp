/*
ImageSlicer.cpp designed to reproduce the results of
ImageSlicer.py and slice up a target image into sections
used in image matching for creating photo mosaics
*/

#include <math.h>
#include <climits>
#include <list>
#include <jpeglib.h>
#include <stdlib.h>
#include <stdio.h>
#include "mongo/client/dbclient.h"
#include "CycleTimer.h"
#include "imageSlicer.h"

using namespace mongo;
void add_images_to_raw(unsigned char* img, int index);

/* pointer to new image */
unsigned char *raw_image = NULL;

/* mosaic tile image dimensions */
int iwidth = 48;
int iheight = 48;

/* mosaic dimensions and component info */
int width = 4896;
int height = 4896;
int bytes_per_pixel = 3;
J_COLOR_SPACE color_space = JCS_RGB;

/*** change in c++ to iwidth/width ***/
int dim = width / iwidth;

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

int read_jpeg_to_array(char *filename, int idx) {
  /* standard libjpeg structures for reading */
  struct jpeg_decompress_struct cinfo;
  struct jpeg_error_mgr jerr;
  /* structure to store scanline of image */
  JSAMPROW row_pointer[1];
  /* array to hold current image */
  unsigned char *image_lines;
  FILE *infile = fopen(filename, "rb");
  unsigned long loc = 0;
  int i = 0;

  if (!infile) {
    printf("Error opening file %s.\n", filename);
    return -1;
  }
  /* set up all decompress and reading image */
  cinfo.err = jpeg_std_error(&jerr);
  jpeg_create_decompress(&cinfo);
  jpeg_stdio_src(&cinfo, infile);
  jpeg_read_header(&cinfo, TRUE);
  jpeg_start_decompress(&cinfo);

  image_lines = (unsigned char*)malloc(cinfo.output_width*cinfo.output_height*cinfo.num_components);
  row_pointer[0] = (unsigned char*)malloc(cinfo.output_width*cinfo.num_components);

  /* writes image scanline info to image_lines */
  while(cinfo.output_scanline < cinfo.image_height) {
    jpeg_read_scanlines(&cinfo, row_pointer, 1);
    for(i=0; i<(int)cinfo.image_width*cinfo.num_components; i++) {
      image_lines[loc++] = row_pointer[0][i];
    }
  }

  jpeg_finish_decompress(&cinfo);
  jpeg_destroy_decompress(&cinfo);
  free(row_pointer[0]);
  fclose(infile);

  /* write image to respective idx in mosaic array */
  add_images_to_raw(image_lines, idx);
  free(image_lines);
  return 1;
}

void add_images_to_raw(unsigned char* img, int index) {
  int j;
  //int raw_loc = index*iwidth*iheight*bytes_per_pixel;
  int start_loc = (index*iwidth*bytes_per_pixel) + ((width*(index/dim))*iheight*bytes_per_pixel);
  int offset = (dim-1)*bytes_per_pixel;
  int count = 0;
  //printf("Image index %i\n", index);
  for (j=0; j < iheight*iwidth*bytes_per_pixel; j++) {
    if (count == iwidth*bytes_per_pixel) {
      start_loc += iwidth*offset;
      count = 0;
    }
    raw_image[start_loc++] = img[j];
    count++;;
  }
}

int write_jpeg_to_file(char *filename) {
  struct jpeg_compress_struct cinfo;
  struct jpeg_error_mgr jerr;
  JSAMPROW row_pointer[1];
  FILE *outfile = fopen(filename, "wb");

  if (!outfile) {
    printf("Error opening output file %s.\n", filename);
    return -1;
  }

  cinfo.err = jpeg_std_error(&jerr);
  jpeg_create_compress(&cinfo);
  jpeg_stdio_dest(&cinfo, outfile);

  cinfo.image_width = width;
  cinfo.image_height = height;
  cinfo.input_components = bytes_per_pixel;
  cinfo.in_color_space = color_space;

  jpeg_set_defaults(&cinfo);
  jpeg_start_compress(&cinfo, TRUE);

  while(cinfo.next_scanline < cinfo.image_height) {
    row_pointer[0] = &raw_image[cinfo.next_scanline*cinfo.image_width*cinfo.input_components];
    jpeg_write_scanlines(&cinfo, row_pointer, 1);
  }

  jpeg_finish_compress(&cinfo);
  jpeg_destroy_compress(&cinfo);
  fclose(outfile);
  return 1;
}

int main(int argc, char **argv) {
  InitializeMagick(0);
  if (argc < 4) {
    cout << "usage: " << argv[0] << " <image path> <save path> <cutSize>\n";
    return 1;
  }

  raw_image = (unsigned char*)malloc(width*height*bytes_per_pixel*sizeof(char*));

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
    double subimagestart = CycleTimer::currentSeconds();
    for (size_t k = 0; k < dbImageColors.size(); k++) {
      // get distance from current subimage to current image from db.
      int dist = totalDistance(current, dbImageColors[k]);
      if (dist < minVal) {
        minIndex = k;
        minVal = dist;
      }
    }
    double subimageend = CycleTimer::currentSeconds();
    if (i % 50 == 0) {
      printf("Time to find one image match: %f\n", (subimageend - subimagestart));
    }
    finalImages.push_back(dbImageSources[minIndex]);
    // replace minIndex with empty vector to remove values and avoid duplicates.
    vector<RGB> empty;
    dbImageColors[minIndex] = empty;
  }
  dbImageColors.resize(0);
  averages.resize(0);
  double imgend = CycleTimer::currentSeconds();
  printf("Time to compute image matches %f\n", (imgend - imgstart));
  list <Image> finalMontage;
  vector<Image> images;
  double montagestart = CycleTimer::currentSeconds();
  for (int n = 0; n < (int)finalImages.size(); n++) {
    string filename = finalImages[n];
    if (n % 100 == 0) {
      printf("Opening image %d at path %s\n", n, filename.c_str());
    }
    // Image mosaicImage(filename);
    // images.push_back(mosaicImage);
    char* writable = new char[filename.size() + 1];
    copy(filename.begin(), filename.end(), writable);
    writable[filename.size()] = '\0';
    read_jpeg_to_array(writable, n);
    delete [] writable;
  }
  // montageImages(&finalMontage, images.begin(), images.end(), montage);
  // writeImages(finalMontage.begin(), finalMontage.end(), savepath);
  char *writable = new char[savepath.size() + 1];
  copy(savepath.begin(), savepath.end(), writable);
  writable[savepath.size()] = '\0';
  write_jpeg_to_file(writable);
  delete [] writable;
  free(raw_image);
  double montageend = CycleTimer::currentSeconds();
  printf("Time to create and write montage to file %f\n", (montageend - montagestart));
}



