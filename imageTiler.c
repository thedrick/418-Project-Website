#include <stdio.h>
#include <jpeglib.h>
#include <stdlib.h>

/* pointer to new image */
unsigned char *raw_image = NULL;

int iwidth = 612;
int iheight = 612;

int width = 2448;
int height = 2448;
int bytes_per_pixel = 3;
int color_space = JCS_RGB;

int dim = 2448/612;

unsigned char* read_jpeg_to_array(char *filename) {
  /* standard libjpeg structures for reading */
  struct jpeg_decompress_struct cinfo;
  struct jpeg_error_mgr jerr;
  /* structure to store scanline of image */
  JSAMPROW row_pointer[1];

  unsigned char *image_lines;

  FILE *infile = fopen(filename, "rb");
  unsigned long loc = 0;
  int i = 0;

  if (!infile) {
    printf("Error opening file %s.\n", filename);
    exit(1);
  }

  /* set up all decompress and reading image */
  cinfo.err = jpeg_std_error(&jerr);
  jpeg_create_decompress(&cinfo);
  jpeg_stdio_src(&cinfo, infile);
  jpeg_read_header(&cinfo, TRUE);
  jpeg_start_decompress(&cinfo);

  image_lines = (unsigned char*)malloc(cinfo.output_width*cinfo.output_height*cinfo.num_components);
  row_pointer[0] = (unsigned char*)malloc(cinfo.output_width*cinfo.num_components);

  while(cinfo.output_scanline < cinfo.image_height) {
    jpeg_read_scanlines(&cinfo, row_pointer, 1);

    for(i=0; i<cinfo.image_width*cinfo.num_components; i++) {
      image_lines[loc++] = row_pointer[0][i];
    }
  } // close while loop

  jpeg_finish_decompress(&cinfo);
  jpeg_destroy_decompress(&cinfo);
  free(row_pointer[0]);
  fclose(infile);

  return image_lines;
}

int add_images_to_raw(unsigned char* img, int index) {
  int j;
  int start_loc;
  int offset = (dim-1)*bytes_per_pixel;
  printf("Image index %i\n", index);
  //int raw_loc = index*iwidth*iheight*bytes_per_pixel; 

  start_loc = (index*iwidth*bytes_per_pixel) + ((width*(index/dim))*iheight*bytes_per_pixel);
  int count = 0;
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

int main() {
  raw_image = (unsigned char*)malloc(width*height*bytes_per_pixel*dim);
  char *filenames[16];
  filenames[0] = "one.jpg";
  filenames[1] = "two.jpg";
  filenames[2] = "three.jpg";
  filenames[3] = "four.jpg";
  filenames[4] = "five.jpg";
  filenames[5] = "six.jpg";
  filenames[6] = "seven.jpg";
  filenames[7] = "eight.jpg";
  filenames[8] = "nine.jpg";
  filenames[9] = "ten.jpg";
  filenames[10] = "eleven.jpg";
  filenames[11] = "twelve.jpg";
  filenames[12] = "thirteen.jpg";
  filenames[13] = "fourteen.jpg";
  filenames[14] = "fifteen.jpg";
  filenames[15] = "sixteen.jpg";
  char *outfilename = "testall.jpg";
  int i;
  for (i=0; i<16; i++) {
    add_images_to_raw(read_jpeg_to_array(filenames[i]), i);
  }
  write_jpeg_to_file(outfilename);
  return 0;
}
