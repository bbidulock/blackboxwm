// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Image.cc for Blackbox - an X11 Window manager
// Copyright (c) 2001 - 2003 Sean 'Shaleh' Perry <shaleh@debian.org>
// Copyright (c) 1997 - 2000, 2002 - 2003
//         Bradley T Hughes <bhughes at trolltech.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

#ifdef    HAVE_CONFIG_H
#  include "../config.h"
#endif // HAVE_CONFIG_H

extern "C" {
#ifdef    MITSHM
#  include <sys/types.h>
#  include <sys/ipc.h>
#  include <sys/shm.h>
#  include <unistd.h>
#  include <X11/Xlib.h>
#  include <X11/extensions/XShm.h>
#endif // MITSHM

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
}

#include <algorithm>

#include "Display.hh"
#include "Image.hh"
#include "Pen.hh"
#include "Texture.hh"

// #define COLORTABLE_DEBUG
// #define MITSHM_DEBUG


static const unsigned int maximumWidth  = 8000;
static const unsigned int maximumHeight = 6000;


unsigned int bt::Image::global_colorsPerChannel = 6;
bt::DitherMode bt::Image::global_ditherMode = bt::OrderedDither;


namespace bt {

  class XColorTable {
  public:
    XColorTable(const Display &dpy, unsigned int screen,
                unsigned int colors_per_channel);
    ~XColorTable(void);

    void map(unsigned int &red,
             unsigned int &green,
             unsigned int &blue);
    unsigned long pixel(unsigned int red,
                        unsigned int green,
                        unsigned int blue);

  private:
    const Display &_dpy;
    unsigned int _screen;
    int _vclass;
    unsigned int _cpc, _cpcsq;
    int red_offset, green_offset, blue_offset;
    unsigned int red_bits, green_bits, blue_bits;

    unsigned char _red[256];
    unsigned char _green[256];
    unsigned char _blue[256];
    std::vector<XColor> colors;
  };


  typedef std::vector<unsigned char> Buffer;
  static Buffer buffer;


  typedef std::vector<XColorTable*> XColorTableList;
  static XColorTableList colorTableList;


  void destroyColorTables(void) {
    XColorTableList::iterator it = colorTableList.begin(),
                             end = colorTableList.end();
    for (; it != end; ++it) {
      if (*it) delete *it;
      *it = 0;
    }
    colorTableList.clear();
    buffer.clear();
  }


#ifdef MITSHM
  static XShmSegmentInfo shm_info;
  static bool use_shm = false;
  static bool shm_attached = false;
  static int shm_id = -1;
  static char *shm_addr = reinterpret_cast<char *>(-1);


  static int handleShmError(::Display *, XErrorEvent *) {
    use_shm = false;
    return 0;
  }


  void startupShm(const Display &display) {
    // query MIT-SHM extension
    if (! XShmQueryExtension(display.XDisplay())) return;
    use_shm = true;
  }


  void destroyShmImage(const Display &display, XImage *image) {
    // tell the X server to detach
    if (shm_attached) {
      XShmDetach(display.XDisplay(), &shm_info);

      // wait for the server to render the image and detach the memory
      // segment
      XSync(display.XDisplay(), False);

      shm_attached = false;
    }

    // detach shared memory segment
    if (shm_addr != reinterpret_cast<char *>(-1))
      shmdt(shm_addr);
    shm_addr = reinterpret_cast<char *>(-1);

    // destroy shared memory id
    if (shm_id != -1)
      shmctl(shm_id, IPC_RMID, 0);
    shm_id = -1;

    // destroy XImage
    image->data = 0;
    XDestroyImage(image);
  }


  XImage *createShmImage(const Display &display, const ScreenInfo &screeninfo,
                         unsigned int width, unsigned int height) {
    if (! use_shm) return 0;

    // use MIT-SHM extension
    XImage *image = XShmCreateImage(display.XDisplay(), screeninfo.visual(),
                                    screeninfo.depth(), ZPixmap, 0,
                                    &shm_info, width, height);
    if (! image) return 0;

    // get shared memory id
    unsigned int usage = image->bytes_per_line * image->height;
    shm_id = shmget(IPC_PRIVATE, usage, IPC_CREAT | 0644);
    if (shm_id == -1) {
#ifdef MITSHM_DEBUG
      perror("bt::createShmImage: shmget");
#endif // MITSHM_DEBUG

      use_shm = false;
      XDestroyImage(image);
      return 0;
    }

    // attached shared memory segment
    shm_addr = static_cast<char *>(shmat(shm_id, 0, 0));
    if (shm_addr == reinterpret_cast<char *>(-1)) {
#ifdef MITSHM_DEBUG
      perror("bt::createShmImage: shmat");
#endif // MITSHM_DEBUG

      use_shm = false;
      destroyShmImage(display, image);
      return 0;
    }

    // tell the X server to attach
    shm_info.shmid = shm_id;
    shm_info.shmaddr = shm_addr;
    shm_info.readOnly = True;

    static bool test_server_attach = true;
    if (test_server_attach) {
      // never checked if the X server can do shared memory...
      XErrorHandler old_handler = XSetErrorHandler(handleShmError);
      XShmAttach(display.XDisplay(), &shm_info);
      XSync(display.XDisplay(), False);
      XSetErrorHandler(old_handler);

      if (! use_shm) {
        // the X server failed to attach the shm segment

#ifdef MITSHM_DEBUG
        fprintf(stderr, "bt::createShmImage: X server failed to attach\n");
#endif // MITSHM_DEBUG

        destroyShmImage(display, image);
        return 0;
      }

      test_server_attach = false;
    } else {
      // we know the X server can attach to the memory segment
      XShmAttach(display.XDisplay(), &shm_info);
    }

    shm_attached = true;
    image->data = shm_addr;
    return image;
  }
#endif // MITSHM

} // namespace bt


