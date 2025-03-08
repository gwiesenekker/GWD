//SCU REVISION 7.809 za  8 mrt 2025  5:23:19 CET

#include "globals.h"


//enables conversion of EGTBs from zlib to zstd compression
#undef ENABLE_RECOMPRESSION

#ifdef ENABLE_RECOMPRESSION
#include <zlib.h>
#endif

#include <zstd.h>

#define EGTB_WDL_VERSION        3
#define EGTB_COMPRESSED_VERSION 2

#define COMPRESS_OK Z_OK

#define COMPRESS_LEN_T uLongf
#define COMPRESS_BYTE_T Bytef

#define COMPRESS(D, DLP, S, SL) compress(D, DLP, S, SL)
#define UNCOMPRESS(D, DLP, S, SL) uncompress(D, DLP, S, SL)

#define ALL_I8   0
#define ALL_DRAW 1
#define ALL_I16  2

#define WDL_WON  1
#define WDL_LOST 2
#define WDL_DRAW 3

#define ZSTD

#define ENDGAME_UNAVAILABLE           0
#define ENDGAME_ALPHA_BETA_WDL        1
#define ENDGAME_ALPHA_BETA_COMPRESSED 2
#define ENDGAME_ROOT_ONLY             3
#define ENDGAME_MIRROR                4

#define SUM2(L0)    (((99-L0)*L0)/2)
#define SUM2M(L0,M) (((2*M+1-L0)*L0)/2)

#define SUM3(L0)      (((7202-147*L0+L0*L0)*L0)/6)
#define SUM32(L0, L1) (((1+L0-L1)*(L0+L1-98))/2)

#define SUM4(L0)      (((97-L0)*L0*(4702-97*L0+L0*L0))/24)
#define SUM43(L0, L1) (((L1-L0-1)*(7056-145*L0+L0*L0-146*L1+L0*L1+L1*L1))/6)

#define SUM5(L0)\
  (((26507524-1105200*L0+23035*L0*L0-240*L0*L0*L0+L0*L0*L0*L0)*L0)/120)
#define SUM54(L0, L1)\
  (((1+L0-L1)*(L0+L1-96)*(4606-95*L0+L0*L0-97*L1+L1*L1))/24)

#define MINMAX(I1, I2) {if (I1 > I2) {int t = I1; I1 = I2; I2 = t;}}

typedef struct
{
  int endgame_status;

  char endgame_name[MY_LINE_MAX];

  void *endgame_ref;

  int endgame_unavailable_warning;

  my_mutex_t endgame_fmutex;

  FILE *fendgame;
  FILE *fendgame_wdl;
#ifdef USE_OPENMPI
  MPI_Win wendgame;
#endif
  void *pendgame;

  i64_t endgame_nreads;
  i64_t endgame_nentry_hits;
  i64_t endgame_npage_hits;
} endgame_t;

typedef ui8_t wdl_t;

local endgame_t endgames[NENDGAME_MAX][NENDGAME_MAX][NENDGAME_MAX][NENDGAME_MAX];

local int egtb_level_warning = TRUE;

//TODO: add endgame_hits

local void convert2wdl(void)
{
  PRINTF("convert2wdl..\n");

  HARDBUG(sizeof(entry_i8_t) != 2)
  HARDBUG(sizeof(entry_i16_t) != 4)

  HARDBUG((ARRAY_PAGE_SIZE % 2) != 0)

  entry_i16_t page_i16[NPAGE_ENTRIES];
  entry_i8_t page_i8[NPAGE_ENTRIES];

  HARDBUG(sizeof(page_i16) != ARRAY_PAGE_SIZE)
  HARDBUG(sizeof(page_i8) != (ARRAY_PAGE_SIZE / 2))

  char path[MY_LINE_MAX];

  if (compat_strcasecmp(options.egtb_dir, "NULL") != 0)
  {
    snprintf(path, MY_LINE_MAX, "%s/1wX-0wO-1bX-0bO", options.egtb_dir);

    if (!fexists(path))
    {
      PRINTF("path=%s\n", path);

      FATAL("path does not exist!", EXIT_FAILURE)
    }
  }

  for (int nwk = 0; nwk < NENDGAME_MAX; nwk++)
  {
    for (int nwm = 0; nwm < NENDGAME_MAX; nwm++)
    {
      if ((nwk + nwm) == 0) continue;

      for (int nbk = 0; nbk < NENDGAME_MAX; nbk++)
      {
        for (int nbm = 0; nbm < NENDGAME_MAX; nbm++)
        {
          if ((nbk + nbm) == 0) continue;

          if ((nwk + nwm + nbk + nbm) >= NENDGAME_MAX) continue;

          char endgame_name[MY_LINE_MAX];

          snprintf(endgame_name, MY_LINE_MAX,
                   "%dwX-%dwO-%dbX-%dbO", nwk, nwm, nbk, nbm);
          
          snprintf(path, MY_LINE_MAX, "%s/%s", options.egtb_dir, endgame_name);
  
          if (!fexists(path))
          {
            PRINTF("EGTB %s does not exist!\n", endgame_name);

            continue;
          }

          FILE *fendgame;

          HARDBUG((fendgame = fopen(path, "r+b")) == NULL)

          i8_t version;
          i16_t nblock_entries;
          i64_t index_max;

          HARDBUG(fread(&version, sizeof(i8_t), 1, fendgame) != 1)

          HARDBUG(version != EGTB_COMPRESSED_VERSION)

          HARDBUG(fread(&nblock_entries, sizeof(i16_t), 1, fendgame) != 1)

          HARDBUG(nblock_entries != NPAGE_ENTRIES)

          HARDBUG(fread(&index_max, sizeof(i64_t), 1, fendgame) != 1)
 
          if (options.verbose > 1)
            PRINTF("version=%d nblock_entries=%d index_max=%lld\n",
              version, nblock_entries, index_max);

          i64_t npages = (index_max + 1) / nblock_entries;

          if ((npages * nblock_entries) < (index_max + 1)) npages++;

          HARDBUG((npages * nblock_entries) < (index_max + 1))
                
          //not true because of last block
          //i64_t uncompressed_size = 2 * (index_max + 1);

          i64_t nentries = npages * nblock_entries;

          i64_t uncompressed_size = sizeof(entry_i8_t)  * nentries;

          if (options.verbose > 1)
            PRINTF("npages=%lld nentries=%lld uncompressed_size=%lld\n",
              npages, nentries, uncompressed_size);

          char wdl[MY_LINE_MAX];

          snprintf(wdl, MY_LINE_MAX, "%s/%s.wdl", options.egtb_dir,
                   endgame_name);

          if (fexists(wdl))
          {
            PRINTF("WDL encoded EGTB %s already exists!\n", wdl);

            FCLOSE(fendgame)

            continue;
          }

          PRINTF("WDL encoding EGTB %s..\n", path);

          //each entry consists of wtm_mate and btm_mate
          //each encoded as 2 bits so 4 bits (1 byte) in total

          HARDBUG((nentries % 2) != 0)
  
          i64_t nwdl = nentries / 2;
              
          i64_t wdl_size = nwdl * sizeof(wdl_t);
  
          if (options.verbose > 1)
            PRINTF("nwdl=%lld wdl_size=%lld\n", nwdl, wdl_size);
  
          void *pendgame;
  
          MY_MALLOC(pendgame, wdl_t, nwdl)

          wdl_t *pwdl_t = pendgame;
            
          for (i64_t iwdl = 0; iwdl < nwdl; iwdl++) pwdl_t[iwdl] = 0;

          i64_t jentry = 0;
             
          for (i64_t ipage = 0; ipage < npages; ipage++)
          {
            i64_t seek =
              sizeof(i8_t) + sizeof(i16_t) + sizeof(i64_t) + 
              ipage * (sizeof(i64_t) + sizeof(i16_t) + sizeof(ui32_t));
                
            HARDBUG(compat_fseeko(fendgame, seek, SEEK_SET) != 0)
                    
            i64_t offset;

            i16_t length;

            ui32_t crc32;
                  
            HARDBUG(fread(&offset, sizeof(i64_t), 1, fendgame) != 1)

            HARDBUG(fread(&length, sizeof(i16_t), 1, fendgame) != 1)

            HARDBUG(fread(&crc32,  sizeof(ui32_t), 1, fendgame) != 1)
                  
            //some blocks may have offset INVALID
                  
            if (offset == INVALID)
            {
              HARDBUG(length != 0)
                
              HARDBUG(crc32 != 0xFFFFFFFF)
               
              continue;
            }
                  
            HARDBUG(length <= 0)

            HARDBUG(length > (2 * ARRAY_PAGE_SIZE))
                  
            i8_t source[2 * ARRAY_PAGE_SIZE];

            size_t sourceLen = length;
               
            HARDBUG(compat_fseeko(fendgame, offset, SEEK_SET) != 0)
                
            HARDBUG(fread(source, sizeof(i8_t), sourceLen, fendgame) !=
                    sourceLen)
                
            if (source[0] == ALL_DRAW)
            {
              crc32 = 0xFFFFFFFF;
                    
              crc32 = _mm_crc32_u8(crc32, source[0]);
                  
              HARDBUG(crc32 != crc32)
                   
              for (int ientry = 0; ientry < nblock_entries; ientry++)
              {
                ui8_t wdl_entry = (WDL_DRAW << 2) | WDL_DRAW;

                HARDBUG(jentry >= nentries)
   
                i64_t iwdl = jentry / 2;

                HARDBUG(iwdl >= nwdl)

                int jwdl = jentry % 2;

                pwdl_t[iwdl] |= wdl_entry << (jwdl * 4);

                HARDBUG(pwdl_t[iwdl] == 0)

                jentry++;
              }
            }
            else if (source[0] == ALL_I8)
            {
              entry_i8_t dest[NPAGE_ENTRIES];

              size_t destLen = sizeof(dest);
                      
              //exclude source[0]
              --sourceLen;
                    
              destLen = ZSTD_decompress(dest, destLen,
                                        source + 1, sourceLen);
                  
              HARDBUG(ZSTD_isError(destLen))
                  
              HARDBUG(destLen > sizeof(dest))
                  
              HARDBUG((destLen % sizeof(entry_i8_t)) != 0)
                  
              HARDBUG((destLen / sizeof(entry_i8_t)) != nblock_entries)
#ifdef USE_HARDWARE_CRC32
              crc32 = 0xFFFFFFFF;
                 
              crc32 = _mm_crc32_u8(crc32, source[0]);
                  
              for (int ientry = 0; ientry < nblock_entries; ientry++)
              {
                crc32 = _mm_crc32_u8(crc32, dest[ientry].entry_white_mate);
                crc32 = _mm_crc32_u8(crc32, dest[ientry].entry_black_mate);
              }
#else

#error NOT IMPLEMENTED YET

#endif
              HARDBUG(crc32 != crc32)
                  
              for (int ientry = 0; ientry < nblock_entries; ientry++)
              {
                ui8_t wdl_entry = 0;
 
                if (dest[ientry].entry_black_mate == INVALID)
                  wdl_entry = WDL_DRAW;
                else if (dest[ientry].entry_black_mate > 0)
                  wdl_entry = WDL_WON;
                else
                  wdl_entry = WDL_LOST;

                if (dest[ientry].entry_white_mate == INVALID)
                  wdl_entry = (wdl_entry << 2) | WDL_DRAW;
                else if (dest[ientry].entry_white_mate > 0)
                  wdl_entry = (wdl_entry << 2) | WDL_WON;
                else
                  wdl_entry = (wdl_entry << 2) | WDL_LOST;

                HARDBUG(jentry >= nentries)

                i64_t iwdl = jentry / 2;

                HARDBUG(iwdl >= nwdl)

                int jwdl = jentry % 2;

                pwdl_t[iwdl] |= wdl_entry << (jwdl * 4);

                HARDBUG(pwdl_t[iwdl] == 0)

                jentry++;
              }
            }
            else
            {
              entry_i16_t dest[NPAGE_ENTRIES];

              size_t destLen = sizeof(dest);
                      
              //exclude source[0]
              --sourceLen;
                   
              destLen = ZSTD_decompress(dest, destLen,
                                        source + 1, sourceLen);
                  
              HARDBUG(ZSTD_isError(destLen))
                    
              HARDBUG(destLen > sizeof(dest))
                  
              HARDBUG((destLen % sizeof(entry_i16_t)) != 0)
                  
              HARDBUG((destLen / sizeof(entry_i16_t)) != nblock_entries)
#ifdef USE_HARDWARE_CRC32
              crc32 = 0xFFFFFFFF;
                    
              crc32 = _mm_crc32_u8(crc32, source[0]);
                  
              for (int ientry = 0; ientry < nblock_entries; ientry++)
              {
                crc32 = _mm_crc32_u16(crc32, dest[ientry].entry_white_mate);
                crc32 = _mm_crc32_u16(crc32, dest[ientry].entry_black_mate);
              }
#else

#error NOT IMPLEMENTED YET

#endif
              HARDBUG(crc32 != crc32)
                  
              for (int ientry = 0; ientry < nblock_entries; ientry++)
              {
                ui8_t wdl_entry = 0;

                if (dest[ientry].entry_black_mate == INVALID)
                  wdl_entry = WDL_DRAW;
                else if (dest[ientry].entry_black_mate > 0)
                  wdl_entry = WDL_WON;
                else
                  wdl_entry = WDL_LOST;

                if (dest[ientry].entry_white_mate == INVALID)
                  wdl_entry = (wdl_entry << 2) | WDL_DRAW;
                else if (dest[ientry].entry_white_mate > 0)
                  wdl_entry = (wdl_entry << 2) | WDL_WON;
                else
                  wdl_entry = (wdl_entry << 2) | WDL_LOST;

                HARDBUG(jentry >= nentries)

                i64_t iwdl = jentry / 2;

                HARDBUG(iwdl >= nwdl)

                int jwdl = jentry % 2;

                pwdl_t[iwdl] |= wdl_entry << (jwdl * 4);

                HARDBUG(pwdl_t[iwdl] == 0)

                jentry++;
              }
            }
          }//for (i64_t ipage = 0; ipage < npages; ipage++)

          HARDBUG(jentry != nentries)

          FCLOSE(fendgame)
         
          PRINTF("writing WDL encoded EGTB %s to disk..\n", path);

          FILE *fwdl;
 
          HARDBUG((fwdl = fopen(wdl, "wb")) == NULL)

          i8_t wdl_version = EGTB_WDL_VERSION;

          HARDBUG(fwrite(&wdl_version, sizeof(i8_t), 1, fwdl) != 1)

          HARDBUG(fwrite(pendgame, sizeof(i8_t), wdl_size, fwdl) != wdl_size)
  
          FCLOSE(fwdl)

          MY_FREE_AND_NULL(pendgame)

          PRINTF("..done size is %lld\n", wdl_size);
        }
      }
    }
  }
  PRINTF("..done\n");
}

