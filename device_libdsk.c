/* #includes */ /*{{{C}}}*//*{{{*/
#include "config.h"

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "device.h"

#ifdef USE_DMALLOC
#include <dmalloc.h>
#endif
/*}}}*/

static const char *lookup_format(DSK_GEOMETRY *geom, const char *name)
{
  dsk_format_t fmt = FMT_180K;
  const char *fname;

  while (dg_stdformat(NULL, fmt, &fname, NULL) == DSK_ERR_OK)
  {
    if (!strcmp(name, fname))
    {
      dg_stdformat(geom, fmt, &fname, NULL);
      return NULL;
    }
    ++fmt;
  }
  return "Unrecognised LibDsk geometry specification";
}

/* Device_open           -- Open an image file                      */ /*{{{*/
const char *Device_open(struct Device *this, const char *filename, int mode, const char *deviceOpts)
{
  char *format;
  char driverName[80];
  const char *boo;

  /* Assume driver name & format name both fit in 80 characters, rather than
   * malloccing the exact size */
  strncpy(driverName, deviceOpts, 79);
  driverName[79] = 0;
  format = strchr(driverName, ',');
  if (format)
  {
    *format = 0;
    ++format;
  }
  dsk_err_t e = dsk_open(&this->dev, filename, driverName, NULL);
  this->opened = 0;
  if (e) return dsk_strerror(e);
  this->opened = 1;
  if (format) 
  {
    boo = lookup_format(&this->geom, format);
    if (boo) return boo;
  }
  else 
  {
    dsk_getgeom(this->dev, &this->geom); 
  }
  return NULL;
}
/*}}}*/
/* Device_setGeometry    -- Set disk geometry                       */ /*{{{*/
void Device_setGeometry(struct Device *this, int secLength, int sectrk, int tracks, off_t offset)
{
  this->secLength=secLength;
  this->sectrk=sectrk;
  this->tracks=tracks;
  /* Must be an even multiple of sector size */
  assert(offset%secLength==0);
  this->offset=offset;
  
  this->geom.dg_secsize   = secLength;
  this->geom.dg_sectors   = sectrk;
  /* Did the autoprobe guess right about the number of sectors & cylinders? */
  if (this->geom.dg_cylinders * this->geom.dg_heads == tracks) return;
  /* Otherwise we guess: <= 43 tracks: single-sided. Else double. This
   * fails for 80-track single-sided if there are any such beasts */
  if (tracks <= 43) 
  {
    this->geom.dg_cylinders = tracks;
    this->geom.dg_heads     = 1; 
  }
  else
  {
    this->geom.dg_cylinders = tracks/2;
    this->geom.dg_heads     = 2; 
  }
}
/*}}}*/
/* Device_close          -- Close an image file                     */ /*{{{*/
const char *Device_close(struct Device *this)
{
  dsk_err_t e;
  this->opened=0;
  e = dsk_close(&this->dev);
  return (e?dsk_strerror(e):(const char*)0);
}
/*}}}*/
/* Device_readSector     -- read a physical sector                  */ /*{{{*/
const char *Device_readSector(const struct Device *this, int track, int sector, char *buf)
{
  dsk_err_t e;
  e = dsk_lread(this->dev, &this->geom, buf, (track * this->sectrk) + sector + this->offset/this->secLength);
  return (e?dsk_strerror(e):(const char*)0);
}
/*}}}*/
/* Device_writeSector    -- write physical sector                   */ /*{{{*/
const char *Device_writeSector(const struct Device *this, int track, int sector, const char *buf)
{
  dsk_err_t e;
  e = dsk_lwrite(this->dev, &this->geom, buf, (track * this->sectrk) + sector + this->offset/this->secLength);
  return (e?dsk_strerror(e):(const char*)0);
}
/*}}}*/