bt::XColorTable::XColorTable(const Display &dpy, unsigned int screen,
                             unsigned int colors_per_channel)
  : _dpy(dpy), _screen(screen),
    _cpc(colors_per_channel), _cpcsq(_cpc * _cpc) {
  const ScreenInfo &screeninfo = _dpy.screenInfo(_screen);
  _vclass = screeninfo.visual()->c_class;
  unsigned int depth = screeninfo.depth();

  red_offset = green_offset = blue_offset = 0;
  red_bits = green_bits = blue_bits = 0;

  switch (_vclass) {
  case StaticGray: {
    red_bits = green_bits = blue_bits =
               255u / (DisplayCells(_dpy.XDisplay(), screen) - 1);
    break;
  }

  case GrayScale:
  case StaticColor:
  case PseudoColor: {
    unsigned int ncolors = _cpc * _cpc * _cpc;

    if (ncolors > (1u << depth)) {
      _cpc = (1u << depth) / 3u;
      ncolors = _cpc * _cpc * _cpc;
    }

    if (_cpc < 2u || ncolors > (1u << depth)) {
      // invalid colormap size, reduce
      _cpc = (1u << depth) / 3u;
    }

    colors.resize(ncolors);
    red_bits = green_bits = blue_bits = 255u / (_cpc - 1);
    break;
  }

  case TrueColor:
  case DirectColor: {
    // compute color tables
    unsigned long red_mask = screeninfo.visual()->red_mask,
                green_mask = screeninfo.visual()->green_mask,
                 blue_mask = screeninfo.visual()->blue_mask;

    while (! (  red_mask & 1)) {   red_offset++;   red_mask >>= 1; }
    while (! (green_mask & 1)) { green_offset++; green_mask >>= 1; }
    while (! ( blue_mask & 1)) {  blue_offset++;  blue_mask >>= 1; }

    red_bits   = 255u /   red_mask;
    green_bits = 255u / green_mask;
    blue_bits  = 255u /  blue_mask;

    break;
  }
  } // switch

  // initialize color tables
  unsigned int i;
  for (i = 0; i < 256u; i++) {
    _red[i]   = i / red_bits;
    _green[i] = i / green_bits;
    _blue[i]  = i / blue_bits;
  }

  if (! colors.empty()) {
    // query existing colormap
    const Colormap colormap = screeninfo.colormap();
    XColor icolors[256];
    unsigned int incolors = (((1u << depth) > 256u) ? 256u : (1u << depth));
    for (i = 0; i < incolors; i++) icolors[i].pixel = i;

    XQueryColors(_dpy.XDisplay(), colormap, icolors, incolors);

#ifdef COLORTABLE_DEBUG
    for (i = 0; i < incolors; ++i) {
      fprintf(stderr, "query %3d: %02x/%02x/%02x %d/%d/%d flags %x\n", i,
              (icolors[i].red   >> 8),
              (icolors[i].green >> 8),
              (icolors[i].blue  >> 8),
              (icolors[i].red   >> 8) / red_bits,
              (icolors[i].green >> 8) / green_bits,
              (icolors[i].blue  >> 8) / blue_bits,
              icolors[i].flags);
    }
#endif // COLORTABLE_DEBUG

    // create color cube
    unsigned int ii, p, r, g, b;
    for (r = 0, i = 0; r < _cpc; r++) {
      for (g = 0; g < _cpc; g++) {
     	for (b = 0; b < _cpc; b++, i++) {
     	  colors[i].red   = (r * 0xffff) / (_cpc - 1);
     	  colors[i].green = (g * 0xffff) / (_cpc - 1);
     	  colors[i].blue  = (b * 0xffff) / (_cpc - 1);
     	  colors[i].flags = DoRed|DoGreen|DoBlue;

#ifdef COLORTABLE_DEBUG
          fprintf(stderr, "req   %3d: %02x/%02x/%02x %1d/%1d/%1d\n", i,
                  (colors[i].red   >> 8),
                  (colors[i].green >> 8),
                  (colors[i].blue  >> 8),
                  (colors[i].red   >> 8) / red_bits,
                  (colors[i].green >> 8) / green_bits,
                  (colors[i].blue  >> 8) / blue_bits);
#endif // COLORTABLE_DEBUG
     	}
      }
    }

    // for missing colors, find the closest color in the existing colormap
    for (i = 0; i < colors.size(); i++) {
      unsigned long chk = 0xffffffff, pix, close = 0;

      p = 2;
      while (p--) {
        for (ii = 0; ii < incolors; ii++) {
          r = (colors[i].red   - icolors[ii].red)   >> 8;
          g = (colors[i].green - icolors[ii].green) >> 8;
          b = (colors[i].blue  - icolors[ii].blue)  >> 8;
          pix = (r * r) + (g * g) + (b * b);

          if (pix < chk) {
            chk = pix;
            close = ii;
          }
        }

        colors[i].red   = icolors[close].red;
        colors[i].green = icolors[close].green;
        colors[i].blue  = icolors[close].blue;

        if (XAllocColor(_dpy.XDisplay(), colormap,
                        &colors[i])) {

#ifdef COLORTABLE_DEBUG
          fprintf(stderr, "close %3d: %02x/%02x/%02x %1d/%1d/%1d (%d)\n", i,
                  (colors[i].red   >> 8),
                  (colors[i].green >> 8),
                  (colors[i].blue  >> 8),
                  (colors[i].red   >> 8) / red_bits,
                  (colors[i].green >> 8) / green_bits,
                  (colors[i].blue  >> 8) / blue_bits,
                  colors[i].pixel);
#endif // COLORTABLE_DEBUG

          colors[i].flags = DoRed|DoGreen|DoBlue;
          break;
        }
      }
    }
  }
}


bt::XColorTable::~XColorTable(void) {
  if (! colors.empty()) {
    std::vector<unsigned long> pixvals(colors.size());
    std::vector<unsigned long>::iterator pt = pixvals.begin();
    std::vector<XColor>::const_iterator xt = colors.begin();
    for (; xt != colors.end() && pt != pixvals.end(); ++xt, ++pt)
      *pt = xt->pixel;

    XFreeColors(_dpy.XDisplay(), _dpy.screenInfo(_screen).colormap(),
                &pixvals[0], pixvals.size(), 0);

    pixvals.clear();
    colors.clear();
  }
}


void bt::XColorTable::map(unsigned int &red,
                          unsigned int &green,
                          unsigned int &blue) {
  red   = _red  [  red];
  green = _green[green];
  blue  = _blue [ blue];
}


unsigned long bt::XColorTable::pixel(unsigned int red,
                                     unsigned int green,
                                     unsigned int blue) {
  switch (_vclass) {
  case StaticGray:
    return std::max(red, std::max(green, blue));

  case GrayScale:
  case StaticColor:
  case PseudoColor:
    return colors[(red * _cpcsq) + (green * _cpc) + blue].pixel;

  case TrueColor:
  case DirectColor:
    return ((red   << red_offset) |
            (green << green_offset) |
            (blue  << blue_offset));
  }

  // not reached
  return 0; // shut up compiler warning
}


bt::Image::Image(unsigned int w, unsigned int h)
  :  width(w), height(h) {
  assert(width > 0  && width  < maximumWidth);
  assert(height > 0 && height < maximumHeight);

  data = static_cast<RGB *>(malloc(width * height * sizeof(RGB)));
}


