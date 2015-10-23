#include <ossimPlanet/ossimPlanetJpegImage.h>
#include <ossimPlanet/ossimPlanetImage.h>
#include <iostream>
#include <fstream>

extern "C" {
#include <jpeglib.h>
#include <setjmp.h>
#define JPEG_IO_BUFFER_SIZE   2048
#define OUTPUT_BUF_SIZE  4096    /* choose an efficiently fwrite'able size */
  
typedef struct {
    struct jpeg_source_mgr pub;   /* public fields */

    JOCTET* buffer;               /* start of buffer */
   std::istream *stream;
} ossimPlanet_source_mgr;
typedef struct {
    struct jpeg_destination_mgr pub;

   std::ostream *stream;
    JOCTET * buffer;
} ossimPlanet_destination_mgr;

typedef ossimPlanet_destination_mgr * ossimPlanet_dest_ptr;


typedef ossimPlanet_source_mgr * ossimPlanet_src_ptr;

   
static void ossimPlanet_init_source ( j_decompress_ptr )
{
}
#define OUTPUT_BUF_SIZE  4096    /* choose an efficiently fwrite'able size */

static void init_destination (j_compress_ptr cinfo)
{
    ossimPlanet_dest_ptr dest = (ossimPlanet_dest_ptr) cinfo->dest;

    /* Allocate the output buffer --- it will be released when done with image */
    dest->buffer = (JOCTET *)
        (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
        OUTPUT_BUF_SIZE * sizeof(JOCTET));
    dest->pub.next_output_byte = dest->buffer;
    dest->pub.free_in_buffer = OUTPUT_BUF_SIZE;
}

static boolean empty_output_buffer (j_compress_ptr cinfo)
{
    ossimPlanet_dest_ptr dest = (ossimPlanet_dest_ptr) cinfo->dest;

    dest->stream->write((char*)dest->buffer, OUTPUT_BUF_SIZE);
    dest->pub.next_output_byte = dest->buffer;
    dest->pub.free_in_buffer = OUTPUT_BUF_SIZE;
    return TRUE;
}
	
static void term_destination (j_compress_ptr cinfo)
{
    ossimPlanet_dest_ptr dest = (ossimPlanet_dest_ptr) cinfo->dest;
    size_t datacount = OUTPUT_BUF_SIZE - dest->pub.free_in_buffer;
    /* Write any data remaining in the buffer */
    if (datacount > 0)
       dest->stream->write((char*)dest->buffer, datacount);
}

static boolean ossimPlanet_fill_input_buffer ( j_decompress_ptr cinfo )
{
    ossimPlanet_src_ptr src = (ossimPlanet_src_ptr) cinfo->src;

    src->pub.next_input_byte = src->buffer;
    std::streamsize start = src->stream->tellg();
    src->stream->read((char*)(src->buffer),
                      JPEG_IO_BUFFER_SIZE).tellg();
    std::streamsize end = src->stream->tellg();
    
    src->pub.bytes_in_buffer = end - start;

    if (src->pub.bytes_in_buffer == 0) // check for end-of-stream
    {
        // Insert a fake EOI marker
        src->buffer[0] = 0xFF;
        src->buffer[1] = JPEG_EOI;
        src->pub.bytes_in_buffer = 2;
    }
    return TRUE;
}

static void ossimPlanet_skip_input_data ( j_decompress_ptr cinfo, long num_bytes )
{
    if (num_bytes > 0)
    {
        ossimPlanet_src_ptr src = (ossimPlanet_src_ptr) cinfo->src;

        while (num_bytes > (long)src->pub.bytes_in_buffer)
        {
            num_bytes -= (long) src->pub.bytes_in_buffer;
            src->pub.fill_input_buffer(cinfo);
        }
        src->pub.next_input_byte += (size_t) num_bytes;
        src->pub.bytes_in_buffer -= (size_t) num_bytes;
    }
}

static void ossimPlanet_term_source ( j_decompress_ptr cinfo )
{
    ossimPlanet_src_ptr src = (ossimPlanet_src_ptr) cinfo->src;

    if (src->pub.bytes_in_buffer > 0)
       src->stream->seekg(-(long)src->pub.bytes_in_buffer, std::ios_base::cur);
    delete[] src->buffer;
}


// JPEG error manager:

struct ossimPlanet_error_mgr {
  struct jpeg_error_mgr pub;    /* "public" fields */

  jmp_buf setjmp_buffer;    /* for return to caller */
};

typedef struct ossimPlanet_error_mgr * ossimPlanet_error_ptr;

/*
 * Here's the routine that will replace the standard error_exit method:
 */

static void ossimPlanet_error_exit (j_common_ptr cinfo)
{
  /* cinfo->err really points to a ossimPlanet_error_mgr struct, so coerce pointer */
  ossimPlanet_error_ptr myerr = (ossimPlanet_error_ptr) cinfo->err;

  /* Always display the message. */
  /* We could postpone this until after returning, if we chose. */
  if (cinfo->err->output_message) (*cinfo->err->output_message) (cinfo);

  /* Return control to the setjmp point */
  longjmp(myerr->setjmp_buffer, 1);
}

   
void ossimPlanet_jpeg_io_src( j_decompress_ptr cinfo, std::istream& infile )
{
    ossimPlanet_src_ptr src;

    if (cinfo->src == NULL) {    /* first time for this JPEG object? */
        cinfo->src = (struct jpeg_source_mgr *)
            (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT,
            sizeof(ossimPlanet_source_mgr));
    }
    src = (ossimPlanet_src_ptr) cinfo->src;
    src->pub.bytes_in_buffer = 0; /* forces fill_input_buffer on first read */
    src->buffer = new JOCTET[JPEG_IO_BUFFER_SIZE];
    src->pub.next_input_byte = NULL; /* until buffer loaded */
    src->stream = &infile;

    src->pub.init_source = ossimPlanet_init_source;
    src->pub.fill_input_buffer = ossimPlanet_fill_input_buffer;
    src->pub.skip_input_data = ossimPlanet_skip_input_data;
    src->pub.resync_to_restart = jpeg_resync_to_restart; /* use default method */
    src->pub.term_source = ossimPlanet_term_source;
}

static void ossimPlanet_init_destination (j_compress_ptr cinfo)
{
    ossimPlanet_dest_ptr dest = (ossimPlanet_dest_ptr) cinfo->dest;

    /* Allocate the output buffer --- it will be released when done with image */
    dest->buffer = (JOCTET *)
        (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
        OUTPUT_BUF_SIZE * sizeof(JOCTET));
    dest->pub.next_output_byte = dest->buffer;
    dest->pub.free_in_buffer = OUTPUT_BUF_SIZE;
}

static boolean ossimPlanet_empty_output_buffer (j_compress_ptr cinfo)
{
    ossimPlanet_dest_ptr dest = (ossimPlanet_dest_ptr) cinfo->dest;

    dest->stream->write((char*)(dest->buffer), OUTPUT_BUF_SIZE);
    dest->pub.next_output_byte = dest->buffer;
    dest->pub.free_in_buffer = OUTPUT_BUF_SIZE;
    return TRUE;
}

static void ossimPlanet_term_destination (j_compress_ptr cinfo)
{
    ossimPlanet_dest_ptr dest = (ossimPlanet_dest_ptr) cinfo->dest;
    size_t datacount = OUTPUT_BUF_SIZE - dest->pub.free_in_buffer;
    /* Write any data remaining in the buffer */
    if (datacount > 0)
       dest->stream->write((char*)(dest->buffer), datacount);
}

static void ossimPlanet_jpeg_io_dest (j_compress_ptr cinfo, std::ostream& outfile)
{
    ossimPlanet_dest_ptr dest;

    if (cinfo->dest == NULL) {    /* first time for this JPEG object? */
        cinfo->dest = (struct jpeg_destination_mgr *)
            (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT,
            sizeof(ossimPlanet_destination_mgr));
    }

    dest = (ossimPlanet_dest_ptr) cinfo->dest;
    dest->pub.init_destination = ossimPlanet_init_destination;
    dest->pub.empty_output_buffer = ossimPlanet_empty_output_buffer;
    dest->pub.term_destination = ossimPlanet_term_destination;
    dest->stream = &outfile;
}



ossimPlanetJpegImage::ossimPlanetJpegImage()
{
}

ossimPlanetJpegImage::~ossimPlanetJpegImage()
{
}

bool ossimPlanetJpegImage::loadFile(std::istream& inputStream,
                                  ossimPlanetImage& image)
{
    struct jpeg_decompress_struct cinfo;
    struct ossimPlanet_error_mgr jerr;
    JSAMPARRAY tempbuf;
    unsigned char *ptr;
    unsigned stride;

    cinfo.err = jpeg_std_error( &jerr.pub );
    jerr.pub.error_exit = ossimPlanet_error_exit;

    cinfo.err->output_message=NULL;

    /* Establish the setjmp return context for ossimPlanet_error_exit to use. */
    if (setjmp(jerr.setjmp_buffer)) {
      /* If we get here, the JPEG code has signaled an error.
       * We need to clean up the JPEG object, close the input file, and return.
       */
      (cinfo.src->term_source)(&cinfo);
      jpeg_destroy_decompress(&cinfo);
      return FALSE;
    }

    jpeg_create_decompress( &cinfo );
    ossimPlanet_jpeg_io_src( &cinfo, inputStream );
    jpeg_read_header( &cinfo, TRUE );
    cinfo.out_color_space = JCS_RGB;
    jpeg_start_decompress( &cinfo );

    image.allocateImage(cinfo.image_width,
                        cinfo.image_height,
                        1,
                        GL_RGB,               
                        GL_UNSIGNED_BYTE);
//    image.create(cinfo.image_width, cinfo.image_height);
    ptr = image.data();
    stride = cinfo.output_width * 3;
    tempbuf = (*cinfo.mem->alloc_sarray)
        ((j_common_ptr) &cinfo, JPOOL_IMAGE, stride, 1 );

    while ( cinfo.output_scanline < cinfo.output_height )
    {
        jpeg_read_scanlines( &cinfo, tempbuf, 1 );
        memcpy( ptr, tempbuf[0], stride );
        ptr += stride;
    }
    jpeg_finish_decompress( &cinfo );
    jpeg_destroy_decompress( &cinfo );
    
    return TRUE;
}

bool ossimPlanetJpegImage::saveFile( std::ostream& stream,
                                   ossimPlanetImage &image,
                                   bool verbose )
{
    struct jpeg_compress_struct cinfo;
    struct ossimPlanet_error_mgr jerr;
    JSAMPROW row_pointer[1];    /* pointer to JSAMPLE row[s] */
    JSAMPLE *image_buffer;
    int stride;                /* physical row width in image buffer */

    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = ossimPlanet_error_exit;

    if (!verbose) cinfo.err->output_message=NULL;

    /* Establish the setjmp return context for my_error_exit to use. */
    if (setjmp(jerr.setjmp_buffer))
    {
        /* If we get here, the JPEG code has signaled an error.
         * We need to clean up the JPEG object, close the input file, and return.
         */
         jpeg_destroy_compress(&cinfo);
         return false;
    }

    jpeg_create_compress(&cinfo);
    ossimPlanet_jpeg_io_dest(&cinfo, stream);

    cinfo.image_width      = image.getWidth();
    cinfo.image_height     = image.getHeight();
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;
    jpeg_set_defaults(&cinfo);

    // TODO: 3rd parameter is force_baseline, what value should this be?
    // Code says: "If force_baseline is TRUE, the computed quantization table entries
    // are limited to 1..255 for JPEG baseline compatibility."
    // 'Quality' is a number between 0 (terrible) and 100 (very good).
    // The default (in jcparam.c, jpeg_set_defaults) is 75,
    // and force_baseline is TRUE.
    jpeg_set_quality(&cinfo, 100, TRUE);

    jpeg_start_compress(&cinfo, TRUE);

    stride = cinfo.image_width * 3;    /* JSAMPLEs per row in image_buffer */
    image_buffer = image.data();
    ossim_uint32 scanlineTemp = cinfo.next_scanline;
    while (cinfo.next_scanline < cinfo.image_height)
    {
        row_pointer[0] = &image_buffer[cinfo.next_scanline * stride];
        jpeg_write_scanlines( &cinfo, row_pointer, 1 );
        if(scanlineTemp == cinfo.next_scanline)
        {
           ++cinfo.next_scanline;
        }
        scanlineTemp = cinfo.next_scanline;
    }
    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);

    return true;
}

bool ossimPlanetJpegImage::loadFile(std::string& inputFile,
                                  ossimPlanetImage& image)
{
   std::ifstream in(inputFile.c_str(),
                    std::ios::in|std::ios::binary);
   if(in)
   {
      return loadFile(in, image);
   }
   return false;
}
   
bool ossimPlanetJpegImage::saveFile(std::string& outputFile,
                                  ossimPlanetImage& image)
{
   std::ofstream out(outputFile.c_str(),
                     std::ios::out|std::ios::binary);
   if(out)
   {
      return saveFile(out, image);
   }
   return false;
}
  
}