void init_endgame(void)
{
  PRINTF("init_endgame..\n");

  if (my_mpi_globals.MY_MPIG_nglobal <= 1) convert2wdl();

  my_mpi_barrier("convert2wdl", my_mpi_globals.MY_MPIG_comm_global, TRUE);

  HARDBUG(sizeof(entry_i8_t) != 2)
  HARDBUG(sizeof(entry_i16_t) != 4)

  HARDBUG((ARRAY_PAGE_SIZE % 2) != 0)

  entry_i16_t page_i16[NPAGE_ENTRIES];
  entry_i8_t page_i8[NPAGE_ENTRIES];

  HARDBUG(sizeof(page_i16) != ARRAY_PAGE_SIZE)
  HARDBUG(sizeof(page_i8) != (ARRAY_PAGE_SIZE / 2))

  char path[MY_LINE_MAX];

  if (compat_strcasecmp(options.egtb_dir, "NULL") != 0)
  {
    snprintf(path, MY_LINE_MAX, "%s/1wX-0wO-1bX-0bO", options.egtb_dir);

    if (!fexists(path))
    {
      PRINTF("path=%s\n", path);
      FATAL("path does not exist!", EXIT_FAILURE)
    }

#ifdef DEBUG
    snprintf(path, MY_LINE_MAX, "%s/0wX-5wO-0bX-2bO", options.egtb_dir);

    if (!fexists(path))
    {
      PRINTF("path=%s\n", path);
      FATAL("path does not exist!", EXIT_FAILURE)
    }
#endif
  }

  int nwk, nwm, nbk, nbm;

  for (nwk = 0; nwk < NENDGAME_MAX; nwk++)
  {
    for (nwm = 0; nwm < NENDGAME_MAX; nwm++)
    {
      for (nbk = 0; nbk < NENDGAME_MAX; nbk++)
      {
        for (nbm = 0; nbm < NENDGAME_MAX; nbm++)
        {
          endgame_t *with = &(endgames[nwk][nwm][nbk][nbm]);

          with->endgame_status = INVALID;

          strncpy(with->endgame_name, "NULL", MY_LINE_MAX);

          with->endgame_ref = NULL;

          with->endgame_unavailable_warning = FALSE;

          with->fendgame = NULL;
          with->fendgame_wdl = NULL;
          with->pendgame = NULL;

          with->endgame_nreads = 0;
          with->endgame_nentry_hits = 0;
          with->endgame_npage_hits = 0;
        }
      }
    }
  }

  i64_t endgame_ram = 0;

  for (nwk = 0; nwk < NENDGAME_MAX; nwk++)
  {
    for (nwm = 0; nwm < NENDGAME_MAX; nwm++)
    {
      for (nbk = 0; nbk < NENDGAME_MAX; nbk++)
      {
        for (nbm = 0; nbm < NENDGAME_MAX; nbm++)
        {
          endgame_t *with_endgame = &(endgames[nwk][nwm][nbk][nbm]);

          if (with_endgame->endgame_status != INVALID) continue;

          snprintf(with_endgame->endgame_name, MY_LINE_MAX,
                   "%dwX-%dwO-%dbX-%dbO", nwk, nwm, nbk, nbm);
          
          snprintf(path, MY_LINE_MAX, "%s/%s",
            options.egtb_dir, with_endgame->endgame_name);
  
          if (!fexists(path))
          {
            with_endgame->endgame_status = ENDGAME_UNAVAILABLE;
        
            with_endgame->endgame_unavailable_warning = TRUE;

            continue;
          }

          struct stat st;
  
          stat(path, &st);

          HARDBUG((with_endgame->fendgame = fopen(path, "r+b")) == NULL)

          HARDBUG(compat_mutex_init(&(with_endgame->endgame_fmutex)) != 0)

          i8_t version;
          i16_t nblock_entries;
          i64_t index_max;

          HARDBUG(fread(&version, sizeof(i8_t), 1, with_endgame->fendgame) != 1)

          HARDBUG(version != EGTB_COMPRESSED_VERSION)

          HARDBUG(fread(&nblock_entries, sizeof(i16_t), 1,
                    with_endgame->fendgame) != 1)

          HARDBUG(nblock_entries != NPAGE_ENTRIES)

          HARDBUG(fread(&index_max, sizeof(i64_t), 1,
                    with_endgame->fendgame) != 1)
 
          if (options.verbose > 1)
            PRINTF("version=%d nblock_entries=%d index_max=%lld\n",
              version, nblock_entries, index_max);

          i64_t npages = (index_max + 1) / nblock_entries;

          if ((npages * nblock_entries) < (index_max + 1)) npages++;

          HARDBUG((npages * nblock_entries) < (index_max + 1))
                
          //not true because of last block
          //i64_t uncompressed_size = 2 * (index_max + 1);

          i64_t nentries = npages * nblock_entries;

          i64_t uncompressed_size = sizeof(entry_i8_t)  * nentries;

          if (options.verbose > 1)
            PRINTF("npages=%lld nentries=%lld uncompressed_size=%lld\n",
              npages, nentries, uncompressed_size);

          //always open WDL

          char wdl[MY_LINE_MAX];

          snprintf(wdl, MY_LINE_MAX, "%s/%s.wdl",
            options.egtb_dir, with_endgame->endgame_name);

          HARDBUG(!fexists(wdl))
  
          HARDBUG((with_endgame->fendgame_wdl = fopen(wdl, "r+b")) == NULL)

          i8_t wdl_version;
  
          HARDBUG(fread(&wdl_version, sizeof(i8_t), 1,
                        with_endgame->fendgame_wdl) != 1)
      
          HARDBUG(wdl_version != EGTB_WDL_VERSION)
      
          if (options.verbose > 1)
            PRINTF("wdl_version=%d\n", wdl_version);

          //each entry consists of wtm_mate and btm_mate
          //each encoded as 2 bits so 4 bits (1 byte) in total

          HARDBUG((nentries % 2) != 0)

          i64_t nwdl = nentries / 2;
            
          i64_t wdl_size = nwdl * sizeof(wdl_t);

          if (options.verbose > 1)
            PRINTF("nwdl=%lld wdl_size=%lld\n", nwdl, wdl_size);

          //optionally load WDL in RAM

          cJSON *egtbs = NULL;

          if (compat_strcasecmp(options.egtb_ram_wdl, "NULL") == 0)
          {
            egtbs = cJSON_CreateArray();
          }
          else
          {
            egtbs =
             cJSON_GetObjectItem(gwd_json, options.egtb_ram_wdl);
          }
 
          HARDBUG(!cJSON_IsArray(egtbs))

          cJSON *egtb;

          cJSON_ArrayForEach(egtb, egtbs)
          {
            HARDBUG(!cJSON_IsString(egtb))

            if (compat_strcasecmp(with_endgame->endgame_name,
                              cJSON_GetStringValue(egtb)) != 0) continue;

            if (my_mpi_globals.MY_MPIG_nglobal <= 1)
            {
              MY_MALLOC(with_endgame->pendgame, wdl_t, nwdl)
            }
            else
            {
#ifdef USE_OPENMPI
              MPI_Win_allocate_shared(
                my_mpi_globals.MY_MPIG_id_global == 0 ? wdl_size : 0,
                sizeof(wdl_t), MPI_INFO_NULL,
                my_mpi_globals.MY_MPIG_comm_global,
                &(with_endgame->pendgame), &(with_endgame->wendgame));

              my_mpi_win_fence(my_mpi_globals.MY_MPIG_comm_global,
                               0, with_endgame->wendgame);

              int disp_unit;
              MPI_Aint ssize;

              MPI_Win_shared_query(with_endgame->wendgame, 0,
                &ssize, &disp_unit, &(with_endgame->pendgame));

              PRINTF("ssize=%lld disp_unit=%d\n",
                (long long) ssize, disp_unit);

              my_mpi_win_fence(my_mpi_globals.MY_MPIG_comm_global,
                               0, with_endgame->wendgame);
#else
              FATAL("my_mpi_globals.MY_MPIG_nglobal > 1", EXIT_FAILURE)
#endif
            }

            wdl_t *pwdl_t = with_endgame->pendgame;
            
            if ((my_mpi_globals.MY_MPIG_id_global == INVALID) or
                (my_mpi_globals.MY_MPIG_id_global == 0))
            {
              for (i64_t iwdl = 0; iwdl < nwdl; iwdl++) pwdl_t[iwdl] = 0;

              if (options.verbose > 1)
                PRINTF("loading wdl EGTB %s into RAM..\n", path);

              HARDBUG(fread(with_endgame->pendgame, sizeof(i8_t), wdl_size,
                            with_endgame->fendgame_wdl) != wdl_size)
  
                if (options.verbose > 1)
                  PRINTF("..done size in RAM=%lld\n", wdl_size);
            }
#ifdef USE_OPENMPI
            if (my_mpi_globals.MY_MPIG_nglobal > 1)
              my_mpi_win_fence(my_mpi_globals.MY_MPIG_comm_global,
                                0, with_endgame->wendgame);

            if ((my_mpi_globals.MY_MPIG_id_global != INVALID) and
                (my_mpi_globals.MY_MPIG_id_global != 0))
            {
              PRINTF("sharing wdl EGTB %s in RAM\n", path);
            }
#endif

            endgame_ram += wdl_size;

            with_endgame->endgame_status = ENDGAME_ALPHA_BETA_WDL;

            break;
          }//cJSON_ArrayForEach(egtb, egtbs)

          if (with_endgame->endgame_status == INVALID)
          {
            egtbs = NULL;

            if (compat_strcasecmp(options.egtb_ram, "NULL") == 0)
            {
              egtbs = cJSON_CreateArray();
            }
            else
            {
              egtbs = cJSON_GetObjectItem(gwd_json, options.egtb_ram);
            }
 
            HARDBUG(!cJSON_IsArray(egtbs))

            cJSON_ArrayForEach(egtb, egtbs)
            {
              HARDBUG(!cJSON_IsString(egtb))

              if (compat_strcasecmp(with_endgame->endgame_name,
                                cJSON_GetStringValue(egtb)) != 0) continue;

              MY_MALLOC(with_endgame->pendgame, i8_t, st.st_size)

              PRINTF("loading compressed EGTB %s into RAM..\n", path);
                
              HARDBUG(compat_fseeko(with_endgame->fendgame, 0, SEEK_SET) != 0)
  
              HARDBUG(fread(with_endgame->pendgame, 1, st.st_size,
                        with_endgame->fendgame) != st.st_size)
  
              PRINTF("..done size in RAM=%lld\n", (i64_t) st.st_size);

              endgame_ram += st.st_size;

              with_endgame->endgame_status = ENDGAME_ALPHA_BETA_COMPRESSED;

              break;
            }
          }

          if (with_endgame->endgame_status == INVALID)
          {
            HARDBUG(compat_fseeko(with_endgame->fendgame, 0, SEEK_SET) != 0)

            with_endgame->endgame_status = ENDGAME_ROOT_ONLY;
          }
        
          HARDBUG(with_endgame->endgame_status == INVALID)

          if ((nwk != nbk) or (nwm != nbm))
          {
            endgame_t *with_mirror = &(endgames[nbk][nbm][nwk][nwm]);
        
            HARDBUG(with_mirror->endgame_status == ENDGAME_ALPHA_BETA_WDL)
            HARDBUG(with_mirror->endgame_status ==
                    ENDGAME_ALPHA_BETA_COMPRESSED)
            HARDBUG(with_mirror->endgame_status == ENDGAME_ROOT_ONLY)

            with_mirror->endgame_status = ENDGAME_MIRROR;

            with_mirror->endgame_ref = with_endgame;
          }
        }
      }
    }
  }

  PRINTF("endgame_ram=%lld MBYTE\n", endgame_ram / MBYTE + 1);

  for (nwk = 0; nwk < NENDGAME_MAX; nwk++)
  {
    for (nwm = 0; nwm < NENDGAME_MAX; nwm++)
    {
      for (nbk = 0; nbk < NENDGAME_MAX; nbk++)
      {
        for (nbm = 0; nbm < NENDGAME_MAX; nbm++)
        {
          endgame_t *with_endgame = &(endgames[nwk][nwm][nbk][nbm]);

          if (with_endgame->endgame_status == ENDGAME_UNAVAILABLE) continue;

          PRINTF("%d %d %d %d %s", nwk, nwm, nbk, nbm,
                 with_endgame->endgame_name);

          if (with_endgame->endgame_status ==
              ENDGAME_ALPHA_BETA_WDL)
          {
            PRINTF(" alpha-beta wdl\n");
          }
          else if (with_endgame->endgame_status ==
                   ENDGAME_ALPHA_BETA_COMPRESSED)
          {
            PRINTF(" alpha-beta compressed\n");
          }
          else if (with_endgame->endgame_status == ENDGAME_ROOT_ONLY)
          {
            PRINTF(" root-only\n");
          }
          else if (with_endgame->endgame_status == ENDGAME_MIRROR)
          {
            PRINTF(" mirror\n");
          }
          else
            FATAL("unknown with_endgame->endgame_status", EXIT_FAILURE) 
        }
      }
    }
  }
  PRINTF("..done\n");
}