bt::Image::~Image(void) {
  free(data);
  data = 0;
}


Pixmap bt::Image::render(const Display &display, unsigned int screen,
                         const bt::Texture &texture) {
  if (texture.texture() & bt::Texture::Parent_Relative)
    return ParentRelative;
  if (texture.texture() & bt::Texture::Solid)
    return None;
  if (! (texture.texture() & bt::Texture::Gradient))
    return None;

  Color from, to;
  bool interlaced = texture.texture() & bt::Texture::Interlaced;

  if (texture.texture() & bt::Texture::Sunken) {
    from = texture.colorTo();
    to = texture.color();
  } else {
    from = texture.color();
    to = texture.colorTo();
  }

  if (texture.texture() & bt::Texture::Diagonal)
    dgradient(from, to, interlaced);
  else if (texture.texture() & bt::Texture::Elliptic)
    egradient(from, to, interlaced);
  else if (texture.texture() & bt::Texture::Horizontal)
    hgradient(from, to, interlaced);
  else if (texture.texture() & bt::Texture::Pyramid)
    pgradient(from, to, interlaced);
  else if (texture.texture() & bt::Texture::Rectangle)
    rgradient(from, to, interlaced);
  else if (texture.texture() & bt::Texture::Vertical)
    vgradient(from, to, interlaced);
  else if (texture.texture() & bt::Texture::CrossDiagonal)
    cdgradient(from, to, interlaced);
  else if (texture.texture() & bt::Texture::PipeCross)
    pcgradient(from, to, interlaced);

  if (texture.texture() & (bt::Texture::Sunken | bt::Texture::Raised))
    bevel(texture.borderWidth());

  Pixmap pixmap = renderPixmap(display, screen);

  unsigned int bw = 0;
  if (texture.texture() & bt::Texture::Border) {
    Pen penborder(screen, texture.borderColor());
    bw = texture.borderWidth();

    for (unsigned int i = 0; i < bw; ++i)
      XDrawRectangle(display.XDisplay(), pixmap, penborder.gc(),
                     i, i, width - (i * 2) - 1, height - (i * 2) - 1);
  }

  return pixmap;
}


/*
 * Ordered dither table
 */
static const unsigned int dither16[16][16] = {
  {     0, 49152, 12288, 61440,  3072, 52224, 15360, 64512,
      768, 49920, 13056, 62208,  3840, 52992, 16128, 65280 },
  { 32768, 16384, 45056, 28672, 35840, 19456, 48128, 31744,
    33536, 17152, 45824, 29440, 36608, 20224, 48896, 32512 },
  {  8192, 57344,  4096, 53248, 11264, 60416,  7168, 56320,
     8960, 58112,  4864, 54016, 12032, 61184,  7936, 57088 },
  { 40960, 24576, 36864, 20480, 44032, 27648, 39936, 23552,
    41728, 25344, 37632, 21248, 44800, 28416, 40704, 24320 },
  {  2048, 51200, 14336, 63488,  1024, 50176, 13312, 62464,
     2816, 51968, 15104, 64256,  1792, 50944, 14080, 63232 },
  { 34816, 18432, 47104, 30720, 33792, 17408, 46080, 29696,
    35584, 19200, 47872, 31488, 34560, 18176, 46848, 30464 },
  { 10240, 59392,  6144, 55296,  9216, 58368,  5120, 54272,
    11008, 60160,  6912, 56064,  9984, 59136,  5888, 55040 },
  { 43008, 26624, 38912, 22528, 41984, 25600, 37888, 21504,
    43776, 27392, 39680, 23296, 42752, 26368, 38656, 22272 },
  {   512, 49664, 12800, 61952,  3584, 52736, 15872, 65024,
      256, 49408, 12544, 61696,  3328, 52480, 15616, 64768 },
  { 33280, 16896, 45568, 29184, 36352, 19968, 48640, 32256,
    33024, 16640, 45312, 28928, 36096, 19712, 48384, 32000 },
  {  8704, 57856,  4608, 53760, 11776, 60928,  7680, 56832,
     8448, 57600,  4352, 53504, 11520, 60672,  7424, 56576 },
  { 41472, 25088, 37376, 20992, 44544, 28160, 40448, 24064,
    41216, 24832, 37120, 20736, 44288, 27904, 40192, 23808 },
  {  2560, 51712, 14848, 64000,  1536, 50688, 13824, 62976,
     2304, 51456, 14592, 63744,  1280, 50432, 13568, 62720 },
  { 35328, 18944, 47616, 31232, 34304, 17920, 46592, 30208,
    35072, 18688, 47360, 30976, 34048, 17664, 46336, 29952 },
  { 10752, 59904,  6656, 55808,  9728, 58880,  5632, 54784,
    10496, 59648,  6400, 55552,  9472, 58624,  5376, 54528 },
  { 43520, 27136, 39424, 23040, 42496, 26112, 38400, 22016,
    43264, 26880, 39168, 22784, 42240, 25856, 38144, 21760 }
};


/*
 * Helper function for TrueColorDither and renderXImage
 *
 * This handles the proper setting of the image data based on the image depth
 * and the machine's byte ordering
 */
static inline
void assignPixelData(unsigned int bit_depth, unsigned char **data,
		     unsigned long pixel) {
  unsigned char *pixel_data = *data;
  switch (bit_depth) {
  case  8: //  8bpp
    pixel_data[0] = pixel;
    ++pixel_data;
    break;

  case 16: // 16bpp LSB
    pixel_data[0] = pixel;
    pixel_data[1] = pixel >> 8;
    pixel_data += 2;
    break;

  case 17: // 16bpp MSB
    pixel_data[0] = pixel >> 8;
    pixel_data[1] = pixel;
    pixel_data += 2;
    break;

  case 24: // 24bpp LSB
    pixel_data[0] = pixel;
    pixel_data[1] = pixel >> 8;
    pixel_data[2] = pixel >> 16;
    pixel_data += 3;
    break;

  case 25: // 24bpp MSB
    pixel_data[0] = pixel >> 16;
    pixel_data[1] = pixel >> 8;
    pixel_data[2] = pixel;
    pixel_data += 3;
    break;

  case 32: // 32bpp LSB
    pixel_data[0] = pixel;
    pixel_data[1] = pixel >> 8;
    pixel_data[2] = pixel >> 16;
    pixel_data[3] = pixel >> 24;
    pixel_data += 4;
    break;

  case 33: // 32bpp MSB
    pixel_data[0] = pixel >> 24;
    pixel_data[1] = pixel >> 16;
    pixel_data[2] = pixel >> 8;
    pixel_data[3] = pixel;
    pixel_data += 4;
    break;
  }
  *data = pixel_data; // assign back so we don't lose our place
}


