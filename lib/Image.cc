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

#include "Image.hh"
#include "Display.hh"
#include "Pen.hh"
#include "Texture.hh"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#ifdef    MITSHM
#  include <sys/types.h>
#  include <sys/ipc.h>
#  include <sys/shm.h>
#  include <unistd.h>
#  include <X11/extensions/XShm.h>
#endif // MITSHM
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <algorithm>
#include <vector>

// #define COLORTABLE_DEBUG
// #define MITSHM_DEBUG


unsigned int bt::Image::global_maximumColors = 126; // 6/7/3
bt::DitherMode bt::Image::global_ditherMode = bt::OrderedDither;


namespace bt {

  class XColorTable {
  public:
    XColorTable(const Display &dpy, unsigned int screen,
                unsigned int ncolors);
    ~XColorTable(void);

    inline bt::DitherMode ditherMode(void) const
    { return ((_nred < 256u || _ngreen < 256u || _nblue < 256u)
              ? bt::Image::ditherMode()
              : bt::NoDither); }

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
    unsigned int _nred, _ngreen, _nblue;
    int red_offset, green_offset, blue_offset;

    unsigned short _red[256];
    unsigned short _green[256];
    unsigned short _blue[256];
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
                             unsigned int ncolors)
  : _dpy(dpy), _screen(screen),
    _nred(0u), _ngreen(0u), _nblue(0u),
    red_offset(0u), green_offset(0u), blue_offset(0u)
{
  const ScreenInfo &screeninfo = _dpy.screenInfo(_screen);
  Visual *visual = screeninfo.visual();
  unsigned int depth = screeninfo.depth();
  double red_bits = 0., green_bits = 0., blue_bits = 0.;

  _vclass = screeninfo.visual()->c_class;

  switch (_vclass) {
  case StaticGray:
    _nred = _ngreen = _nblue = DisplayCells(_dpy.XDisplay(), screen);
    red_bits = green_bits = blue_bits = 255. / (_nred - 1);
    break;

  case StaticColor:
    // use all (and assume) 256 colors: 8 red, 8 green, 4 blue
    _nred   = 8u;
    _ngreen = 8u;
    _nblue  = 4u;
    colors.resize(256u);
    red_bits   = 255. / (_nred   - 1u);
    green_bits = 255. / (_ngreen - 1u);
    blue_bits  = 255. / (_nblue  - 1u);
    break;

  case GrayScale:
  case PseudoColor: {
    switch (ncolors) {
    case 216u:
      _nred   = 6u;
      _ngreen = 6u;
      _nblue  = 6u;
      break;

    default:
      // calculate 2:2:1 proportions
      _nred   = 2u;
      _ngreen = 2u;
      _nblue  = 2u;
      for (;;) {
        if ((_nblue * 2u) < _nred
            && (_nblue + 1u) * _nred * _ngreen <= ncolors)
          ++_nblue;
        else if (_nred < _ngreen
                 && _nblue * (_nred + 1u) * _ngreen <= ncolors)
          ++_nred;
        else if (_nblue * _nred * (_ngreen + 1u) <= ncolors)
          ++_ngreen;
        else break;
      }
      ncolors = _nred * _ngreen * _nblue;
      break;
    } //switch

    colors.resize(ncolors);
    red_bits   = 255. / (_nred   - 1u);
    green_bits = 255. / (_ngreen - 1u);
    blue_bits  = 255. / (_nblue  - 1u);
    break;
  }

  case TrueColor:
  case DirectColor: {
    // compute color tables
    unsigned long red_mask = visual->red_mask,
                green_mask = visual->green_mask,
                 blue_mask = visual->blue_mask;

    while (! (  red_mask & 1ul)) {   red_offset++;   red_mask >>= 1ul; }
    while (! (green_mask & 1ul)) { green_offset++; green_mask >>= 1ul; }
    while (! ( blue_mask & 1ul)) {  blue_offset++;  blue_mask >>= 1ul; }

    _nred   = static_cast<unsigned int>(  red_mask + 1ul);
    _ngreen = static_cast<unsigned int>(green_mask + 1ul);
    _nblue  = static_cast<unsigned int>( blue_mask + 1ul);
    red_bits   = 255. /   red_mask;
    green_bits = 255. / green_mask;
    blue_bits  = 255. /  blue_mask;

    break;
  }
  } // switch

  // initialize color tables
  unsigned int i;
  for (i = 0; i < 256u; i++) {
    _red[i]   = static_cast<unsigned short>(rint(i / red_bits));
    _green[i] = static_cast<unsigned short>(rint(i / green_bits));
    _blue[i]  = static_cast<unsigned short>(rint(i / blue_bits));
  }

  if (! colors.empty()) {
    const Colormap colormap = screeninfo.colormap();
    bool need_query = false;

    // create color cube
    unsigned int ii, p, r, g, b;
    for (r = 0, i = 0; r < _nred; r++) {
      for (g = 0; g < _ngreen; g++) {
     	for (b = 0; b < _nblue; b++, i++) {
     	  colors[i].red   = ((r * 0xff) / (_nred   - 1)) * 0x101;
     	  colors[i].green = ((g * 0xff) / (_ngreen - 1)) * 0x101;
     	  colors[i].blue  = ((b * 0xff) / (_nblue  - 1)) * 0x101;

          if ((_vclass & 1)
              && XAllocColor(_dpy.XDisplay(), colormap, &colors[i])) {
            colors[i].flags = DoRed|DoGreen|DoBlue;
          } else {
            colors[i].flags = 0;
            need_query = true;
          }

#ifdef COLORTABLE_DEBUG
          fprintf(stderr, "%s %3u: %02x/%02x/%02x %.2f/%.2f/%.2f (%lu)\n",
                  colors[i].flags == 0 ? "req  " : "alloc",
                  i,
                  (colors[i].red   >> 8),
                  (colors[i].green >> 8),
                  (colors[i].blue  >> 8),
                  (colors[i].red   >> 8) / red_bits,
                  (colors[i].green >> 8) / green_bits,
                  (colors[i].blue  >> 8) / blue_bits,
                  colors[i].pixel);
#endif // COLORTABLE_DEBUG
     	}
      }
    }

    if (!need_query)
      return;

    // query existing colormap
    unsigned int incolors = (((1u << depth) > 256u) ? 256u : (1u << depth));
    XColor icolors[256];
    for (i = 0; i < incolors; i++) icolors[i].pixel = i;
    XQueryColors(_dpy.XDisplay(), colormap, icolors, incolors);

#ifdef COLORTABLE_DEBUG
    for (i = 0; i < incolors; ++i) {
      fprintf(stderr, "query %3u: %02x/%02x/%02x %.2f/%.2f/%.2f flags %x\n", i,
              (icolors[i].red   >> 8),
              (icolors[i].green >> 8),
              (icolors[i].blue  >> 8),
              (icolors[i].red   >> 8) / red_bits,
              (icolors[i].green >> 8) / green_bits,
              (icolors[i].blue  >> 8) / blue_bits,
              icolors[i].flags);
    }
#endif // COLORTABLE_DEBUG

    // for missing colors, find the closest color in the existing colormap
    for (i = 0; i < colors.size(); i++) {
      if (colors[i].flags != 0) continue;

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
          fprintf(stderr, "close %3u: %02x/%02x/%02x %.2f/%.2f/%.2f (%lu)\n",
                  i,
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
    return (red+green+blue)/3u;
    // return std::max(red, std::max(green, blue));

  case GrayScale:
  case StaticColor:
  case PseudoColor:
    return colors[(red * _ngreen * _nblue) + (green * _nblue) + blue].pixel;

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
  : data(0), width(w), height(h) {
  assert(width > 0);
  assert(height > 0);
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

  data = static_cast<RGB *>(malloc(width * height * sizeof(RGB)));

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
      XDrawRectangle(penborder.XDisplay(), pixmap, penborder.gc(),
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
static void assignPixelData(unsigned int bit_depth, unsigned char **data,
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
  int * const error = new int[width * 6];
  int * const r_line1 = error + (width * 0);
  int * const g_line1 = error + (width * 1);
  int * const b_line1 = error + (width * 2);
  int * const r_line2 = error + (width * 3);
  int * const g_line2 = error + (width * 4);
  int * const b_line2 = error + (width * 5);

  int rer, ger, ber;
  unsigned int x, y, r, g, b, offset;
  unsigned char *ppixel_data = pixel_data;
  RGB *pixels = new RGB[width];

  unsigned int maxr = 255, maxg = 255, maxb = 255;
  colortable->map(maxr, maxg, maxb);
  maxr = 255u / maxr;
  maxg = 255u / maxg;
  maxb = 255u / maxb;

  for (y = 0, offset = 0; y < height; ++y) {
    const bool reverse = bool(y & 1);

    int * const rl1 = (reverse) ? r_line2 : r_line1;
    int * const gl1 = (reverse) ? g_line2 : g_line1;
    int * const bl1 = (reverse) ? b_line2 : b_line1;
    int * const rl2 = (reverse) ? r_line1 : r_line2;
    int * const gl2 = (reverse) ? g_line1 : g_line2;
    int * const bl2 = (reverse) ? b_line1 : b_line2;

    if (y == 0) {
      for (x = 0; x < width; ++x) {
        rl1[x] = static_cast<int>(data[x].red);
        gl1[x] = static_cast<int>(data[x].green);
        bl1[x] = static_cast<int>(data[x].blue);
      }
    }
    if (y+1 < height) {
      for (x = 0; x < width; ++x) {
        rl2[x] = static_cast<int>(data[offset + width + x].red);
        gl2[x] = static_cast<int>(data[offset + width + x].green);
        bl2[x] = static_cast<int>(data[offset + width + x].blue);
      }
    }

    // bi-directional dither
    if (reverse) {
      for (x = 0; x < width; ++x) {
        r = static_cast<unsigned int>(std::max(std::min(rl1[x], 255), 0));
        g = static_cast<unsigned int>(std::max(std::min(gl1[x], 255), 0));
        b = static_cast<unsigned int>(std::max(std::min(bl1[x], 255), 0));
        colortable->map(r, g, b);
        pixels[x].red   = r;
        pixels[x].green = g;
        pixels[x].blue  = b;

        rer = rl1[x] - static_cast<int>(r * maxr);
        ger = gl1[x] - static_cast<int>(g * maxg);
        ber = bl1[x] - static_cast<int>(b * maxb);

        if (x+1 < width) {
          rl1[x+1] += rer * 7 / 16;
          gl1[x+1] += ger * 7 / 16;
          bl1[x+1] += ber * 7 / 16;
          rl2[x+1] += rer * 1 / 16;
          gl2[x+1] += ger * 1 / 16;
          bl2[x+1] += ber * 1 / 16;
        }
        rl2[x] += rer * 5 / 16;
        gl2[x] += ger * 5 / 16;
        bl2[x] += ber * 5 / 16;
        if (x > 0) {
          rl2[x-1] += rer * 3 / 16;
          gl2[x-1] += ger * 3 / 16;
          bl2[x-1] += ber * 3 / 16;
        }
      }
    } else {
      for (x = width; x-- > 0; ) {
        r = static_cast<unsigned int>(std::max(std::min(rl1[x], 255), 0));
        g = static_cast<unsigned int>(std::max(std::min(gl1[x], 255), 0));
        b = static_cast<unsigned int>(std::max(std::min(bl1[x], 255), 0));
        colortable->map(r, g, b);
        pixels[x].red   = r;
        pixels[x].green = g;
        pixels[x].blue  = b;

        rer = rl1[x] - static_cast<int>(r * maxr);
        ger = gl1[x] - static_cast<int>(g * maxg);
        ber = bl1[x] - static_cast<int>(b * maxb);

        if (x > 0) {
          rl1[x-1] += rer * 7 / 16;
          gl1[x-1] += ger * 7 / 16;
          bl1[x-1] += ber * 7 / 16;
          rl2[x-1] += rer * 1 / 16;
          gl2[x-1] += ger * 1 / 16;
          bl2[x-1] += ber * 1 / 16;
        }
        rl2[x] += rer * 5 / 16;
        gl2[x] += ger * 5 / 16;
        bl2[x] += ber * 5 / 16;
        if (x+1 < width) {
          rl2[x+1] += rer * 3 / 16;
          gl2[x+1] += ger * 3 / 16;
          bl2[x+1] += ber * 3 / 16;
        }
      }
    }
    for (x = 0; x < width; ++x) {
      r = pixels[x].red;
      g = pixels[x].green;
      b = pixels[x].blue;
      assignPixelData(bit_depth, &pixel_data, colortable->pixel(r, g, b));
    }

    offset += width;
    pixel_data = (ppixel_data += bytes_per_line);
  }

  delete [] error;
  delete [] pixels;
}


Pixmap bt::Image::renderPixmap(const Display &display, unsigned int screen) {
  // get the colortable for the screen. if necessary, we will create one.
  if (colorTableList.empty())
    colorTableList.resize(display.screenCount(), 0);

  if (! colorTableList[screen]) {
    colorTableList[screen] =
      new XColorTable(display, screen, maximumColors());
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

  DitherMode dmode =
    (width > 1 && height > 1) ? colortable->ditherMode() : NoDither;

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
    XShmPutImage(pen.XDisplay(), pixmap, pen.gc(), image,
                 0, 0, 0, 0, width, height, False);

    destroyShmImage(display, image);
  } else
#endif // MITSHM
    {
      // normal XPutImage
      XPutImage(pen.XDisplay(), pixmap, pen.gc(), image,
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
  unsigned int x, y;

  const unsigned int dimension = std::max(width, height);
  unsigned int *alloc = new unsigned int[dimension * 6];
  unsigned int *xt[3], *yt[3];
  xt[0] = alloc + (dimension * 0);
  xt[1] = alloc + (dimension * 1);
  xt[2] = alloc + (dimension * 2);
  yt[0] = alloc + (dimension * 3);
  yt[1] = alloc + (dimension * 4);
  yt[2] = alloc + (dimension * 5);

  dry = drx = static_cast<double>(to.red()   - from.red());
  dgy = dgx = static_cast<double>(to.green() - from.green());
  dby = dbx = static_cast<double>(to.blue()  - from.blue());

  // Create X table
  drx /= w;
  dgx /= w;
  dbx /= w;

  for (x = 0; x < width; ++x) {
    xt[0][x] = static_cast<unsigned char>(xr);
    xt[1][x] = static_cast<unsigned char>(xg);
    xt[2][x] = static_cast<unsigned char>(xb);

    xr += drx;
    xg += dgx;
    xb += dbx;
  }

  // Create Y table
  dry /= h;
  dgy /= h;
  dby /= h;

  for (y = 0; y < height; ++y) {
    yt[0][y] = static_cast<unsigned char>(yr);
    yt[1][y] = static_cast<unsigned char>(yg);
    yt[2][y] = static_cast<unsigned char>(yb);

    yr += dry;
    yg += dgy;
    yb += dby;
  }

  // Combine tables to create gradient

  if (! interlaced) {
    // normal dgradient
    for (y = 0; y < height; ++y) {
      for (x = 0; x < width; ++x, ++p) {
        p->red   = xt[0][x] + yt[0][y];
        p->green = xt[1][x] + yt[1][y];
        p->blue  = xt[2][x] + yt[2][y];
      }
    }
    return;
  }

  // interlacing effect
  for (y = 0; y < height; ++y) {
    for (x = 0; x < width; ++x, ++p) {
      p->red   = xt[0][x] + yt[0][y];
      p->green = xt[1][x] + yt[1][y];
      p->blue  = xt[2][x] + yt[2][y];

      if (y & 1) {
        p->red   = (p->red   >> 1) + (p->red   >> 2);
        p->green = (p->green >> 1) + (p->green >> 2);
        p->blue  = (p->blue  >> 1) + (p->blue  >> 2);
      }
    }
  }

  delete [] alloc;
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
      memcpy(p, data, width * sizeof(RGB));
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
  unsigned int x, y;

  const unsigned int dimension = std::max(width, height);
  unsigned int *alloc = new unsigned int[dimension * 6];
  unsigned int *xt[3], *yt[3];
  xt[0] = alloc + (dimension * 0);
  xt[1] = alloc + (dimension * 1);
  xt[2] = alloc + (dimension * 2);
  yt[0] = alloc + (dimension * 3);
  yt[1] = alloc + (dimension * 4);
  yt[2] = alloc + (dimension * 5);

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
    xt[0][x] = static_cast<unsigned char>(fabs(xr));
    xt[1][x] = static_cast<unsigned char>(fabs(xg));
    xt[2][x] = static_cast<unsigned char>(fabs(xb));

    xr -= drx;
    xg -= dgx;
    xb -= dbx;
  }

  // Create Y table
  dry /= height;
  dgy /= height;
  dby /= height;

  for (y = 0; y < height; ++y) {
    yt[0][y] = static_cast<unsigned char>(fabs(yr));
    yt[1][y] = static_cast<unsigned char>(fabs(yg));
    yt[2][y] = static_cast<unsigned char>(fabs(yb));

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
          static_cast<unsigned char>(tr - (rsign * (xt[0][x] + yt[0][y])));
        p->green =
          static_cast<unsigned char>(tg - (gsign * (xt[1][x] + yt[1][y])));
        p->blue =
          static_cast<unsigned char>(tb - (bsign * (xt[2][x] + yt[2][y])));
      }
    }
    return;
  }

  // interlacing effect
  for (y = 0; y < height; ++y) {
    for (x = 0; x < width; ++x, ++p) {
      p->red =
        static_cast<unsigned char>(tr - (rsign * (xt[0][x] + yt[0][y])));
      p->green =
        static_cast<unsigned char>(tg - (gsign * (xt[1][x] + yt[1][y])));
      p->blue =
        static_cast<unsigned char>(tb - (bsign * (xt[2][x] + yt[2][y])));

      if (y & 1) {
        p->red   = (p->red   >> 1) + (p->red   >> 2);
        p->green = (p->green >> 1) + (p->green >> 2);
        p->blue  = (p->blue  >> 1) + (p->blue  >> 2);
      }
    }
  }

  delete [] alloc;
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
  unsigned int x, y;

  const unsigned int dimension = std::max(width, height);
  unsigned int *alloc = new unsigned int[dimension * 6];
  unsigned int *xt[3], *yt[3];
  xt[0] = alloc + (dimension * 0);
  xt[1] = alloc + (dimension * 1);
  xt[2] = alloc + (dimension * 2);
  yt[0] = alloc + (dimension * 3);
  yt[1] = alloc + (dimension * 4);
  yt[2] = alloc + (dimension * 5);

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
    xt[0][x] = static_cast<unsigned char>(fabs(xr));
    xt[1][x] = static_cast<unsigned char>(fabs(xg));
    xt[2][x] = static_cast<unsigned char>(fabs(xb));

    xr -= drx;
    xg -= dgx;
    xb -= dbx;
  }

  // Create Y table
  dry /= height;
  dgy /= height;
  dby /= height;

  for (y = 0; y < height; ++y) {
    yt[0][y] = static_cast<unsigned char>(fabs(yr));
    yt[1][y] = static_cast<unsigned char>(fabs(yg));
    yt[2][y] = static_cast<unsigned char>(fabs(yb));

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
                                           std::max(xt[0][x], yt[0][y])));
        p->green =
          static_cast<unsigned char>(tg - (gsign *
                                           std::max(xt[1][x], yt[1][y])));
        p->blue =
          static_cast<unsigned char>(tb - (bsign *
                                           std::max(xt[2][x], yt[2][y])));
      }
    }
    return;
  }

  // interlacing effect
  for (y = 0; y < height; ++y) {
    for (x = 0; x < width; ++x, ++p) {
      p->red =
        static_cast<unsigned char>(tr - (rsign *
                                         std::max(xt[0][x], yt[0][y])));
      p->green =
        static_cast<unsigned char>(tg - (gsign *
                                         std::max(xt[1][x], yt[1][y])));
      p->blue =
        static_cast<unsigned char>(tb - (bsign *
                                         std::max(xt[2][x], yt[2][y])));

      if (y & 1) {
        p->red   = (p->red   >> 1) + (p->red   >> 2);
        p->green = (p->green >> 1) + (p->green >> 2);
        p->blue  = (p->blue  >> 1) + (p->blue  >> 2);
      }
    }
  }

  delete [] alloc;
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
  unsigned int x, y;

  const unsigned int dimension = std::max(width, height);
  unsigned int *alloc = new unsigned int[dimension * 6];
  unsigned int *xt[3], *yt[3];
  xt[0] = alloc + (dimension * 0);
  xt[1] = alloc + (dimension * 1);
  xt[2] = alloc + (dimension * 2);
  yt[0] = alloc + (dimension * 3);
  yt[1] = alloc + (dimension * 4);
  yt[2] = alloc + (dimension * 5);

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
    xt[0][x] = static_cast<unsigned int>(xr * xr);
    xt[1][x] = static_cast<unsigned int>(xg * xg);
    xt[2][x] = static_cast<unsigned int>(xb * xb);

    xr -= drx;
    xg -= dgx;
    xb -= dbx;
  }