void update_endgame_index(int arg_nlist, int arg_list[NENDGAME_MAX],
  i64_t *endgame_index)
{
  if (arg_nlist == 0) return;
  if (arg_nlist == 1)
  {
    int list0;

    list0 = arg_list[0];

    *endgame_index = *endgame_index * 50 + list0;
  }
  else if (arg_nlist == 2)
  {
    int list0, list1;

    list0 = arg_list[0];
    list1 = arg_list[1];
      
    SOFTBUG(list0 > list1)

    *endgame_index = *endgame_index * SUM2(49) +
      SUM2(list0) + list1 - list0 - 1;
  }
  else if (arg_nlist == 3)
  {
    int list0, list1, list2;

    list0 = arg_list[0];
    list1 = arg_list[1];
    list2 = arg_list[2];
      
    SOFTBUG(list0 > list1)
    SOFTBUG(list1 > list2)

    *endgame_index = *endgame_index * SUM3(49) + SUM3(list0) +
      SUM32(list0, list1) + list2 - list1 - 1;
  }  
  else if (arg_nlist == 4)
  {
    int list0, list1, list2, list3;

    list0 = arg_list[0];
    list1 = arg_list[1];
    list2 = arg_list[2];
    list3 = arg_list[3];
     
    SOFTBUG(list0 > list1)
    SOFTBUG(list1 > list2)
    SOFTBUG(list2 > list3)

    *endgame_index = *endgame_index * SUM4(49) + SUM4(list0) +
      SUM43(list0, list1) + SUM32(list1, list2) + list3 - list2 - 1;
  }
  else if (arg_nlist == 5)
  {
    int list0, list1, list2, list3, list4;

    list0 = arg_list[0];
    list1 = arg_list[1];
    list2 = arg_list[2];
    list3 = arg_list[3];
    list4 = arg_list[4];
      
    SOFTBUG(list0 > list1)
    SOFTBUG(list1 > list2)
    SOFTBUG(list2 > list3)
    SOFTBUG(list3 > list4)

    *endgame_index = *endgame_index * SUM5(49) + SUM5(list0) +
      SUM54(list0, list1) + SUM43(list1, list2) +
      SUM32(list2, list3) + list4 - list3 - 1;
  }
  else
    FATAL("nlist > 5 not yet implemented", EXIT_FAILURE)
}

