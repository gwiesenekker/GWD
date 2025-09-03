//SCU REVISION 7.902 di 26 aug 2025  4:15:00 CEST
#include "globals.h"

#include <zstd.h>

#define BLOCK_LENGTH 1024

#define COMPRESSED_VERSION 1
#define ZSTD_LEVEL         9

#if ((BLOCK_LENGTH % 8) != 0)
#error "BLOCK_LENGTH must be a multiple of 8"
#endif

typedef struct
{
  ui32_t CB_crc32_data;
  i64_t CB_offset;
  i16_t CB_ldata_compressed;
  i8_t *CB_data_compressed;
} compressed_block_t;

typedef struct
{
  i64_t CF_nblocks;
  compressed_block_t *CF_blocks;
  i8_t *CF_data_compressed;
  i64_t CF_ldata_compressed;
} compressed_file_t;

local ui32_t return_crc32_data(ui64_t *a)
{
  ui32_t crc32 = 0xFFFFFFFF;

  for (int i = 0; i < (BLOCK_LENGTH / 8); i++)
    crc32 = HW_CRC32_U64(crc32, a[i]);

  crc32 = ~crc32;

  return(crc32);
}

local void construct_compressed_file(compressed_file_t *self, i64_t arg_nblocks)
{
  HARDBUG(arg_nblocks < 1)

  compressed_file_t *object = self;

  object->CF_nblocks = arg_nblocks;

  MY_MALLOC_BY_TYPE(object->CF_blocks, compressed_block_t, object->CF_nblocks)

  for (i64_t iblock = 0; iblock < object->CF_nblocks; iblock++)
  {
    compressed_block_t *with = object->CF_blocks + iblock;

    with->CB_ldata_compressed = 0;
    with->CB_data_compressed = NULL;
    with->CB_crc32_data = 0;
  }

  object->CF_data_compressed = NULL;
}

local void add_block(compressed_file_t *self, i64_t iblock,
  i8_t *arg_data)
{
  compressed_file_t *object = self;

  HARDBUG(iblock < 0)
  HARDBUG(iblock >= object->CF_nblocks)

  compressed_block_t *with = object->CF_blocks + iblock;

  HARDBUG(with->CB_ldata_compressed != 0)
  HARDBUG(with->CB_data_compressed != NULL)

  with->CB_crc32_data = return_crc32_data((ui64_t *) arg_data);

  i8_t data_compressed[2 * BLOCK_LENGTH];

  HARDBUG(ZSTD_compressBound(BLOCK_LENGTH) > sizeof(data_compressed))

  size_t zstd_size = ZSTD_compress(data_compressed, sizeof(data_compressed),
                                   arg_data, BLOCK_LENGTH, ZSTD_LEVEL);

  HARDBUG(ZSTD_isError(zstd_size))

  int ldata_compressed = zstd_size;

  MY_MALLOC_BY_TYPE(with->CB_data_compressed, i8_t, ldata_compressed);

  with->CB_ldata_compressed = ldata_compressed;

  memcpy(with->CB_data_compressed, data_compressed, ldata_compressed);
}

local void save_compressed_file(compressed_file_t *self, bstring bname)
{
  compressed_file_t *object = self;

  HARDBUG(object->CF_nblocks < 1)

  FILE *fcompressed;

  HARDBUG((fcompressed = fopen(bdata(bname), "wb")) == NULL)

  //offsets

  object->CF_blocks[0].CB_offset = 0;

  for (i64_t iblock = 1; iblock < object->CF_nblocks; iblock++)
  {
    object->CF_blocks[iblock].CB_offset =
      object->CF_blocks[iblock - 1].CB_offset + 
      object->CF_blocks[iblock - 1].CB_ldata_compressed;
  }

  i64_t ldata_compressed =
    object->CF_blocks[object->CF_nblocks - 1].CB_offset + 
    object->CF_blocks[object->CF_nblocks - 1].CB_ldata_compressed;

  PRINTF("ldata_compressed=%lld\n", ldata_compressed);

  //header

  i8_t version = COMPRESSED_VERSION;
  
  FWRITE(fcompressed, &version, i8_t, 1)

  i8_t level = ZSTD_LEVEL;

  FWRITE(fcompressed, &level, i8_t, 1)

  FWRITE(fcompressed, &(object->CF_nblocks), i64_t, 1)

  FWRITE(fcompressed, &ldata_compressed, i64_t, 1)

  //data

  FWRITE(fcompressed, object->CF_blocks, compressed_block_t,
         object->CF_nblocks)

  for (i64_t iblock = 0; iblock < object->CF_nblocks; iblock++)
    FWRITE(fcompressed, object->CF_blocks[iblock].CB_data_compressed, i8_t,
                        object->CF_blocks[iblock].CB_ldata_compressed);

  FCLOSE(fcompressed)
}