// algorithm: ordered dithering... many many thanks to rasterman
// (raster@rasterman.com) for telling me about this... portions of this
// code is based off of his code in Imlib
void bt::Image::OrderedDither(XColorTable *colortable,
                              unsigned int bit_depth,
                              unsigned int bytes_per_line,
                              unsigned char *pixel_data) {
  unsigned int x, y, dithx, dithy, r, g, b, error, offset;
  unsigned char *ppixel_data = pixel_data;

  unsigned int maxr = 255, maxg = 255, maxb = 255;
  colortable->map(maxr, maxg, maxb);

  for (y = 0, offset = 0; y < height; ++y) {
    dithy = y & 15;

    for (x = 0; x < width; ++x, ++offset) {
      dithx = x & 15;

      error = dither16[dithy][dithx];

      r = (((256 * maxr + maxr + 1) * data[offset].red   + error) / 65536);
      g = (((256 * maxg + maxg + 1) * data[offset].green + error) / 65536);
      b = (((256 * maxb + maxb + 1) * data[offset].blue  + error) / 65536);

      assignPixelData(bit_depth, &pixel_data, colortable->pixel(r, g, b));
    }

    pixel_data = (ppixel_data += bytes_per_line);
  }
}


void bt::Image::FloydSteinbergDither(XColorTable *colortable,
                                     unsigned int bit_depth,
                                     unsigned int bytes_per_line,
                                     unsigned char *pixel_data) {
  int *err[2][3], *terr;
  err[0][0] = new int[width + 2];
  err[0][1] = new int[width + 2];
  err[0][2] = new int[width + 2];
  err[1][0] = new int[width + 2];
  err[1][1] = new int[width + 2];
  err[1][2] = new int[width + 2];

  int rer, ger, ber;
  unsigned int x, y, r, g, b, offset;
  unsigned char *ppixel_data = pixel_data;

  unsigned int maxr = 255, maxg = 255, maxb = 255;
  colortable->map(maxr, maxg, maxb);
  maxr = 255u / maxr;
  maxg = 255u / maxg;
  maxb = 255u / maxb;

  for (x = 0; x < width; ++x) {
    err[0][0][x] = static_cast<int>(data[x].red  );
    err[0][1][x] = static_cast<int>(data[x].green);
    err[0][2][x] = static_cast<int>(data[x].blue );
  }

  err[0][0][x] = err[0][1][x] = err[0][2][x] = 0;

  for (y = 0, offset = 0; y < height; ++y) {
    if (y < (height - 1)) {
      for (x = 0; x < width; ++x) {
	err[1][0][x] = static_cast<int>(data[offset + width + x].red  );
	err[1][1][x] = static_cast<int>(data[offset + width + x].green);
	err[1][2][x] = static_cast<int>(data[offset + width + x].blue );
      }

      err[1][0][x] = err[1][0][x + 1];
      err[1][1][x] = err[1][1][x + 1];
      err[1][2][x] = err[1][2][x + 1];
    }

    for (x = 0; x < width; ++x) {
      r = static_cast<unsigned int>(std::max(std::min(err[0][0][x], 255), 0));
      g = static_cast<unsigned int>(std::max(std::min(err[0][1][x], 255), 0));
      b = static_cast<unsigned int>(std::max(std::min(err[0][2][x], 255), 0));

      colortable->map(r, g, b);
      assignPixelData(bit_depth, &pixel_data, colortable->pixel(r, g, b));

      rer = err[0][0][x] - static_cast<int>(r * maxr);
      ger = err[0][1][x] - static_cast<int>(g * maxg);
      ber = err[0][2][x] - static_cast<int>(b * maxb);

      err[0][0][x + 1]   += rer * 7 / 16;
      err[0][1][x + 1]   += ger * 7 / 16;
      err[0][2][x + 1]   += ber * 7 / 16;

      if (x > 0) {
        err[1][0][x - 1] += rer * 3 / 16;
        err[1][1][x - 1] += ger * 3 / 16;
        err[1][2][x - 1] += ber * 3 / 16;
      }

      err[1][0][x]       += rer * 5 / 16;
      err[1][1][x]       += ger * 5 / 16;
      err[1][2][x]       += ber * 5 / 16;

      err[1][0][x + 1]   += rer / 16;
      err[1][1][x + 1]   += ger / 16;
      err[1][2][x + 1]   += ber / 16;
    }

    offset += width;

    pixel_data = (ppixel_data += bytes_per_line);

    terr      = err[0][0];
    err[0][0] = err[1][0];
    err[1][0] = terr;

    terr      = err[0][1];
    err[0][1] = err[1][1];
    err[1][1] = terr;

    terr      = err[0][2];
    err[0][2] = err[1][2];
    err[1][2] = terr;
  }

  delete [] err[0][0];
  delete [] err[0][1];
  delete [] err[0][2];
  delete [] err[1][0];
  delete [] err[1][1];
  delete [] err[1][2];
}