  // Create Y table
  dry /= height;
  dgy /= height;
  dby /= height;

  for (y = 0; y < height; y++) {
    yt[0][y] = static_cast<unsigned int>(yr * yr);
    yt[1][y] = static_cast<unsigned int>(yg * yg);
    yt[2][y] = static_cast<unsigned int>(yb * yb);

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
                   (tr - (rsign * static_cast<int>(sqrt(xt[0][x] +
                                                        yt[0][y]))));
        p->green = static_cast<unsigned char>
                   (tg - (gsign * static_cast<int>(sqrt(xt[1][x] +
                                                        yt[1][y]))));
        p->blue  = static_cast<unsigned char>
                   (tb - (bsign * static_cast<int>(sqrt(xt[2][x] +
                                                        yt[2][y]))));
      }
    }
    return;
  }

  // interlacing effect
  for (y = 0; y < height; ++y) {
    for (x = 0; x < width; ++x, ++p) {
      p->red   = static_cast<unsigned char>
                 (tr - (rsign * static_cast<int>(sqrt(xt[0][x] + yt[0][y]))));
      p->green = static_cast<unsigned char>
                 (tg - (gsign * static_cast<int>(sqrt(xt[1][x] + yt[1][y]))));
      p->blue  = static_cast<unsigned char>
                 (tb - (bsign * static_cast<int>(sqrt(xt[2][x] + yt[2][y]))));

      if (y & 1) {
        p->red   = (p->red   >> 1) + (p->red   >> 2);
        p->green = (p->green >> 1) + (p->green >> 2);
        p->blue  = (p->blue  >> 1) + (p->blue  >> 2);
      }
    }
  }

  delete [] alloc;
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
  unsigned int x, y;

  const unsigned int dimension = std::max(width, height);
  unsigned int *alloc = new unsigned int[dimension * 6];
  unsigned int *xt[3], *yt[3];
  xt[0] = alloc + (dimension * 0);
  xt[1] = alloc + (dimension * 1);
  xt[2] = alloc + (dimension * 2);
  yt[0] = alloc + (dimension * 3);
  yt[1] = alloc + (dimension * 4);
  yt[2] = alloc + (dimension * 5);

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
    xt[0][x] = static_cast<unsigned char>(fabs(xr));
    xt[1][x] = static_cast<unsigned char>(fabs(xg));
    xt[2][x] = static_cast<unsigned char>(fabs(xb));

    xr -= drx;
    xg -= dgx;
    xb -= dbx;
  }

  // Create Y table
  dry /= height;
  dgy /= height;
  dby /= height;

  for (y = 0; y < height; ++y) {
    yt[0][y] = static_cast<unsigned char>(fabs(yr));
    yt[1][y] = static_cast<unsigned char>(fabs(yg));
    yt[2][y] = static_cast<unsigned char>(fabs(yb));

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
                                           std::min(xt[0][x], yt[0][y])));
        p->green =
          static_cast<unsigned char>(tg - (gsign *
                                           std::min(xt[1][x], yt[1][y])));
        p->blue =
          static_cast<unsigned char>(tb - (bsign *
                                           std::min(xt[2][x], yt[2][y])));
      }
    }
    return;
  }

  // interlacing effect
  for (y = 0; y < height; ++y) {
    for (x = 0; x < width; ++x, ++p) {
      p->red =
        static_cast<unsigned char>(tr - (rsign *
                                         std::min(xt[0][x], yt[0][y])));
      p->green =
        static_cast<unsigned char>(tg - (gsign *
                                         std::min(xt[1][x], yt[1][y])));
      p->blue =
        static_cast<unsigned char>(tb - (bsign *
                                         std::min(xt[2][x], yt[2][y])));

      if (y & 1) {
        p->red   = (p->red   >> 1) + (p->red   >> 2);
        p->green = (p->green >> 1) + (p->green >> 2);
        p->blue  = (p->blue  >> 1) + (p->blue  >> 2);
      }
    }
  }

  delete [] alloc;
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
  unsigned int x, y;

  const unsigned int dimension = std::max(width, height);
  unsigned int *alloc = new unsigned int[dimension * 6];
  unsigned int *xt[3], *yt[3];
  xt[0] = alloc + (dimension * 0);
  xt[1] = alloc + (dimension * 1);
  xt[2] = alloc + (dimension * 2);
  yt[0] = alloc + (dimension * 3);
  yt[1] = alloc + (dimension * 4);
  yt[2] = alloc + (dimension * 5);

  dry = drx = static_cast<double>(to.red()   - from.red()  );
  dgy = dgx = static_cast<double>(to.green() - from.green());
  dby = dbx = static_cast<double>(to.blue()  - from.blue() );

  // Create X table
  drx /= w;
  dgx /= w;
  dbx /= w;

  for (x = width - 1; x != 0; --x) {
    xt[0][x] = static_cast<unsigned char>(xr);
    xt[1][x] = static_cast<unsigned char>(xg);
    xt[2][x] = static_cast<unsigned char>(xb);

    xr += drx;
    xg += dgx;
    xb += dbx;
  }

  xt[0][x] = static_cast<unsigned char>(xr);
  xt[1][x] = static_cast<unsigned char>(xg);
  xt[2][x] = static_cast<unsigned char>(xb);

  // Create Y table
  dry /= h;
  dgy /= h;
  dby /= h;

  for (y = 0; y < height; ++y) {
    yt[0][y] = static_cast<unsigned char>(yr);
    yt[1][y] = static_cast<unsigned char>(yg);
    yt[2][y] = static_cast<unsigned char>(yb);

    yr += dry;
    yg += dgy;
    yb += dby;
  }

  // Combine tables to create gradient

  if (! interlaced) {
    // normal dgradient
    for (y = 0; y < height; ++y) {
      for (x = 0; x < width; ++x, ++p) {
        p->red   = xt[0][x] + yt[0][y];
        p->green = xt[1][x] + yt[1][y];
        p->blue  = xt[2][x] + yt[2][y];
      }
    }
    return;
  }

  // interlacing effect
  for (y = 0; y < height; ++y) {
    for (x = 0; x < width; ++x, ++p) {
      p->red   = xt[0][x] + yt[0][y];
      p->green = xt[1][x] + yt[1][y];
      p->blue  = xt[2][x] + yt[2][y];

      if (y & 1) {
        p->red   = (p->red   >> 1) + (p->red   >> 2);
        p->green = (p->green >> 1) + (p->green >> 2);
        p->blue  = (p->blue  >> 1) + (p->blue  >> 2);
      }
    }
  }

  delete [] alloc;
}