local void load_compressed_file(compressed_file_t *self, bstring bname)
{
  compressed_file_t *object = self;

  FILE *fcompressed;

  HARDBUG((fcompressed = fopen(bdata(bname), "rb")) == NULL)

  //header

  i8_t version;

  FREAD(fcompressed, &version, i8_t, 1)

  HARDBUG(version != COMPRESSED_VERSION)

  i8_t level;

  FREAD(fcompressed, &level, i8_t, 1)

  HARDBUG(level != ZSTD_LEVEL)

  FREAD(fcompressed, &(object->CF_nblocks), i64_t, 1)

  HARDBUG(object->CF_nblocks < 1)

  i64_t ldata_compressed;

  FREAD(fcompressed, &ldata_compressed, i64_t, 1)

  HARDBUG(ldata_compressed < 1)

  MY_MALLOC_BY_TYPE(object->CF_blocks, compressed_block_t, object->CF_nblocks)

  FREAD(fcompressed, object->CF_blocks, compressed_block_t,
         object->CF_nblocks)

  MY_MALLOC_BY_TYPE(object->CF_data_compressed, i8_t, ldata_compressed)
   
  FREAD(fcompressed, object->CF_data_compressed, i8_t, ldata_compressed)

  FCLOSE(fcompressed)

  PRINTF("verifying..\n");

  for (i64_t iblock = 0; iblock < object->CF_nblocks; iblock++)
  {
    i8_t data_uncompressed[BLOCK_LENGTH];

    size_t zstd_size =
      ZSTD_decompress(data_uncompressed, BLOCK_LENGTH,
                      object->CF_data_compressed +
                      object->CF_blocks[iblock].CB_offset,
                      object->CF_blocks[iblock].CB_ldata_compressed);
  
    HARDBUG(ZSTD_isError(zstd_size))
  
    HARDBUG(zstd_size != BLOCK_LENGTH)

    HARDBUG(return_crc32_data((ui64_t *) data_uncompressed) !=
            object->CF_blocks[iblock].CB_crc32_data)
  }

  PRINTF("..OK\n");
}

local i8_t read_compressed_file(compressed_file_t *self, i64_t where)
{
  compressed_file_t *object = self;

  i64_t iblock = where / BLOCK_LENGTH;

  //uncompress iblock

  i8_t data_uncompressed[BLOCK_LENGTH];

  size_t zstd_size =
    ZSTD_decompress(data_uncompressed, BLOCK_LENGTH,
                    object->CF_data_compressed +
                    object->CF_blocks[iblock].CB_offset,
                    object->CF_blocks[iblock].CB_ldata_compressed);

  HARDBUG(ZSTD_isError(zstd_size))

  HARDBUG(zstd_size != BLOCK_LENGTH)

  HARDBUG(return_crc32_data((ui64_t *) data_uncompressed) !=
          object->CF_blocks[iblock].CB_crc32_data)

  return(data_uncompressed[where % BLOCK_LENGTH]);
}

local void compress_file(bstring bwdl)
{
  //get file stats

  i64_t size;

  HARDBUG((size = compat_size(bdata(bwdl))) == -1)

  PRINTF("size=%lld\n", size);

  FILE *fwdl;

  HARDBUG((fwdl = fopen(bdata(bwdl), "rb")) == NULL)
 
  i8_t wdl_version;
    
  HARDBUG(fread(&wdl_version, sizeof(i8_t), 1, fwdl) != 1)

  PRINTF("wdl_version=%d\n", wdl_version);

  size--;

  PRINTF("adjusted size=%lld\n", size);

  PRINTF("size %% 128=%lld\n", size % 128);
        
  i64_t nblocks = size / BLOCK_LENGTH;

  if (nblocks * BLOCK_LENGTH < size) ++nblocks;

  HARDBUG(nblocks * BLOCK_LENGTH < size)

  PRINTF("nblocks=%lld\n", nblocks);

  compressed_file_t cf;

  construct_compressed_file(&cf, nblocks);

  i8_t data[BLOCK_LENGTH];
 
  int ldata;

  i64_t iblock = 0;

  while((ldata = fread(data, sizeof(i8_t), BLOCK_LENGTH, fwdl)) > 0)
  {
    HARDBUG(iblock >= nblocks)
     
    if (ldata < BLOCK_LENGTH)
    {  
      HARDBUG(iblock != (nblocks -1 ))

      for (int idata = ldata; idata < BLOCK_LENGTH; idata++)   
        data[idata] = 0;
    }

    add_block(&cf, iblock, data);

    iblock++;
  }

  FCLOSE(fwdl)

  HARDBUG(iblock != nblocks)

  BSTRING(bname_zstd)

  bformata(bname_zstd, "%s.zstd", bdata(bwdl));

  PRINTF("bname_zstd=%s\n", bdata(bname_zstd));

  save_compressed_file(&cf, bname_zstd);
 
  BDESTROY(bname_zstd)
}

void test_compress(void)
{
  BSTRING(bname)

  HARDBUG(bassigncstr(bname, "test.wdl") == BSTR_ERR)

  compress_file(bname);

  compressed_file_t cf;

  HARDBUG(bassigncstr(bname, "test.wdl.zstd") == BSTR_ERR)

  load_compressed_file(&cf, bname);

  BDESTROY(bname)
}