Pixmap bt::Image::renderPixmap(const Display &display, unsigned int screen) {
  // get the colortable for the screen. if necessary, we will create one.
  if (colorTableList.empty())
    colorTableList.resize(display.screenCount(), 0);

  if (! colorTableList[screen]) {
    colorTableList[screen] =
      new XColorTable(display, screen, colorsPerChannel());
  }

  XColorTable *colortable = colorTableList[screen];
  const ScreenInfo &screeninfo = display.screenInfo(screen);
  XImage *image = 0;
  bool shm_ok = false;

#ifdef MITSHM
  // try to use MIT-SHM extension
  if (use_shm) {
    image = createShmImage(display, screeninfo, width, height);
    shm_ok = (image != 0);
  }
#endif // MITSHM

  if (! shm_ok) {
    // regular XImage
    image = XCreateImage(display.XDisplay(), screeninfo.visual(),
                         screeninfo.depth(), ZPixmap,
                         0, 0, width, height, 32, 0);
    if (! image)
      return None;

    buffer.reserve(image->bytes_per_line * (height + 1));
    image->data = reinterpret_cast<char *>(&buffer[0]);
  }

  unsigned char *d = reinterpret_cast<unsigned char *>(image->data);
  unsigned int o = image->bits_per_pixel +
                   ((image->byte_order == MSBFirst) ? 1 : 0);

  DitherMode dmode = NoDither;
  if (screeninfo.depth() < 24 && width > 1 && height > 1)
    dmode = ditherMode();

  // render to XImage
  switch (dmode) {
  case bt::FloydSteinbergDither:
    FloydSteinbergDither(colortable, o, image->bytes_per_line, d);
    break;

  case bt::OrderedDither:
    OrderedDither(colortable, o, image->bytes_per_line, d);
    break;

  case bt::NoDither: {
    unsigned int x, y, offset, r, g, b;
    unsigned char *pixel_data = d, *ppixel_data = d;

    for (y = 0, offset = 0; y < height; ++y) {
      for (x = 0; x < width; ++x, ++offset) {
        r = data[offset].red;
        g = data[offset].green;
        b = data[offset].blue;

        colortable->map(r, g, b);
        assignPixelData(o, &pixel_data, colortable->pixel(r, g, b));
      }

      pixel_data = (ppixel_data += image->bytes_per_line);
    }
    break;
  }
  } // switch dmode

  Pixmap pixmap = XCreatePixmap(display.XDisplay(), screeninfo.rootWindow(),
                                width, height, screeninfo.depth());
  if (pixmap == None) {
    image->data = 0;
    XDestroyImage(image);

    return None;
  }

  Pen pen(screen, Color(0, 0, 0));

#ifdef MITSHM
  if (shm_ok) {
    // use MIT-SHM extension
    XShmPutImage(display.XDisplay(), pixmap, pen.gc(), image,
                 0, 0, 0, 0, width, height, False);

    destroyShmImage(display, image);
  } else
#endif // MITSHM
    {
      // normal XPutImage
      XPutImage(display.XDisplay(), pixmap, pen.gc(), image,
                0, 0, 0, 0, width, height);

      image->data = 0;
      XDestroyImage(image);
    }

  return pixmap;
}


void bt::Image::bevel(unsigned int border_width) {
  if (width <= 2 || height <= 2 ||
      width <= (border_width * 4) || height <= (border_width * 4))
    return;

  RGB *p = data + (border_width * width) + border_width;
  unsigned int w = width - (border_width * 2);
  unsigned int h = height - (border_width * 2) - 2;
  unsigned char rr, gg, bb;

  // top of the bevel
  do {
    rr = p->red   + (p->red   >> 1);
    gg = p->green + (p->green >> 1);
    bb = p->blue  + (p->blue  >> 1);

    if (rr < p->red  ) rr = ~0;
    if (gg < p->green) gg = ~0;
    if (bb < p->blue ) bb = ~0;

    p->red = rr;
    p->green = gg;
    p->blue = bb;

    ++p;
  } while (--w);

  p += border_width + border_width;
  w = width - (border_width * 2);

  // left and right of the bevel
  do {
    rr = p->red   + (p->red   >> 1);
    gg = p->green + (p->green >> 1);
    bb = p->blue  + (p->blue  >> 1);

    if (rr < p->red) rr = ~0;
    if (gg < p->green) gg = ~0;
    if (bb < p->blue) bb = ~0;

    p->red = rr;
    p->green = gg;
    p->blue = bb;

    p += w - 1;

    rr = (p->red   >> 2) + (p->red   >> 1);
    gg = (p->green >> 2) + (p->green >> 1);
    bb = (p->blue  >> 2) + (p->blue  >> 1);

    if (rr > p->red  ) rr = 0;
    if (gg > p->green) gg = 0;
    if (bb > p->blue ) bb = 0;

    p->red   = rr;
    p->green = gg;
    p->blue  = bb;

    p += border_width + border_width + 1;
  } while (--h);

  w = width - (border_width * 2);

  // bottom of the bevel
  do {
    rr = (p->red   >> 2) + (p->red   >> 1);
    gg = (p->green >> 2) + (p->green >> 1);
    bb = (p->blue  >> 2) + (p->blue  >> 1);

    if (rr > p->red  ) rr = 0;
    if (gg > p->green) gg = 0;
    if (bb > p->blue ) bb = 0;

    p->red   = rr;
    p->green = gg;
    p->blue  = bb;

    ++p;
  } while (--w);
}


void bt::Image::dgradient(const Color &from, const Color &to,
                          bool interlaced) {
  // diagonal gradient code was written by Mike Cole <mike@mydot.com>
  // modified for interlacing by Brad Hughes

  double drx, dgx, dbx, dry, dgy, dby;
  double yr = 0.0, yg = 0.0, yb = 0.0,
         xr = static_cast<double>(from.red()),
         xg = static_cast<double>(from.green()),
         xb = static_cast<double>(from.blue());

  RGB *p = data;
  unsigned int w = width * 2, h = height * 2;
  unsigned int xt[maximumWidth][3], yt[maximumHeight][3];

  unsigned int x, y;

  dry = drx = static_cast<double>(to.red()   - from.red());
  dgy = dgx = static_cast<double>(to.green() - from.green());
  dby = dbx = static_cast<double>(to.blue()  - from.blue());

  // Create X table
  drx /= w;
  dgx /= w;
  dbx /= w;

  for (x = 0; x < width; ++x) {
    xt[x][0] = static_cast<unsigned char>(xr);
    xt[x][1] = static_cast<unsigned char>(xg);
    xt[x][2] = static_cast<unsigned char>(xb);

    xr += drx;
    xg += dgx;
    xb += dbx;
  }

  // Create Y table
  dry /= h;
  dgy /= h;
  dby /= h;

  for (y = 0; y < height; ++y) {
    yt[y][0] = static_cast<unsigned char>(yr);
    yt[y][1] = static_cast<unsigned char>(yg);
    yt[y][2] = static_cast<unsigned char>(yb);

    yr += dry;
    yg += dgy;
    yb += dby;
  }

  // Combine tables to create gradient

  if (! interlaced) {
    // normal dgradient
    for (y = 0; y < height; ++y) {
      for (x = 0; x < width; ++x, ++p) {
        p->red   = xt[x][0] + yt[y][0];
        p->green = xt[x][1] + yt[y][1];
        p->blue  = xt[x][2] + yt[y][2];
      }
    }
    return;
  }

  // interlacing effect
  for (y = 0; y < height; ++y) {
    for (x = 0; x < width; ++x, ++p) {
      p->red   = xt[x][0] + yt[y][0];
      p->green = xt[x][1] + yt[y][1];
      p->blue  = xt[x][2] + yt[y][2];

      if (y & 1) {
        p->red   = (p->red   >> 1) + (p->red   >> 2);
        p->green = (p->green >> 1) + (p->green >> 2);
        p->blue  = (p->blue  >> 1) + (p->blue  >> 2);
      }
    }
  }
}