#define ENCODE_INVALID   (0)
#define ENCODE_DRAW      (1)
#define ENCODE_16BIT     2
#define ENCODE_8BIT_GT_0 3
#define ENCODE_8BIT_LE_0 4
#define ENCODE_8BIT      5

#ifdef ENABLE_RECOMPRESSION
local void decode(i8_t *arg_flag, i16_t *arg_shift, ui8_t arg_code[ARRAY_PAGE_SIZE],
  entry_i16_t *arg_entries)
{
  entry_i16_t entry_i16_default = {MATE_DRAW, MATE_DRAW};

  if (*arg_flag == ENCODE_DRAW)
  {
    for (int ientry = 0; ientry < ARRAY_PAGE_SIZE / 4; ientry++)
    {
      entry_i16_t *entry = arg_entries + ientry;

      memcpy(entry, &entry_i16_default, sizeof(entry_i16_t));
    }
  }
  else if (*arg_flag == ENCODE_16BIT)
  {
    memcpy(arg_entries, arg_code, ARRAY_PAGE_SIZE);
  }
  else if (*arg_flag == ENCODE_8BIT_GT_0)
  {
    int icode = 0;

    for (int ientry = 0; ientry < ARRAY_PAGE_SIZE / 4; ientry++)
    {
      entry_i16_t *entry = arg_entries + ientry;

      //code[icode++] = (entry->entry_white_mate - mate_min) / 2;

      entry->entry_white_mate = 2 * (i16_t) arg_code[icode++] + *arg_shift;

      entry->entry_black_mate = 2 * (i16_t) arg_code[icode++] + *arg_shift;
    } 
    SOFTBUG(icode != (ARRAY_PAGE_SIZE / 2))
  }
  else if (*arg_flag == ENCODE_8BIT_LE_0)
  {
    int icode = 0;

    for (int ientry = 0; ientry < ARRAY_PAGE_SIZE / 4; ientry++)
    {
      entry_i16_t *entry = arg_entries + ientry;

      //if (entry->entry_white_mate == MATE_DRAW) 
      //  code[icode++] = 0;
      //else
      //  code[icode++] = (2 - entry->entry_white_mate) / 2;

      if (arg_code[icode] == 0)
        entry->entry_white_mate = MATE_DRAW;
      else
        entry->entry_white_mate = 2 - 2 * (i16_t) arg_code[icode];

      icode++;

      if (arg_code[icode] == 0)
        entry->entry_black_mate = MATE_DRAW;
      else
        entry->entry_black_mate = 2 - 2 * (i16_t) arg_code[icode];

      icode++;
    } 
    SOFTBUG(icode != (ARRAY_PAGE_SIZE / 2))
  }
  else if (*arg_flag == ENCODE_8BIT)
  {
    int icode = 0;

    for (int ientry = 0; ientry < ARRAY_PAGE_SIZE / 4; ientry++)
    {
      entry_i16_t *entry = arg_entries + ientry;
   
      //if (entry->entry_white_mate == MATE_DRAW) 
      //  code[icode++] = 0;
      //else if (entry->entry_white_mate > 0)
      //  code[icode++] = (entry->entry_white_mate + 1) / 2;
      //else
      //  code[icode++] = (mate_max + 3 - entry->entry_white_mate) / 2;
      
      if (arg_code[icode] == 0)
        entry->entry_white_mate = MATE_DRAW;
      else if (arg_code[icode] >= ((*arg_shift + 3) / 2))
        entry->entry_white_mate = *arg_shift + 3 - 2 * (i16_t) arg_code[icode];
      else
        entry->entry_white_mate = 2 * (i16_t) arg_code[icode] - 1;

      icode++;

      if (arg_code[icode] == 0)
        entry->entry_black_mate = MATE_DRAW;
      else if (arg_code[icode] >= ((*arg_shift + 3) / 2))
        entry->entry_black_mate = *arg_shift + 3 - 2 * (i16_t) arg_code[icode];
      else
        entry->entry_black_mate = 2 * (i16_t) arg_code[icode] - 1;

      icode++;
    }
    SOFTBUG(icode != (ARRAY_PAGE_SIZE / 2))
  }
  else
  {
    PRINTF("*flag=%d\n", *arg_flag);
    FATAL("invalid flag", EXIT_FAILURE)
  }
}
#endif