void bt::Image::hgradient(const Color &from, const Color &to,
                          bool interlaced) {
  double drx, dgx, dbx,
    xr = static_cast<double>(from.red()),
    xg = static_cast<double>(from.green()),
    xb = static_cast<double>(from.blue());
  RGB *p = data;
  unsigned int total = width * (height - 2);
  unsigned int x;

  drx = static_cast<double>(to.red()   - from.red());
  dgx = static_cast<double>(to.green() - from.green());
  dbx = static_cast<double>(to.blue()  - from.blue());

  drx /= width;
  dgx /= width;
  dbx /= width;

  // second line
  if (interlaced && height > 1) {
    // interlacing effect

    // first line
    for (x = 0; x < width; ++x, ++p) {
      p->red   = static_cast<unsigned char>(xr);
      p->green = static_cast<unsigned char>(xg);
      p->blue  = static_cast<unsigned char>(xb);

      xr += drx;
      xg += dgx;
      xb += dbx;
    }

    // second line
    xr = static_cast<double>(from.red()),
    xg = static_cast<double>(from.green()),
    xb = static_cast<double>(from.blue());

    for (x = 0; x < width; ++x, ++p) {
      p->red   = static_cast<unsigned char>(xr);
      p->green = static_cast<unsigned char>(xg);
      p->blue  = static_cast<unsigned char>(xb);

      p->red   = (p->red   >> 1) + (p->red   >> 2);
      p->green = (p->green >> 1) + (p->green >> 2);
      p->blue  = (p->blue  >> 1) + (p->blue  >> 2);

      xr += drx;
      xg += dgx;
      xb += dbx;
    }
  } else {
    // first line
    for (x = 0; x < width; ++x, ++p) {
      p->red   = static_cast<unsigned char>(xr);
      p->green = static_cast<unsigned char>(xg);
      p->blue  = static_cast<unsigned char>(xb);

      xr += drx;
      xg += dgx;
      xb += dbx;
    }

    if (height > 1) {
      // second line
      memcpy(p, data, width);
      p += width;
    }
  }

  // rest of the gradient
  for (x = 0; x < total; ++x)
    p[x] = data[x];
}


void bt::Image::vgradient(const Color &from, const Color &to,
                          bool interlaced) {
  double dry, dgy, dby,
    yr = static_cast<double>(from.red()  ),
    yg = static_cast<double>(from.green()),
    yb = static_cast<double>(from.blue() );
  RGB *p = data;
  unsigned int x, y;

  dry = static_cast<double>(to.red()   - from.red()  );
  dgy = static_cast<double>(to.green() - from.green());
  dby = static_cast<double>(to.blue()  - from.blue() );

  dry /= height;
  dgy /= height;
  dby /= height;

  if (interlaced) {
    // faked interlacing effect
    for (y = 0; y < height; ++y) {
      p->red   = static_cast<unsigned char>(yr);
      p->green = static_cast<unsigned char>(yg);
      p->blue  = static_cast<unsigned char>(yb);

      if (y & 1) {
        p->red   = (p->red   >> 1) + (p->red   >> 2);
        p->green = (p->green >> 1) + (p->green >> 2);
        p->blue  = (p->blue  >> 1) + (p->blue  >> 2);
      }

      for (x = 0; x < width; ++x) *p++ = *data;

      yr += dry;
      yg += dgy;
      yb += dby;
    }
  } else {
    // normal vgradient
    for (y = 0; y < height; ++y) {
      p->red   = static_cast<unsigned char>(yr);
      p->green = static_cast<unsigned char>(yg);
      p->blue  = static_cast<unsigned char>(yb);

      for (x = 0; x < width; ++x) *p++ = *data;

      yr += dry;
      yg += dgy;
      yb += dby;
    }
  }
}


void bt::Image::pgradient(const Color &from, const Color &to,
                          bool interlaced) {
  // pyramid gradient -  based on original dgradient, written by
  // Mosfet (mosfet@kde.org)
  // adapted from kde sources for Blackbox by Brad Hughes

  double yr, yg, yb, drx, dgx, dbx, dry, dgy, dby, xr, xg, xb;
  int rsign, gsign, bsign;
  RGB *p = data;
  unsigned int tr = to.red(), tg = to.green(), tb = to.blue();
  unsigned int xt[maximumWidth][3], yt[maximumHeight][3];
  unsigned int x, y;

  dry = drx = static_cast<double>(to.red()   - from.red());
  dgy = dgx = static_cast<double>(to.green() - from.green());
  dby = dbx = static_cast<double>(to.blue()  - from.blue());

  rsign = (drx < 0) ? -1 : 1;
  gsign = (dgx < 0) ? -1 : 1;
  bsign = (dbx < 0) ? -1 : 1;

  xr = yr = (drx / 2);
  xg = yg = (dgx / 2);
  xb = yb = (dbx / 2);

  // Create X table
  drx /= width;
  dgx /= width;
  dbx /= width;

  for (x = 0; x < width; ++x) {
    xt[x][0] = static_cast<unsigned char>(fabs(xr));
    xt[x][1] = static_cast<unsigned char>(fabs(xg));
    xt[x][2] = static_cast<unsigned char>(fabs(xb));

    xr -= drx;
    xg -= dgx;
    xb -= dbx;
  }

  // Create Y table
  dry /= height;
  dgy /= height;
  dby /= height;

  for (y = 0; y < height; ++y) {
    yt[y][0] = static_cast<unsigned char>(fabs(yr));
    yt[y][1] = static_cast<unsigned char>(fabs(yg));
    yt[y][2] = static_cast<unsigned char>(fabs(yb));

    yr -= dry;
    yg -= dgy;
    yb -= dby;
  }

  // Combine tables to create gradient

  if (! interlaced) {
    // normal pgradient
    for (y = 0; y < height; ++y) {
      for (x = 0; x < width; ++x, ++p) {
        p->red =
          static_cast<unsigned char>(tr - (rsign * (xt[x][0] + yt[y][0])));
        p->green =
          static_cast<unsigned char>(tg - (gsign * (xt[x][1] + yt[y][1])));
        p->blue =
          static_cast<unsigned char>(tb - (bsign * (xt[x][2] + yt[y][2])));
      }
    }
    return;
  }

  // interlacing effect
  for (y = 0; y < height; ++y) {
    for (x = 0; x < width; ++x, ++p) {
      p->red =
        static_cast<unsigned char>(tr - (rsign * (xt[x][0] + yt[y][0])));
      p->green =
        static_cast<unsigned char>(tg - (gsign * (xt[x][1] + yt[y][1])));
      p->blue =
        static_cast<unsigned char>(tb - (bsign * (xt[x][2] + yt[y][2])));

      if (y & 1) {
        p->red   = (p->red   >> 1) + (p->red   >> 2);
        p->green = (p->green >> 1) + (p->green >> 2);
        p->blue  = (p->blue  >> 1) + (p->blue  >> 2);
      }
    }
  }
}


void bt::Image::rgradient(const Color &from, const Color &to,
                          bool interlaced) {
  // rectangle gradient -  based on original dgradient, written by
  // Mosfet (mosfet@kde.org)
  // adapted from kde sources for Blackbox by Brad Hughes

  double drx, dgx, dbx, dry, dgy, dby, xr, xg, xb, yr, yg, yb;
  int rsign, gsign, bsign;
  RGB *p = data;
  unsigned int tr = to.red(), tg = to.green(), tb = to.blue();
  unsigned int xt[maximumWidth][3], yt[maximumHeight][3];
  unsigned int x, y;

  dry = drx = static_cast<double>(to.red()   - from.red());
  dgy = dgx = static_cast<double>(to.green() - from.green());
  dby = dbx = static_cast<double>(to.blue()  - from.blue());

  rsign = (drx < 0) ? -2 : 2;
  gsign = (dgx < 0) ? -2 : 2;
  bsign = (dbx < 0) ? -2 : 2;

  xr = yr = (drx / 2);
  xg = yg = (dgx / 2);
  xb = yb = (dbx / 2);

  // Create X table
  drx /= width;
  dgx /= width;
  dbx /= width;

  for (x = 0; x < width; ++x) {
    xt[x][0] = static_cast<unsigned char>(fabs(xr));
    xt[x][1] = static_cast<unsigned char>(fabs(xg));
    xt[x][2] = static_cast<unsigned char>(fabs(xb));

    xr -= drx;
    xg -= dgx;
    xb -= dbx;
  }

  // Create Y table
  dry /= height;
  dgy /= height;
  dby /= height;

  for (y = 0; y < height; ++y) {
    yt[y][0] = static_cast<unsigned char>(fabs(yr));
    yt[y][1] = static_cast<unsigned char>(fabs(yg));
    yt[y][2] = static_cast<unsigned char>(fabs(yb));

    yr -= dry;
    yg -= dgy;
    yb -= dby;
  }

  // Combine tables to create gradient

  if (! interlaced) {
    // normal rgradient
    for (y = 0; y < height; ++y) {
      for (x = 0; x < width; ++x, ++p) {
        p->red =
          static_cast<unsigned char>(tr - (rsign *
                                           std::max(xt[x][0], yt[y][0])));
        p->green =
          static_cast<unsigned char>(tg - (gsign *
                                           std::max(xt[x][1], yt[y][1])));
        p->blue =
          static_cast<unsigned char>(tb - (bsign *
                                           std::max(xt[x][2], yt[y][2])));
      }
    }
    return;
  }

  // interlacing effect
  for (y = 0; y < height; ++y) {
    for (x = 0; x < width; ++x, ++p) {
      p->red =
        static_cast<unsigned char>(tr - (rsign *
                                         std::max(xt[x][0], yt[y][0])));
      p->green =
        static_cast<unsigned char>(tg - (gsign *
                                         std::max(xt[x][1], yt[y][1])));
      p->blue =
        static_cast<unsigned char>(tb - (bsign *
                                         std::max(xt[x][2], yt[y][2])));

      if (y & 1) {
        p->red   = (p->red   >> 1) + (p->red   >> 2);
        p->green = (p->green >> 1) + (p->green >> 2);
        p->blue  = (p->blue  >> 1) + (p->blue  >> 2);
      }
    }
  }
}


void bt::Image::egradient(const Color &from, const Color &to,
                          bool interlaced) {
  // elliptic gradient -  based on original dgradient, written by
  // Mosfet (mosfet@kde.org)
  // adapted from kde sources for Blackbox by Brad Hughes

  double drx, dgx, dbx, dry, dgy, dby, yr, yg, yb, xr, xg, xb;
  int rsign, gsign, bsign;
  RGB *p = data;
  unsigned int tr = to.red(), tg = to.green(), tb = to.blue();
  unsigned int xt[maximumWidth][3], yt[maximumHeight][3];
  unsigned int x, y;

  dry = drx = static_cast<double>(to.red() - from.red());
  dgy = dgx = static_cast<double>(to.green() - from.green());
  dby = dbx = static_cast<double>(to.blue() - from.blue());

  rsign = (drx < 0) ? -1 : 1;
  gsign = (dgx < 0) ? -1 : 1;
  bsign = (dbx < 0) ? -1 : 1;

  xr = yr = (drx / 2);
  xg = yg = (dgx / 2);
  xb = yb = (dbx / 2);

  // Create X table
  drx /= width;
  dgx /= width;
  dbx /= width;

  for (x = 0; x < width; x++) {
    xt[x][0] = static_cast<unsigned int>(xr * xr);
    xt[x][1] = static_cast<unsigned int>(xg * xg);
    xt[x][2] = static_cast<unsigned int>(xb * xb);

    xr -= drx;
    xg -= dgx;
    xb -= dbx;
  }

  // Create Y table
  dry /= height;
  dgy /= height;
  dby /= height;

  for (y = 0; y < height; y++) {
    yt[y][0] = static_cast<unsigned int>(yr * yr);
    yt[y][1] = static_cast<unsigned int>(yg * yg);
    yt[y][2] = static_cast<unsigned int>(yb * yb);

    yr -= dry;
    yg -= dgy;
    yb -= dby;
  }

  // Combine tables to create gradient

  if (! interlaced) {
    // normal egradient
    for (y = 0; y < height; ++y) {
      for (x = 0; x < width; ++x, ++p) {
        p->red   = static_cast<unsigned char>
                   (tr - (rsign * static_cast<int>(sqrt(xt[x][0] +
                                                        yt[y][0]))));
        p->green = static_cast<unsigned char>
                   (tg - (gsign * static_cast<int>(sqrt(xt[x][1] +
                                                        yt[y][1]))));
        p->blue  = static_cast<unsigned char>
                   (tb - (bsign * static_cast<int>(sqrt(xt[x][2] +
                                                        yt[y][2]))));
      }
    }
    return;
  }

  // interlacing effect
  for (y = 0; y < height; ++y) {
    for (x = 0; x < width; ++x, ++p) {
      p->red   = static_cast<unsigned char>
                 (tr - (rsign * static_cast<int>(sqrt(xt[x][0] + yt[y][0]))));
      p->green = static_cast<unsigned char>
                 (tg - (gsign * static_cast<int>(sqrt(xt[x][1] + yt[y][1]))));
      p->blue  = static_cast<unsigned char>
                 (tb - (bsign * static_cast<int>(sqrt(xt[x][2] + yt[y][2]))));

      if (y & 1) {
        p->red   = (p->red   >> 1) + (p->red   >> 2);
        p->green = (p->green >> 1) + (p->green >> 2);
        p->blue  = (p->blue  >> 1) + (p->blue  >> 2);
      }
    }
  }
}