int read_endgame(search_t *object, int arg_colour2move, int *arg_wdl)
{
  BEGIN_BLOCK(__FUNC__)

  int result = ENDGAME_UNKNOWN;

  int nwk = BIT_COUNT(object->S_board.board_white_king_bb);
  int nwm = BIT_COUNT(object->S_board.board_white_man_bb);

  int nbk = BIT_COUNT(object->S_board.board_black_king_bb);
  int nbm = BIT_COUNT(object->S_board.board_black_man_bb);

  int npieces = nwk + nwm + nbk + nbm;

  if (npieces > NENDGAME_MAX)
  {
    result = ENDGAME_UNKNOWN;
    goto label_return;
  }

  if (options.egtb_level < NENDGAME_MAX)
  {
    if (egtb_level_warning)
    {
      PRINTF("WARNING: ONLY EGTB WITH AT MOST %d PIECES WILL BE USED!\n",
        options.egtb_level);

      egtb_level_warning = FALSE;
    }
    if (npieces > options.egtb_level)
    {
      result = ENDGAME_UNKNOWN;
      goto label_return;
    }
  }

  //just in case

  if (nwk >= NENDGAME_MAX)
  {
    result = ENDGAME_UNKNOWN;
    goto label_return;
  }
  if (nwm >= NENDGAME_MAX)
  {
    result = ENDGAME_UNKNOWN;
    goto label_return;
  }
  if (nbk >= NENDGAME_MAX)
  {
    result = ENDGAME_UNKNOWN;
    goto label_return;
  }
  if (nbm >= NENDGAME_MAX)
  {
    result = ENDGAME_UNKNOWN;
    goto label_return;
  }

  if ((nwk + nwm) == 0)
  {
    if (IS_WHITE(arg_colour2move))
    {
      result = 0;
      if (arg_wdl != NULL) *arg_wdl = FALSE;
      goto label_return;
    }
    else
    {
      result = ENDGAME_UNKNOWN;
      goto label_return;
    }
  }
  if ((nbk + nbm) == 0)
  {
    if (IS_BLACK(arg_colour2move))
    {
      result = 0;
      if (arg_wdl != NULL) *arg_wdl = FALSE;
      goto label_return;
    }
    else
    {
      result = ENDGAME_UNKNOWN;
      goto label_return;
    }
  }

  endgame_t *with_endgame = &(endgames[nwk][nwm][nbk][nbm]);

  if (with_endgame->endgame_status == ENDGAME_UNAVAILABLE)
  {
    if (with_endgame->endgame_unavailable_warning)
    {
      my_printf(object->S_my_printf,
        "WARNING: endgame database %s not available\n",
        with_endgame->endgame_name);

      with_endgame->endgame_unavailable_warning = FALSE;
    }
    result = ENDGAME_UNKNOWN;
    goto label_return;
  }

  int mirror = FALSE;

  if (with_endgame->endgame_status == ENDGAME_MIRROR)
  {
    mirror = TRUE;

    with_endgame = with_endgame->endgame_ref;

    HARDBUG(with_endgame == NULL)
  }

  if ((arg_wdl != NULL) and
      (with_endgame->endgame_status == ENDGAME_ROOT_ONLY))
  {
    *arg_wdl = FALSE;

    result = ENDGAME_UNKNOWN;

    goto label_return;
  }
  
  i64_t endgame_index = 0;

  int white_king_sort[NENDGAME_MAX];
  int white_man_sort[NENDGAME_MAX];

  int black_king_sort[NENDGAME_MAX];
  int black_man_sort[NENDGAME_MAX];

  if (mirror)
  {
    ui64_t bb;
    int nbb;

    bb = object->S_board.board_white_king_bb; 
    nbb = 0;

    while(bb != 0)
    {
      int lsb = BIT_CTZ(bb);
      white_king_sort[nwk - 1 - nbb++] = 49 - inverse_map[lsb];
      bb &= ~BITULL(lsb);
    }
    HARDBUG(nbb != nwk)

    bb = object->S_board.board_white_man_bb; 
    nbb = 0;

    while(bb != 0)
    {
      int lsb = BIT_CTZ(bb);
      white_man_sort[nwm - 1 - nbb++] = 49 - inverse_map[lsb];
      bb &= ~BITULL(lsb);
    }
    HARDBUG(nbb != nwm)

    bb = object->S_board.board_black_king_bb; 
    nbb = 0;

    while(bb != 0)
    {
      int lsb = BIT_CTZ(bb);
      black_king_sort[nbk - 1 - nbb++] = 49 - inverse_map[lsb];
      bb &= ~BITULL(lsb);
    }
    HARDBUG(nbb != nbk)

    bb = object->S_board.board_black_man_bb; 
    nbb = 0;

    while(bb != 0)
    {
      int lsb = BIT_CTZ(bb);
      black_man_sort[nbm - 1 - nbb++] = 49 - inverse_map[lsb];
      bb &= ~BITULL(lsb);
    }
    HARDBUG(nbb != nbm)
  }
  else
  {
    ui64_t bb;
    int nbb;

    bb = object->S_board.board_white_king_bb; 
    nbb = 0;

    while(bb != 0)
    {
      int lsb = BIT_CTZ(bb);
      white_king_sort[nbb++] = inverse_map[lsb];
      bb &= ~BITULL(lsb);
    }
    HARDBUG(nbb != nwk)

    bb = object->S_board.board_white_man_bb;
    nbb = 0;
  
    while(bb != 0)
    {
      int lsb = BIT_CTZ(bb);
      white_man_sort[nbb++] = inverse_map[lsb];
      bb &= ~BITULL(lsb);
    }
    HARDBUG(nbb != nwm)

    bb = object->S_board.board_black_king_bb; 
    nbb = 0;

    while(bb != 0)
    {
      int lsb = BIT_CTZ(bb);
      black_king_sort[nbb++] = inverse_map[lsb];
      bb &= ~BITULL(lsb);
    }
    HARDBUG(nbb != nbk)

    bb = object->S_board.board_black_man_bb;
    nbb = 0;
  
    while(bb != 0)
    {
      int lsb = BIT_CTZ(bb);
      black_man_sort[nbb++] = inverse_map[lsb];
      bb &= ~BITULL(lsb);
    }
    HARDBUG(nbb != nbm)
  }

  if (mirror)
  {
    update_endgame_index(nbk, black_king_sort, &endgame_index);

    update_endgame_index(nbm, black_man_sort, &endgame_index);

    update_endgame_index(nwk, white_king_sort, &endgame_index);

    update_endgame_index(nwm, white_man_sort, &endgame_index);
  } 
  else
  {
    update_endgame_index(nwk, white_king_sort, &endgame_index);

    update_endgame_index(nwm, white_man_sort, &endgame_index);

    update_endgame_index(nbk, black_king_sort, &endgame_index);

    update_endgame_index(nbm, black_man_sort, &endgame_index);
  }

  HARDBUG(endgame_index < 0)

  HARDBUG((with_endgame->fendgame == NULL) and (with_endgame->pendgame == NULL))

  entry_i16_t entry;
 
  entry.entry_white_mate = INVALID;
  entry.entry_black_mate = INVALID;

  with_endgame->endgame_nreads++;

  if ((arg_wdl != NULL) and
      (with_endgame->endgame_status == ENDGAME_ALPHA_BETA_WDL))
  {
    *arg_wdl = TRUE;

    wdl_t *pwdl_t = with_endgame->pendgame;

    i64_t iwdl = endgame_index / 2;

    int jwdl = endgame_index % 2;

    HARDBUG(pwdl_t[iwdl] == 0)

    ui8_t wdl_entry = pwdl_t[iwdl] >> (jwdl * 4);
    
    HARDBUG(wdl_entry == 0)
     
    if ((wdl_entry & 0x3) == WDL_DRAW)
    {
      entry.entry_white_mate = INVALID;
    }
    else if ((wdl_entry & 0x3) == WDL_WON)
    {
      entry.entry_white_mate = 1;
    }
    else if ((wdl_entry & 0x3) == WDL_LOST)
    {
      entry.entry_white_mate = -2;
    }
    else
      FATAL("unknown wdl_entry", EXIT_FAILURE)

    wdl_entry = (wdl_entry >> 2) & 0x3;

    if ((wdl_entry & 0x3) == WDL_DRAW)
    {
      entry.entry_black_mate = INVALID;
    }
    else if ((wdl_entry & 0x3) == WDL_WON)
    {
      entry.entry_black_mate = 1;
    }
    else if ((wdl_entry & 0x3) == WDL_LOST)
    {
      entry.entry_black_mate = -2;
    }
    else
      FATAL("unknown wdl_entry", EXIT_FAILURE)
  }
  else if (arg_wdl != NULL)
  {
    *arg_wdl = TRUE;

    //check if the entry is in the WDL entry cache

    i64_t endgame_id = nwk * (NENDGAME_MAX * NENDGAME_MAX * NENDGAME_MAX) +
                       nwm * (NENDGAME_MAX * NENDGAME_MAX) +
                       nbk * (NENDGAME_MAX) +
                       nbm;
  
    i64_t endgame_index_key = (endgame_index << 16) | endgame_id;
  
    if (check_entry_in_cache(&(object->S_endgame_wdl_entry_cache),
                             &endgame_index_key, &entry))
    {
      with_endgame->endgame_nentry_hits++;

      goto label_result;
    }

    wdl_t vwdl;

    i64_t iwdl = endgame_index / 2;

    int jwdl = endgame_index % 2;

    HARDBUG(compat_fseeko(with_endgame->fendgame_wdl,
                          sizeof(i8_t) + iwdl * sizeof(wdl_t), SEEK_SET) != 0)
                    
    HARDBUG(fread(&vwdl, sizeof(wdl_t), 1, with_endgame->fendgame_wdl) != 1)

    HARDBUG(vwdl == 0)

    ui8_t wdl_entry = vwdl >> (jwdl * 4);
    
    HARDBUG(wdl_entry == 0)
     
    if ((wdl_entry & 0x3) == WDL_DRAW)
    {
      entry.entry_white_mate = INVALID;
    }
    else if ((wdl_entry & 0x3) == WDL_WON)
    {
      entry.entry_white_mate = 1;
    }
    else if ((wdl_entry & 0x3) == WDL_LOST)
    {
      entry.entry_white_mate = -2;
    }
    else
      FATAL("unknown wdl_entry", EXIT_FAILURE)

    wdl_entry = (wdl_entry >> 2) & 0x3;

    if ((wdl_entry & 0x3) == WDL_DRAW)
    {
      entry.entry_black_mate = INVALID;
    }
    else if ((wdl_entry & 0x3) == WDL_WON)
    {
      entry.entry_black_mate = 1;
    }
    else if ((wdl_entry & 0x3) == WDL_LOST)
    {
      entry.entry_black_mate = -2;
    }
    else
      FATAL("unknown wdl_entry", EXIT_FAILURE)

    store_entry_in_cache(&(object->S_endgame_wdl_entry_cache),
                         &endgame_index_key, &entry);
  }
  else
  { 
    if (arg_wdl != NULL) *arg_wdl = FALSE;

    //check if the entry is in the entry cache

    i64_t endgame_id = nwk * (NENDGAME_MAX * NENDGAME_MAX * NENDGAME_MAX) +
                       nwm * (NENDGAME_MAX * NENDGAME_MAX) +
                       nbk * (NENDGAME_MAX) +
                       nbm;
  
    i64_t endgame_index_key = (endgame_index << 16) | endgame_id;
  
    if (check_entry_in_cache(&(object->S_endgame_entry_cache),
                             &endgame_index_key, &entry))
    {
      with_endgame->endgame_nentry_hits++;
      goto label_result;
    }
  
    i64_t ipage = (endgame_index * sizeof(entry_i16_t)) / ARRAY_PAGE_SIZE;

    entry_i16_t page[NPAGE_ENTRIES];
  
    //read the page from memory or file

#ifndef ZSTD
    i64_t seek = 
      sizeof(i64_t) + ipage * (sizeof(i64_t) + sizeof(i16_t) + sizeof(ui64_t));
#else
    i64_t seek = sizeof(i8_t) + sizeof(i16_t) + sizeof(i64_t) +
                 ipage * (sizeof(i64_t) + sizeof(i16_t) + sizeof(ui32_t));
#endif

    i64_t offset;
    i16_t length;
#ifndef ZSTD
    ui64_t crc64;
#else
#ifdef DEBUG
    ui32_t crc32;
#endif
#endif

#ifndef ZSTD
    COMPRESS_LEN_T sourceLen;
    COMPRESS_BYTE_T source[1 + 2 * (2 + ARRAY_PAGE_SIZE)];
#else
    i8_t source[2 * ARRAY_PAGE_SIZE];
    size_t sourceLen;
#endif

    if (with_endgame->endgame_status == ENDGAME_ALPHA_BETA_COMPRESSED)
    {
      memcpy(&offset, (i8_t *) with_endgame->pendgame + seek, sizeof(i64_t));
  
      seek += sizeof(i64_t);
  
      memcpy(&length, (i8_t *) with_endgame->pendgame + seek, sizeof(i16_t));
  
      seek += sizeof(i16_t);

#ifndef ZSTD
      memcpy(&crc64, (i8_t *) with_endgame->pendgame + seek, sizeof(ui64_t));
#else
#ifdef DEBUG
      memcpy(&crc32, (i8_t *) with_endgame->pendgame + seek, sizeof(ui32_t));
#endif
#endif

      HARDBUG (length <= 0)
#ifndef ZSTD
      HARDBUG(length > (1 + 2 * (2 + ARRAY_PAGE_SIZE)))
#else
      HARDBUG(length > (2 + ARRAY_PAGE_SIZE))
#endif

      sourceLen = length;

#ifndef ZSTD
      memcpy(source, (i8_t *) with_endgame->pendgame + offset,
             sizeof(COMPRESS_BYTE_T) * sourceLen);
#else
      memcpy(source, (i8_t *) with_endgame->pendgame + offset, sourceLen);
#endif
    }
    else
    {
      HARDBUG(compat_mutex_lock(&(with_endgame->endgame_fmutex)) != 0)

      HARDBUG(compat_fseeko(with_endgame->fendgame, seek, SEEK_SET) != 0)
      
      HARDBUG(fread(&offset, sizeof(i64_t), 1, with_endgame->fendgame) != 1)
  
      HARDBUG(fread(&length, sizeof(i16_t), 1, with_endgame->fendgame) != 1)

#ifndef ZSTD
      HARDBUG(fread(&crc64,  sizeof(ui64_t), 1, with_endgame->fendgame) != 1)
#else
#ifdef DEBUG
      HARDBUG(fread(&crc32,  sizeof(ui32_t), 1, with_endgame->fendgame) != 1)
#endif
#endif
      HARDBUG(length <= 0)
#ifndef ZSTD
      HARDBUG(length > (1 + 2 * (2 + ARRAY_PAGE_SIZE)))
#else
      HARDBUG(length > (2 + ARRAY_PAGE_SIZE))
#endif

      sourceLen = length;

      HARDBUG(compat_fseeko(with_endgame->fendgame, offset, SEEK_SET) != 0)

#ifndef ZSTD
      HARDBUG(fread(source, sizeof(COMPRESS_BYTE_T), sourceLen, 
                with_endgame->fendgame) != sourceLen)
#else
      HARDBUG(fread(source, sizeof(i8_t), sourceLen, 
                with_endgame->fendgame) != sourceLen)
#endif
  
      HARDBUG(compat_mutex_unlock(&(with_endgame->endgame_fmutex)) != 0)
    }

#ifndef ZSTD
    //exclude source[0]
    --sourceLen;

    COMPRESS_LEN_T destLen;
    COMPRESS_BYTE_T dest[2 * (2 + ARRAY_PAGE_SIZE)];

    if (source[0] != ENCODE_DRAW)
    {
      int err;
  
      destLen = 2 * (2 + ARRAY_PAGE_SIZE);
  
      if ((err = UNCOMPRESS(dest, &destLen, source + 1, sourceLen)) !=
           COMPRESS_OK)
      {
        PRINTF("err=%d\n", err);
        PRINTF("endgame=%s\n", with_endgame->endgame_name);
        FATAL("uncompress error", EXIT_FAILURE)
      }
  
      if (source[0] == ENCODE_16BIT)
        HARDBUG(destLen != (2 + ARRAY_PAGE_SIZE))
      else
        HARDBUG(destLen != (2 + ARRAY_PAGE_SIZE / 2))
    }

    decode((i8_t *) source, (i16_t *) dest, (ui8_t *) (dest + 2), page);

    HARDBUG(return_crc64(page, ARRAY_PAGE_SIZE, 0) != crc64)

#else
    if (source[0] == ALL_DRAW)
    {
#ifdef DEBUG
      ui32_t temp_crc32 = 0xFFFFFFFF;
    
      temp_crc32 = _mm_crc32_u8(temp_crc32, source[0]);
  
      HARDBUG(temp_crc32 != crc32)
#endif

      entry.entry_white_mate = INVALID;
      entry.entry_black_mate = INVALID;
    }
    else if (source[0] == ALL_I8)
    {
      entry_i8_t dest[NPAGE_ENTRIES];
      size_t destLen = sizeof(dest);
      
      //exclude source[0]
      --sourceLen;
    
      destLen = ZSTD_decompress(dest, destLen,
                                source + 1, sourceLen);
  
      HARDBUG(ZSTD_isError(destLen))
    
      HARDBUG(destLen != sizeof(dest))

#ifdef DEBUG

#ifdef USE_HARDWARE_CRC32

      ui32_t temp_crc32 = 0xFFFFFFFF;
  
      temp_crc32 = _mm_crc32_u8(temp_crc32, source[0]);

      for (int ientry = 0; ientry < NPAGE_ENTRIES; ientry++)
      {
        temp_crc32 = _mm_crc32_u8(temp_crc32, dest[ientry].entry_white_mate);
        temp_crc32 = _mm_crc32_u8(temp_crc32, dest[ientry].entry_black_mate);
      }

#else

#error NOT IMPLEMENTED YET

#endif

      HARDBUG(temp_crc32 != crc32)
#endif

      int ientry = endgame_index  % NPAGE_ENTRIES;

      entry.entry_white_mate = dest[ientry].entry_white_mate;
      entry.entry_black_mate = dest[ientry].entry_black_mate;
    }
    else if (source[0] == ALL_I16)
    {
      size_t destLen = sizeof(page);
    
      //exclude source[0]
      --sourceLen;
    
      destLen = ZSTD_decompress(page, destLen,
                                source + 1, sourceLen);
    
      HARDBUG(ZSTD_isError(destLen))
    
      HARDBUG(destLen != sizeof(page))

#ifdef DEBUG

#ifdef USE_HARDWARE_CRC32

      ui32_t temp_crc32 = 0xFFFFFFFF;
    
      temp_crc32 = _mm_crc32_u8(temp_crc32, source[0]);
  
      for (int ientry = 0; ientry < NPAGE_ENTRIES; ientry++)
      {
        temp_crc32 = _mm_crc32_u16(temp_crc32, page[ientry].entry_white_mate);
        temp_crc32 = _mm_crc32_u16(temp_crc32, page[ientry].entry_black_mate);
      }

#else

#error NOT IMPLEMENTED YET

#endif

      HARDBUG(temp_crc32 != crc32)
#endif

      int ientry = endgame_index  % NPAGE_ENTRIES;

      entry.entry_white_mate = page[ientry].entry_white_mate;
      entry.entry_black_mate = page[ientry].entry_black_mate;
    }
    else
      FATAL("eh", EXIT_FAILURE)
#endif

    store_entry_in_cache(&(object->S_endgame_entry_cache),
                         &endgame_index_key, &entry);
  }

  label_result:

  if (mirror)
  {
    if (IS_WHITE(arg_colour2move))
      result = entry.entry_black_mate;
    else
      result = entry.entry_white_mate;
  }
  else
  {
    if (IS_WHITE(arg_colour2move))
      result = entry.entry_white_mate;
    else
      result = entry.entry_black_mate;
  }

  label_return:
  END_BLOCK

  if (arg_wdl != NULL)
    HARDBUG((result != ENDGAME_UNKNOWN) and (*arg_wdl != 0) and (*arg_wdl != 1))

  return(result);
}