void bt::Image::pcgradient(const Color &from, const Color &to,
                           bool interlaced) {
  // pipe cross gradient -  based on original dgradient, written by
  // Mosfet (mosfet@kde.org)
  // adapted from kde sources for Blackbox by Brad Hughes

  double drx, dgx, dbx, dry, dgy, dby, xr, xg, xb, yr, yg, yb;
  int rsign, gsign, bsign;
  RGB *p = data;
  unsigned int tr = to.red(), tg = to.green(), tb = to.blue();
  unsigned int xt[maximumWidth][3], yt[maximumHeight][3];
  unsigned int x, y;

  dry = drx = static_cast<double>(to.red()   - from.red());
  dgy = dgx = static_cast<double>(to.green() - from.green());
  dby = dbx = static_cast<double>(to.blue()  - from.blue());

  rsign = (drx < 0) ? -2 : 2;
  gsign = (dgx < 0) ? -2 : 2;
  bsign = (dbx < 0) ? -2 : 2;

  xr = yr = (drx / 2);
  xg = yg = (dgx / 2);
  xb = yb = (dbx / 2);

  // Create X table
  drx /= width;
  dgx /= width;
  dbx /= width;

  for (x = 0; x < width; ++x) {
    xt[x][0] = static_cast<unsigned char>(fabs(xr));
    xt[x][1] = static_cast<unsigned char>(fabs(xg));
    xt[x][2] = static_cast<unsigned char>(fabs(xb));

    xr -= drx;
    xg -= dgx;
    xb -= dbx;
  }

  // Create Y table
  dry /= height;
  dgy /= height;
  dby /= height;

  for (y = 0; y < height; ++y) {
    yt[y][0] = static_cast<unsigned char>(fabs(yr));
    yt[y][1] = static_cast<unsigned char>(fabs(yg));
    yt[y][2] = static_cast<unsigned char>(fabs(yb));

    yr -= dry;
    yg -= dgy;
    yb -= dby;
  }

  // Combine tables to create gradient

  if (! interlaced) {
    // normal rgradient
    for (y = 0; y < height; ++y) {
      for (x = 0; x < width; ++x, ++p) {
        p->red =
          static_cast<unsigned char>(tr - (rsign *
                                           std::min(xt[x][0], yt[y][0])));
        p->green =
          static_cast<unsigned char>(tg - (gsign *
                                           std::min(xt[x][1], yt[y][1])));
        p->blue =
          static_cast<unsigned char>(tb - (bsign *
                                           std::min(xt[x][2], yt[y][2])));
      }
    }
    return;
  }

  // interlacing effect
  for (y = 0; y < height; ++y) {
    for (x = 0; x < width; ++x, ++p) {
      p->red =
        static_cast<unsigned char>(tr - (rsign *
                                         std::min(xt[x][0], yt[y][0])));
      p->green =
        static_cast<unsigned char>(tg - (gsign *
                                         std::min(xt[x][1], yt[y][1])));
      p->blue =
        static_cast<unsigned char>(tb - (bsign *
                                         std::min(xt[x][2], yt[y][2])));

      if (y & 1) {
        p->red   = (p->red   >> 1) + (p->red   >> 2);
        p->green = (p->green >> 1) + (p->green >> 2);
        p->blue  = (p->blue  >> 1) + (p->blue  >> 2);
      }
    }
  }
}


void bt::Image::cdgradient(const Color &from, const Color &to,
                           bool interlaced) {
  // cross diagonal gradient -  based on original dgradient, written by
  // Mosfet (mosfet@kde.org)
  // adapted from kde sources for Blackbox by Brad Hughes

  double drx, dgx, dbx, dry, dgy, dby;
  double yr = 0.0, yg = 0.0, yb = 0.0,
         xr = static_cast<double>(from.red()  ),
         xg = static_cast<double>(from.green()),
         xb = static_cast<double>(from.blue() );
  RGB *p = data;
  unsigned int w = width * 2, h = height * 2;
  unsigned int xt[maximumWidth][3], yt[maximumHeight][3];
  unsigned int x, y;

  dry = drx = static_cast<double>(to.red()   - from.red()  );
  dgy = dgx = static_cast<double>(to.green() - from.green());
  dby = dbx = static_cast<double>(to.blue()  - from.blue() );

  // Create X table
  drx /= w;
  dgx /= w;
  dbx /= w;

  for (x = width - 1; x != 0; --x) {
    xt[x][0] = static_cast<unsigned char>(xr);
    xt[x][1] = static_cast<unsigned char>(xg);
    xt[x][2] = static_cast<unsigned char>(xb);

    xr += drx;
    xg += dgx;
    xb += dbx;
  }

  xt[x][0] = static_cast<unsigned char>(xr);
  xt[x][1] = static_cast<unsigned char>(xg);
  xt[x][2] = static_cast<unsigned char>(xb);

  // Create Y table
  dry /= h;
  dgy /= h;
  dby /= h;

  for (y = 0; y < height; ++y) {
    yt[y][0] = static_cast<unsigned char>(yr);
    yt[y][1] = static_cast<unsigned char>(yg);
    yt[y][2] = static_cast<unsigned char>(yb);

    yr += dry;
    yg += dgy;
    yb += dby;
  }

  // Combine tables to create gradient

  if (! interlaced) {
    // normal dgradient
    for (y = 0; y < height; ++y) {
      for (x = 0; x < width; ++x, ++p) {
        p->red   = xt[x][0] + yt[y][0];
        p->green = xt[x][1] + yt[y][1];
        p->blue  = xt[x][2] + yt[y][2];
      }
    }
    return;
  }

  // interlacing effect
  for (y = 0; y < height; ++y) {
    for (x = 0; x < width; ++x, ++p) {
      p->red   = xt[x][0] + yt[y][0];
      p->green = xt[x][1] + yt[y][1];
      p->blue  = xt[x][2] + yt[y][2];

      if (y & 1) {
        p->red   = (p->red   >> 1) + (p->red   >> 2);
        p->green = (p->green >> 1) + (p->green >> 2);
        p->blue  = (p->blue  >> 1) + (p->blue  >> 2);
      }
    }
  }
}