void fin_endgame(void)
{
  PRINTF("fin_endgame\n");
  for (int nwk = 0; nwk < NENDGAME_MAX; nwk++)
  {
    for (int nwm = 0; nwm < NENDGAME_MAX; nwm++)
    {
      for (int nbk = 0; nbk < NENDGAME_MAX; nbk++)
      {
        for (int nbm = 0; nbm < NENDGAME_MAX; nbm++)
        {
          endgame_t *with = &(endgames[nwk][nwm][nbk][nbm]);

          if (with->pendgame != NULL)
          {
            PRINTF("ab %1d %1d %1d %1d %s", nwk, nwm, nbk, nbm, 
              with->endgame_name);
            PRINTF("%10lld %10lld %10lld\n",
              with->endgame_nreads,
              with->endgame_nentry_hits,
              with->endgame_npage_hits);
          }
          else if (with->fendgame != NULL)
          {
            PRINTF("ro %1d %1d %1d %1d %s", nwk, nwm, nbk, nbm, 
              with->endgame_name);
            PRINTF("%10lld %10lld %10lld\n",
              with->endgame_nreads,
              with->endgame_nentry_hits,
              with->endgame_npage_hits);
          }
        }
      }
    }
  }
}

#define NSTATS 1024
#define NBLOCK 4

void recompress_endgame(char *arg_name, int arg_level, int arg_nblock)
{
#ifdef ENABLE_RECOMPRESSION
  HARDBUG((NPAGE_ENTRIES % arg_nblock) != 0)

  i16_t nblock_entries = NPAGE_ENTRIES / arg_nblock;
 
  HARDBUG((ARRAY_PAGE_SIZE % arg_nblock) != 0)
  
  i16_t block_size = ARRAY_PAGE_SIZE / arg_nblock;
  
  PRINTF("nblock_entries=%d block_size\n", nblock_entries, block_size);

  PRINTF("recompressing %s with level %d\n", arg_name, arg_level);

  char name_new[MY_LINE_MAX];

  snprintf(name_new, MY_LINE_MAX, "/tmp8/gwies/dbase5zstdencdec3/%s", arg_name);

  i64_t compressed_size_old = 0;
  i64_t uncompressed_size_old = 0;
  i64_t size_new = 0;
 
  i64_t nall_draw = 0;
  i64_t nall_i8 = 0;
  i64_t nall_i16 = 0;
  i64_t nall_inflated = 0;

  i64_t compressed_size_new = 0;

  i64_t white_stats_old[NSTATS];
  i64_t black_stats_old[NSTATS];

  for (int istat= 0; istat < NSTATS; istat++)
    white_stats_old[istat] = black_stats_old[istat] = 0;

  FILE *fold;
  FILE *fnew;

  HARDBUG((fold = fopen(arg_name, "r+b")) == NULL)

  i64_t index_max_old;

  HARDBUG(fread(&index_max_old, sizeof(i64_t), 1, fold) != 1)
  
  PRINTF("index_max_old=%lld\n", index_max_old);

  HARDBUG((fnew = fopen(name_new, "w+b")) == NULL)

  i8_t version = EGTB_COMPRESSED_VERSION;

  HARDBUG(fwrite(&version, sizeof(i8_t), 1, fnew) != 1)

  HARDBUG(fwrite(&nblock_entries, sizeof(i16_t), 1, fnew) != 1)

  HARDBUG(fwrite(&index_max_old, sizeof(i64_t), 1, fnew) != 1)

  i64_t npage_old = (index_max_old + 1) * sizeof(entry_i16_t) / ARRAY_PAGE_SIZE;

  if ((npage_old * ARRAY_PAGE_SIZE) <=
      (index_max_old + 1) * sizeof(entry_i16_t)) npage_old++;

  HARDBUG(((index_max_old + 1) * sizeof(entry_i16_t) / ARRAY_PAGE_SIZE) >=
      npage_old)

  //last block is 'too large' so contains an 'extra' block

  //i64_t npage_new = (index_max_old + 1) / nblock_entries;
  //if ((npage_new * nblock_entries) < (index_max_old + 1)) npage_new++;

  i64_t npage_new = npage_old * arg_nblock;

  HARDBUG((npage_new * nblock_entries) < (index_max_old + 1))

  PRINTF("npage_old=%lld npage_new=%lld\n", npage_old, npage_new);

  i64_t fixed_size_old = sizeof(i64_t) +
    npage_old * (sizeof(i64_t) + sizeof(i16_t) + sizeof(ui32_t));

  i64_t fixed_size_new = sizeof(i8_t) + sizeof(i16_t) + sizeof(i64_t) +
    npage_new * (sizeof(i64_t) + sizeof(i16_t) + sizeof(ui32_t));

  size_new = fixed_size_new;

  i64_t ipage_new = 0;

  i64_t nbucket[ARRAY_PAGE_SIZE];

  for (int ibucket = 0; ibucket < ARRAY_PAGE_SIZE; ibucket++)
    nbucket[ibucket] = 0;

  for (i64_t ipage_old = 0; ipage_old < npage_old; ipage_old++)
  {
    if ((ipage_old % 100000) == 0)
    {
      PRINTF("ipage_old=%lld(%.2f%)\n", ipage_old,
             ipage_old * 100.0 / npage_old);
    }

    i64_t seek_old = sizeof(i64_t) +
      ipage_old * (sizeof(i64_t) + sizeof(i16_t) + sizeof(ui64_t));

    HARDBUG(compat_fseeko(fold, seek_old, SEEK_SET) != 0)
    
    i64_t offset_old;
    i16_t length_old;
    ui64_t crc64_old;

    COMPRESS_LEN_T sourceLen_old;
    COMPRESS_BYTE_T source_old[1 + 2 * (2 + ARRAY_PAGE_SIZE)];

    HARDBUG(fread(&offset_old, sizeof(i64_t), 1, fold) != 1)
    HARDBUG(fread(&length_old, sizeof(i16_t), 1, fold) != 1)
    HARDBUG(fread(&crc64_old,  sizeof(ui64_t), 1, fold) != 1)

    //some blocks may have offset INVALID

    if (offset_old == INVALID)
    {
      HARDBUG(length_old != 0)

      i64_t offset_new = INVALID;
      i16_t length_new = 0;
      ui32_t crc32 = 0xFFFFFFFF;

      for (int iblock = 0; iblock < arg_nblock; iblock++)
      {
        HARDBUG(ipage_new >= npage_new)

        i64_t seek_new = sizeof(i8_t) + sizeof(i16_t) + sizeof(i64_t) + 
          ipage_new * (sizeof(i64_t) + sizeof(i16_t) + sizeof(ui32_t));

        HARDBUG(compat_fseeko(fnew, seek_new, SEEK_SET) != 0)
  
        HARDBUG(fwrite(&offset_new, sizeof(i64_t), 1, fnew) != 1)
        HARDBUG(fwrite(&length_new, sizeof(i16_t), 1, fnew) != 1)
        HARDBUG(fwrite(&crc32,  sizeof(ui32_t), 1, fnew) != 1)
  
        ipage_new++;
      }

      continue;
    }

    HARDBUG(length_old <= 0)
    HARDBUG(length_old > (1 + 2 * (2 + ARRAY_PAGE_SIZE)))

    sourceLen_old = length_old;

    HARDBUG(compat_fseeko(fold, offset_old, SEEK_SET) != 0)

    HARDBUG(fread(source_old, sizeof(COMPRESS_BYTE_T), sourceLen_old, 
              fold) != sourceLen_old)

    compressed_size_old += length_old;

    //exclude source[0]
    --sourceLen_old;
  
    COMPRESS_LEN_T destLen_old;
    COMPRESS_BYTE_T dest_old[2 * (2 + ARRAY_PAGE_SIZE)];
  
    if (source_old[0] != ENCODE_DRAW)
    {
      int err;
  
      destLen_old = 2 * (2 + ARRAY_PAGE_SIZE);
  
      if ((err = UNCOMPRESS(dest_old, &destLen_old,
                            source_old + 1, sourceLen_old)) != COMPRESS_OK)
      {
        PRINTF("err=%d\n", err);
        PRINTF("name=%s\n", arg_name);
        FATAL("uncompress error", EXIT_FAILURE)
      }
  
      if (source_old[0] == ENCODE_16BIT)
        HARDBUG(destLen_old != (2 + ARRAY_PAGE_SIZE))
      else
        HARDBUG(destLen_old != (2 + ARRAY_PAGE_SIZE / 2))
    }
  
    entry_i16_t page_i16[NPAGE_ENTRIES];

    decode((i8_t *) source_old, (i16_t *) dest_old, (ui8_t *) (dest_old + 2),
           page_i16);
  
    HARDBUG(return_crc64(page_i16, ARRAY_PAGE_SIZE, 0) != crc64_old)
  
    uncompressed_size_old += ARRAY_PAGE_SIZE;

    for (int iblock = 0; iblock < arg_nblock; iblock++)
    {
      int jentry = iblock * nblock_entries;

      int all_draw = TRUE;
      int all_i8 = TRUE;

      for (int ientry = 0; ientry < nblock_entries; ientry++)
      {
        int white_mate = page_i16[jentry + ientry].entry_white_mate;
        int black_mate = page_i16[jentry + ientry].entry_black_mate;
  
        if (white_mate != INVALID) all_draw = FALSE;
        if (black_mate != INVALID) all_draw = FALSE;
  
        if (white_mate > 127) all_i8 = FALSE;
        if (white_mate < -128) all_i8 = FALSE;
  
        if (black_mate > 127) all_i8 = FALSE;
        if (black_mate < -128) all_i8 = FALSE;
  
        white_stats_old[white_mate + NSTATS /  2]++;
        black_stats_old[black_mate + NSTATS /  2]++;
      }
      if (all_draw) nall_draw++;
      if (all_i8) nall_i8++;
  
      //dest_new is compressed page
  
      i8_t dest_new[2 * ARRAY_PAGE_SIZE];
      size_t destLen_new = sizeof(dest_new);
  
      ui32_t crc32 = 0xFFFFFFFF;
  
      if (all_draw)
      {
#ifdef USE_HARDWARE_CRC32

        crc32 = _mm_crc32_u8(crc32, ALL_DRAW);

#else

#error NOT IMPLEMENTED YET

#endif

        dest_new[0] = ALL_DRAW;
  
        destLen_new = 1;
      }
      else if (all_i8)
      {
        entry_i8_t source_new[NPAGE_ENTRIES];
  
        size_t sourceLen_new = sizeof(entry_i8_t) * nblock_entries;
  
        for (int ientry = 0; ientry < nblock_entries; ientry++)
        { 
          source_new[ientry].entry_white_mate =
            page_i16[jentry + ientry].entry_white_mate;
          source_new[ientry].entry_black_mate =
            page_i16[jentry + ientry].entry_black_mate;
        }

#ifdef USE_HARDWARE_CRC32

        crc32 = _mm_crc32_u8(crc32, ALL_I8);
  
        for (int ientry = 0; ientry < nblock_entries; ientry++)
        {
          crc32 = _mm_crc32_u8(crc32, source_new[ientry].entry_white_mate);
          crc32 = _mm_crc32_u8(crc32, source_new[ientry].entry_black_mate);
        }

#else

#error NOT IMPLEMENTED YET

#endif
        dest_new[0] = ALL_I8;
        
        destLen_new = ZSTD_compress(dest_new + 1, destLen_new - 1,
          source_new, sourceLen_new, arg_level);
  
        if (ZSTD_isError(destLen_new))
        {
          PRINTF("ZSTD_compress error=%s\n", ZSTD_getErrorName(destLen_new));
          FATAL("ZSTD_compress error", EXIT_FAILURE)
        }
  
        if (destLen_new > sourceLen_new)
          nall_inflated++;
  
        nbucket[destLen_new]++;

        //decompress
   
        entry_i8_t source_newer[NPAGE_ENTRIES];
  
        size_t sourceLen_newer = sizeof(source_newer);
  
        sourceLen_newer = ZSTD_decompress(source_newer, sourceLen_newer,
                                          dest_new + 1, destLen_new);
  
        HARDBUG(ZSTD_isError(sourceLen_newer))

        HARDBUG(sourceLen_newer > ARRAY_PAGE_SIZE)
 
        for (int ientry = 0; ientry < nblock_entries; ientry++)
        {
          HARDBUG(source_newer[ientry].entry_white_mate !=
              source_new[ientry].entry_white_mate)
          HARDBUG(source_newer[ientry].entry_black_mate !=
              source_new[ientry].entry_black_mate)
        }

        destLen_new++;
      }
      else
      {
        nall_i16++;

#ifdef USE_HARDWARE_CRC32

        crc32 = _mm_crc32_u8(crc32, ALL_I16);

        for (int ientry = 0; ientry < nblock_entries; ientry++)
        {
          crc32 = _mm_crc32_u16(crc32,
                                page_i16[jentry + ientry].entry_white_mate);
          crc32 = _mm_crc32_u16(crc32,
                                page_i16[jentry + ientry].entry_black_mate);
        }

#else

#error NOT IMPLEMENTED YET

#endif
        size_t sourceLen_new = sizeof(entry_i16_t) * nblock_entries;
  
        dest_new[0] = ALL_I16;
        
        destLen_new = ZSTD_compress(dest_new + 1, destLen_new,
          page_i16 + jentry, sourceLen_new, arg_level);
  
        HARDBUG(ZSTD_isError(destLen_new))
  
        HARDBUG(destLen_new > sourceLen_new)
  
        nbucket[destLen_new]++;

        //decompress
   
        entry_i16_t source_newer[NPAGE_ENTRIES];
  
        size_t sourceLen_newer = sizeof(source_newer);

        sourceLen_newer = ZSTD_decompress(source_newer, sourceLen_newer,
                                          dest_new + 1, destLen_new);

        HARDBUG(ZSTD_isError(sourceLen_newer))

        HARDBUG(sourceLen_newer > ARRAY_PAGE_SIZE)

        for (int ientry = 0; ientry < nblock_entries; ientry++)
        {
          HARDBUG(source_newer[ientry].entry_white_mate !=
              page_i16[jentry + ientry].entry_white_mate)
          HARDBUG(source_newer[ientry].entry_black_mate !=
              page_i16[jentry + ientry].entry_black_mate)
        }
  
        destLen_new++;
      }

      compressed_size_new += destLen_new;

      HARDBUG(ipage_new >= npage_new)

      i64_t seek_new = sizeof(i8_t) + sizeof(i16_t) + sizeof(i64_t) +
        ipage_new * (sizeof(i64_t) + sizeof(i16_t) + sizeof(ui32_t));

      HARDBUG(compat_fseeko(fnew, seek_new, SEEK_SET) != 0)

      i16_t destLen_i16 = destLen_new;

      HARDBUG(fwrite(&size_new, sizeof(i64_t), 1, fnew) != 1)
      HARDBUG(fwrite(&destLen_i16, sizeof(i16_t), 1, fnew) != 1)
      HARDBUG(fwrite(&crc32,  sizeof(ui32_t), 1, fnew) != 1)

      HARDBUG(compat_fseeko(fnew, size_new, SEEK_SET) != 0)
  
      HARDBUG(fwrite(dest_new, sizeof(i8_t), destLen_new, fnew) != destLen_new)

      size_new += destLen_new;

      ipage_new++;
    }//iblock
  }

  PRINTF("npage_old=%lld npage_new=%lld\n", npage_old, npage_new);

  double compress_ratio_old = 100.0 - compressed_size_old * 100.0 /
                                      uncompressed_size_old;

  PRINTF("fixed_size_old=%lld\n", fixed_size_old);

  PRINTF("compressed_size_old=%lld uncompressed_size_old=%lld"
         " compress_ratio_old=%.2f\n",
    compressed_size_old, uncompressed_size_old, compress_ratio_old);

  PRINTF("nall_draw=%lld nall_i8=%lld nall_i16=%lld ratio_i8=%.2f\n",
    nall_draw, nall_i8, nall_i16, nall_i8 * 100.0 / npage_new);

  PRINTF("nall_inflated=%lld\n", nall_inflated);

  for (int ibucket = 0; ibucket < ARRAY_PAGE_SIZE; ibucket++)
  {  
    if (nbucket[ibucket] > 0) PRINTF("ibucket=%d nbucket=%lld\n",
      ibucket, nbucket[ibucket]);
  }

  double compress_ratio_new = 100.0 - compressed_size_new * 100.0 /
                                      uncompressed_size_old;

  PRINTF("fixed_size_new=%lld\n", fixed_size_new);

  PRINTF("compressed_size_new=%lld compress_ratio_new=%.2f\n",
    compressed_size_new, compress_ratio_new);

  PRINTF("size_new=%lld\n", size_new);
 
  FCLOSE(fold)
  FCLOSE(fnew)

  PRINTF("reading recompressed database..\n");

  i64_t white_stats_new[NSTATS];
  i64_t black_stats_new[NSTATS];

  for (int istat= 0; istat < NSTATS; istat++)
    white_stats_new[istat] = black_stats_new[istat] = 0;

  HARDBUG((fnew = fopen(name_new, "r+b")) == NULL)

  HARDBUG(fread(&version, sizeof(i8_t), 1, fnew) != 1)

  HARDBUG(version != EGTB_COMPRESSED_VERSION)

  HARDBUG(fread(&nblock_entries, sizeof(i16_t), 1, fnew) != 1)

  HARDBUG(nblock_entries != (NPAGE_ENTRIES / arg_nblock))

  i64_t index_max_new;

  HARDBUG(fread(&index_max_new, sizeof(i64_t), 1, fnew) != 1)
  
  PRINTF("index_max_new=%lld\n", index_max_new);

  HARDBUG(index_max_new != index_max_old)

  compressed_size_new = 0;

  for (ipage_new = 0; ipage_new < npage_new; ipage_new++)
  {
    if ((ipage_new % 100000) == 0)
    {
      PRINTF("ipage_new=%lld(%.2f%)\n", ipage_new,
             ipage_new * 100.0 / npage_new);
    }

    i64_t seek_new = sizeof(i8_t) + sizeof(i64_t) + sizeof(i16_t) + 
      ipage_new * (sizeof(i64_t) + sizeof(i16_t) + sizeof(ui32_t));

    HARDBUG(compat_fseeko(fnew, seek_new, SEEK_SET) != 0)
    
    i64_t offset_new;
    i16_t length_new;
    ui32_t crc32_new;

    HARDBUG(fread(&offset_new, sizeof(i64_t), 1, fnew) != 1)
    HARDBUG(fread(&length_new, sizeof(i16_t), 1, fnew) != 1)
    HARDBUG(fread(&crc32_new,  sizeof(ui32_t), 1, fnew) != 1)

    //some blocks may have offset INVALID

    if (offset_new == INVALID)
    {
      HARDBUG(length_new != 0)

      HARDBUG(crc32_new != 0xFFFFFFFF)

      continue;
    }

    HARDBUG(length_new <= 0)
    HARDBUG(length_new > (2 * ARRAY_PAGE_SIZE))

    i8_t source_new[2 * ARRAY_PAGE_SIZE];
    size_t sourceLen_new = length_new;

    HARDBUG(compat_fseeko(fnew, offset_new, SEEK_SET) != 0)

    HARDBUG(fread(source_new, sizeof(i8_t), sourceLen_new, 
              fnew) != sourceLen_new)

    compressed_size_new += sourceLen_new;

    if (source_new[0] == ALL_DRAW)
    {
      ui32_t crc32 = 0xFFFFFFFF;
  
      crc32 = _mm_crc32_u8(crc32, source_new[0]);

      HARDBUG(crc32 != crc32_new)
   
      white_stats_new[INVALID + NSTATS / 2] += NPAGE_ENTRIES;
      black_stats_new[INVALID + NSTATS / 2] += NPAGE_ENTRIES;
    }
    else if (source_new[0] == ALL_I8)
    {
      entry_i8_t dest_new[NPAGE_ENTRIES];
      size_t destLen_new = sizeof(dest_new);
    
      //exclude source_new[0]
      --sourceLen_new;
    
      destLen_new = ZSTD_decompress(dest_new, destLen_new,
                                    source_new + 1, sourceLen_new);
  
      HARDBUG(ZSTD_isError(destLen_new))
  
      HARDBUG(destLen_new > sizeof(dest_new))

      HARDBUG((destLen_new % sizeof(entry_i8_t)) != 0)

      HARDBUG((destLen_new / sizeof(entry_i8_t)) != nblock_entries)

#ifdef USE_HARDWARE_CRC32

      ui32_t crc32 = 0xFFFFFFFF;
  
      crc32 = _mm_crc32_u8(crc32, source_new[0]);

      for (int ientry = 0; ientry < nblock_entries; ientry++)
      {
        crc32 = _mm_crc32_u8(crc32, dest_new[ientry].entry_white_mate);
        crc32 = _mm_crc32_u8(crc32, dest_new[ientry].entry_black_mate);
      }

#else

#error NOT IMPLEMENTED YET

#endif

      HARDBUG(crc32 != crc32_new)

      for (int ientry = 0; ientry < nblock_entries; ientry++)
      {
        white_stats_new[dest_new[ientry].entry_white_mate + NSTATS / 2]++;
        black_stats_new[dest_new[ientry].entry_black_mate + NSTATS / 2]++;
      }
    }
    else if (source_new[0] == ALL_I16)
    {
      entry_i16_t dest_new[NPAGE_ENTRIES];
      size_t destLen_new = sizeof(dest_new);
    
      //exclude source_new[0]
      --sourceLen_new;
    
      destLen_new = ZSTD_decompress(dest_new, destLen_new,
                                    source_new + 1, sourceLen_new);
  
      HARDBUG(ZSTD_isError(destLen_new))
  
      HARDBUG((destLen_new % sizeof(entry_i16_t)) != 0)

      HARDBUG((destLen_new / sizeof(entry_i16_t)) != nblock_entries)

#ifdef USE_HARDWARE_CRC32

      ui32_t crc32 = 0xFFFFFFFF;
  
      crc32 = _mm_crc32_u8(crc32, source_new[0]);

      for (int ientry = 0; ientry < nblock_entries; ientry++)
      {
        crc32 = _mm_crc32_u16(crc32, dest_new[ientry].entry_white_mate);
        crc32 = _mm_crc32_u16(crc32, dest_new[ientry].entry_black_mate);
      }

#else

#error NOT IMPLEMENTED YET

#endif

      HARDBUG(crc32 != crc32_new)

      for (int ientry = 0; ientry < nblock_entries; ientry++)
      {
        white_stats_new[dest_new[ientry].entry_white_mate + NSTATS / 2]++;
        black_stats_new[dest_new[ientry].entry_black_mate + NSTATS / 2]++;
      }
    }
    else
    {
      PRINTF("source_new[0]=%d\n", source_new[0]);
      FATAL("eh", 1)
    }
  }

  PRINTF("statistics\n");

  for (int istat = 0; istat < NSTATS; istat++)
  {
    //INVALID can be different because of reblock

    if ((istat - NSTATS / 2) == INVALID) continue;

    HARDBUG(white_stats_new[istat] != white_stats_old[istat])

    if (white_stats_new[istat] != 0)
      PRINTF("wtm %d %d\n", istat - NSTATS / 2, white_stats_new[istat]);
  }

  for (int istat = 0; istat < NSTATS; istat++)
  {
    if ((istat - NSTATS / 2) == INVALID) continue;

    HARDBUG(black_stats_new[istat] != black_stats_old[istat])

    if (black_stats_new[istat] != 0)
      PRINTF("btm %d %d\n", istat - NSTATS / 2, black_stats_new[istat]);
  }

  PRINTF("compressed_size_new=%lld\n", compressed_size_new);

  PRINTF("RECOMPRESS_OK\n");
#endif
}

