// 67d7842dbbe25473c3c32b93c0da8047785f30d78e8a024de1b57352245f9689
#ifndef __STREAM_ME_CONCRETE_ACCESSORS_H__
#define __STREAM_ME_CONCRETE_ACCESSORS_H__

// This file needs to be included in code that reads and writes streams in PL

#include "adf/stream/me/structs.h"

#if __AIEARCH__<20
// Internal access functions with stream resource typecast
template <typename T>
static inline int _get_ss_SS_rsrc1(input_stream<T> * ss) {
  int tmp1;
  int tmp2 = me_primitive::stream_read(ss->channel_ss0,0,tmp1);
  *ss->channel_sst0 = tmp1;
  return tmp2;
}

template <typename T>
static inline int _get_ss_SS_rsrc2(input_stream<T> * ss) {
  int tmp1;
  int tmp2 = me_primitive::stream_read(ss->channel_ss1,0,tmp1);
  *ss->channel_sst1 = tmp1;
  return tmp2;
}

template <typename T>
static inline v4int32 _get_wss_WSS_rsrc1(input_stream<T> * ss) {
  int tmp1;
  v4int32 tmp2 = me_primitive::stream_read128(ss->channel_wss0, 0, tmp1);
  *ss->channel_wsst0 = tmp1;
  return tmp2;
}

template <typename T>
static inline v4int32 _get_wss_WSS_rsrc2(input_stream<T> * ss) {
  int tmp1;
  v4int32 tmp2 = me_primitive::stream_read128(ss->channel_wss1, 0, tmp1);
  *ss->channel_wsst1 = tmp1;
  return tmp2;
}

template <typename T>
static inline int _get_ss_SS_rsrc1(input_stream<T> * ss, bool &tlast) {
  int tmp1;
  int tmp2 = me_primitive::stream_read(ss->channel_ss0,0,tmp1);
  *ss->channel_sst0 = tmp1;
  tlast = tmp1;
  return tmp2;
}

template <typename T>
static inline int _get_ss_SS_rsrc2(input_stream<T> * ss, bool &tlast) {
  int tmp1;
  int tmp2 = me_primitive::stream_read(ss->channel_ss1,0,tmp1);
  *ss->channel_sst1 = tmp1;
  tlast = tmp1;
  return tmp2;
}

template <typename T>
static inline v4int32 _get_wss_WSS_rsrc1(input_stream<T> * ss, bool &tlast) {
  int tmp1;
  v4int32 tmp2 = me_primitive::stream_read128(ss->channel_wss0, 0, tmp1);
  *ss->channel_wsst0 = tmp1;
  tlast = tmp1;
  return tmp2;
}

template <typename T>
static inline v4int32 _get_wss_WSS_rsrc2(input_stream<T> * ss, bool &tlast) {
  int tmp1;
  v4int32 tmp2 = me_primitive::stream_read128(ss->channel_wss1, 0, tmp1);
  *ss->channel_wsst1 = tmp1;
  tlast = tmp1;
  return tmp2;
}

template <typename T>
static inline void _put_ms_MS_rsrc1(output_stream<T> * ms, int val, int tlast) {
  me_primitive::stream_write(ms->channel_ms0,val,0,tlast);
}

template <typename T>
static inline void _put_ms_MS_rsrc2(output_stream<T> * ms, int val, int tlast) {
  me_primitive::stream_write(ms->channel_ms1,val,0,tlast);
}

template <typename T>
static inline
void _put_wms_WMS_rsrc1(output_stream<T> * wms, v4int32 val, int tlast) {
  me_primitive::stream_write128(wms->channel_wms0,val,0,tlast);
}

template <typename T>
static inline
void _put_wms_WMS_rsrc2(output_stream<T> * wms, v4int32 val, int tlast) {
  me_primitive::stream_write128(wms->channel_wms1,val,0,tlast);
}

#define READINCR(rspec, str) _get_ss_ ## rspec((str))

#define READINCRW(rspec, str) _get_wss_ ## rspec((str))

#define READINCRT(rspec, str, tlast) _get_ss_ ## rspec((str), tlast)

#define READINCRWT(rspec, str, tlast) _get_wss_ ## rspec((str), tlast)

#define WRITEINCR(rspec, str, val) _put_ms_ ## rspec((str), val, 0)

#define WRITEINCRW(rspec, str, val) _put_wms_ ## rspec((str), val, 0)

#define WRITEINCRT(rspec, str, val, tlast) _put_ms_ ## rspec((str), val, (int)(tlast))

#define WRITEINCRWT(rspec, str, val, tlast) _put_wms_ ## rspec((str), val, (int)(tlast))

#endif // __AIEARCH__<20


#include "adf/stream/me/stream_utils.h"

template <aie_stream_resource_in resource = aie_stream_resource_in::none,
          typename streamTy>
requires(stream_utils::is_valid_type<streamTy>)
static inline streamTy
readincr(input_stream<streamTy> *ss)
{
    using rsrcTy = aie_stream_resource_in_set_t<streamTy, resource>;
    int32 origVal = stream_utils::aie_stream_get_ss<rsrcTy, streamTy>(ss);
    streamTy value;

    if constexpr (stream_utils::is_complex<streamTy>)
    {
        if constexpr (stream_utils::type_bits_value<streamTy> == 32)
        {
            value.real = origVal & 0xFFFF;
            value.imag = (origVal>>16) & 0xFFFF;
        }
        if constexpr (stream_utils::type_bits_value<streamTy> == 64)
        {
            int32 imagV = stream_utils::aie_stream_get_ss<rsrcTy, streamTy>(ss);
            if constexpr (stream_utils::is_cfloat<streamTy>)
            {
                value.real = as_float(origVal);
                value.imag = as_float(imagV);
            }
            else
            {
                value.real = origVal;
                value.imag = imagV;
            }
        }
    }
    else
    {
        if constexpr (stream_utils::type_bits_value<streamTy> == 4)
            value = (origVal & 0x0000000F);
        else if constexpr (stream_utils::type_bits_value<streamTy> == 8)
            value = (origVal & 0x000000FF);
        #if __AIEARCH__>=20
        //For bfloat16; needs to be before type_bits_value<streamTy> == 16
        else if constexpr (stream_utils::is_bfloat16<streamTy>)
        {
            int16 temp = (origVal & 0x0000FFFF);
            value = *((bfloat16*)(&temp)) ;
        }
        #endif // #if __AIEARCH__>=20
        else if constexpr (stream_utils::type_bits_value<streamTy> == 16)
            value = (origVal & 0x0000FFFF);
        else if constexpr (stream_utils::type_bits_value<streamTy> == 64)
        {
            streamTy lowerVal = (streamTy)origVal;
            streamTy higherVal =
                (streamTy)stream_utils::aie_stream_get_ss<rsrcTy, streamTy>(ss);
            value = ((higherVal << 32) | (lowerVal & 0xFFFFFFFF));
        }
        else if constexpr (stream_utils::is_float<streamTy>)
            value = as_float(origVal);
        else
            value = origVal;
    }
    return value;
}

template <aie_stream_resource_in resource = aie_stream_resource_in::none,
          typename streamTy>
requires(stream_utils::is_valid_type<streamTy>)
static inline streamTy
readincr(input_stream<streamTy> *ss, bool &tlast)
{
    using rsrcTy = aie_stream_resource_in_set_t<streamTy, resource>;
    int32 origVal = stream_utils::aie_stream_get_ss<rsrcTy, streamTy>(ss, tlast);
    streamTy value;

    if constexpr (stream_utils::is_complex<streamTy>)
    {
        if constexpr (stream_utils::type_bits_value<streamTy> == 32)
        {
            value.real = origVal & 0xFFFF;
            value.imag = (origVal>>16) & 0xFFFF;
        }
        if constexpr (stream_utils::type_bits_value<streamTy> == 64)
        {
            int32 imagV = stream_utils::aie_stream_get_ss<rsrcTy, streamTy>(ss, tlast);
            if constexpr (stream_utils::is_cfloat<streamTy>)
            {
                value.real = as_float(origVal);
                value.imag = as_float(imagV);
            }
            else
            {
                value.real = origVal;
                value.imag = imagV;
            }
        }
    }
    else
    {
        if constexpr (stream_utils::type_bits_value<streamTy> == 4)
            value = (origVal & 0x0000000F);
        else if constexpr (stream_utils::type_bits_value<streamTy> == 8)
            value = (origVal & 0x000000FF);
        #if __AIEARCH__>=20
        //For bfloat16; needs to be before type_bits_value<streamTy> == 16
        else if constexpr (stream_utils::is_bfloat16<streamTy>)
        {
            int16 temp = (origVal & 0x0000FFFF);
            value = *((bfloat16*)(&temp)) ;
        }
        #endif // #if __AIEARCH__>=20
        else if constexpr (stream_utils::type_bits_value<streamTy> == 16)
            value = (origVal & 0x0000FFFF);
        else if constexpr (stream_utils::type_bits_value<streamTy> == 64)
        {
            streamTy lowerVal =  (streamTy)origVal;
            streamTy higherVal =
                (streamTy)stream_utils::aie_stream_get_ss<rsrcTy, streamTy>(ss, tlast);
            value = ((higherVal << 32) | (lowerVal & 0xFFFFFFFF));
        }
        else if constexpr (stream_utils::is_float<streamTy>)
            value = as_float(origVal);
        else
            value = origVal;
    }
    return value;
}

template <aie_stream_resource_out resource = aie_stream_resource_out::none,
          typename streamTy>
requires(stream_utils::is_valid_type<streamTy>)
static inline void
writeincr(output_stream<streamTy> *str, std::type_identity_t<streamTy> value, bool tlast = false)
{
    int32 value32;
    using rsrcTy = aie_stream_resource_out_set_t<streamTy, resource>;

    if constexpr (stream_utils::is_complex<streamTy>)
    {
        if constexpr (stream_utils::type_bits_value<streamTy> == 32)
            value32 = ((value.imag << 16) | (value.real & 0xffff));
        if constexpr (stream_utils::type_bits_value<streamTy> == 64) {
           if constexpr (stream_utils::is_cfloat<streamTy>)
           {
               // push the lower 32bits
               int32 realValue = as_int32(value.real);
               stream_utils::aie_stream_put_ms<rsrcTy, streamTy>(str, realValue, 0);
               value32 = as_int32(value.imag);
           }
           else
           {
               // push the lower 32bits
               stream_utils::aie_stream_put_ms<rsrcTy, streamTy>(str, value.real, 0);
               value32 = value.imag;
           }
        }
    }
    else
    {
        if constexpr (stream_utils::type_bits_value<streamTy> == 4)
            value32 = (int32)(value & 0x0000000F);
        else if constexpr (stream_utils::type_bits_value<streamTy> == 8)
            value32 = (int32)(value & 0x000000FF);
        #if __AIEARCH__>=20
        //For bfloat16; needs to be before type_bits_value<streamTy> == 16
        else if constexpr (stream_utils::is_bfloat16<streamTy>)
            value32 = (int32)(*((int32*)(&value)) & 0x0000FFFF);
        #endif // #if __AIEARCH__>=20
        else if constexpr (stream_utils::type_bits_value<streamTy> == 16)
            value32 = (int32)(value & 0x0000FFFF);
        else if constexpr (stream_utils::type_bits_value<streamTy> == 64)
        {
            int32 lowerVal32 = (int32)(value & (0x00000000FFFFFFFF));
            value32 = (int32)((value & (0xFFFFFFFF00000000)) >> 32);
            // push the lower 32bits
            stream_utils::aie_stream_put_ms<rsrcTy, streamTy>(str, lowerVal32, 0);
        }
        else if constexpr (stream_utils::is_float<streamTy>)
            value32 = as_int32(value);
        else
            value32 = value;
    }
    stream_utils::aie_stream_put_ms<rsrcTy, streamTy>(str, value32, tlast);
}

#if __AIEARCH__<20

template <aie_stream_resource_in resource = aie_stream_resource_in::none>
static inline v8int16
readincr_v8(input_stream<int16> *ss)
{
    using rsrcTy = aie_stream_resource_in_set_t<int16, resource>;
    return stream_utils::aie_stream_get_wss<rsrcTy>(ss);
}

template <aie_stream_resource_in resource = aie_stream_resource_in::none>
static inline v8int16
readincr_v8(input_stream<int16> *ss, bool &tlast)
{
    using rsrcTy = aie_stream_resource_in_set_t<int16, resource>;
    return stream_utils::aie_stream_get_wss<rsrcTy>(ss, tlast);
}

#define VECTOR_READ(ty, ln)                                                    \
  template <aie_stream_resource_in Resource = aie_stream_resource_in::none>    \
  static inline v ## ln ## ty readincr_v ## ln(input_stream_ ## ty * str) {    \
    using T = aie_stream_resource_in_set_t<ty, Resource>;                      \
    return as_v ## ln ## ty(readincr_v8<aie_stream_resource_in_get_v<T>>(      \
                                        (input_stream<int16> *)str));          \
  }                                                                            \
  template <aie_stream_resource_in Resource = aie_stream_resource_in::none>    \
  static inline v ## ln ## ty readincr_v ## ln(input_stream_ ## ty * str,      \
                                               bool &tlast) {                  \
    using T = aie_stream_resource_in_set_t<ty, Resource>;                      \
    return as_v ## ln ## ty(readincr_v8<aie_stream_resource_in_get_v<T>>(      \
                                       (input_stream<int16> *)str,tlast));     \
  }
VECTOR_READ(int8, 16);
VECTOR_READ(uint8, 16);
VECTOR_READ(int32, 4);
VECTOR_READ(float, 4);
VECTOR_READ(cint16, 4);
VECTOR_READ(cint32, 2);
VECTOR_READ(cfloat, 2);

template <aie_stream_resource_out resource = aie_stream_resource_out::none>
static inline void
writeincr_v8(output_stream<int16> *ss,  v8int16 value, bool tlast = false)
{
    using rsrcTy = aie_stream_resource_out_set_t<int16, resource>;
    return stream_utils::aie_stream_put_wms<rsrcTy>(ss, value, tlast);
}

template <aie_stream_resource_out resource = aie_stream_resource_out::none>
static inline void
writeincr(output_stream<int16> *ss, v8int16 value, bool tlast = false)
{
    using rsrcTy = aie_stream_resource_out_set_t<int16, resource>;
    return stream_utils::aie_stream_put_wms<rsrcTy>(ss, value, tlast);
}

#define VECTOR_WRITE(ty, ln)                                                   \
  template <aie_stream_resource_out Resource = aie_stream_resource_out::none>  \
  static inline void writeincr_v ## ln(output_stream_ ## ty * str,             \
                                      v ## ln ## ty value, bool tlast=false) { \
    using T = aie_stream_resource_out_set_t<ty, Resource>;                     \
    writeincr_v8<aie_stream_resource_out_get_v<T>>((output_stream<int16> *)str,\
                                                   as_v8int16(value), tlast);  \
  }                                                                            \
  template <aie_stream_resource_out Resource = aie_stream_resource_out::none>  \
  static inline void writeincr(output_stream_ ## ty * str, v ## ln ## ty value,\
                                                     bool tlast=false) {       \
    using T = aie_stream_resource_out_set_t<ty, Resource>;                     \
    writeincr<aie_stream_resource_out_get_v<T>>((output_stream<int16> *)str,   \
                                                  as_v8int16(value), tlast);   \
  }                                                                            \

VECTOR_WRITE(int8, 16);
VECTOR_WRITE(uint8, 16);
VECTOR_WRITE(int32, 4);
VECTOR_WRITE(float, 4);
VECTOR_WRITE(cint16, 4);
VECTOR_WRITE(cint32, 2);
VECTOR_WRITE(cfloat, 2);

static inline v8acc48 readincr_v8(input_stream<acc48> * str) {
  return get_scd();
}

static inline v4cacc48 readincr_v4(input_stream<cacc48> * str) {
  return getc_scd();
}

static inline v4acc80 readincr_v4(input_stream<acc80> * str) {
  return get_scd_v4acc80();
}

static inline v2cacc80 readincr_v2(input_stream<cacc80> * str) {
  return get_scd_v2cacc80();
}

static inline v32int8 readincr_v32int8(input_stream<acc48> * str) {
  return get_scd_v32int8();
}

static inline v32uint8 readincr_v32uint8(input_stream<acc48> * str) {
  return get_scd_v32uint8();
}

static inline v16int16 readincr_v16int16(input_stream<acc48> * str) {
  return get_scd_v16int16();
}

static inline v8cint16 readincr_v8cint16(input_stream<acc48> * str) {
  return get_scd_v8cint16();
}

static inline v8int32 readincr_v8int32(input_stream<acc48> * str) {
  return get_scd_v8int32();
}

static inline v4cint32 readincr_v4cint32(input_stream<cacc48> * str) {
  return get_scd_v4cint32();
}

static inline v4cfloat readincr_v4cfloat(input_stream<cacc48> * str){
  return get_scd_v4cfloat();
}

static inline v8float readincr_v8float(input_stream<acc48> * str){
  return get_scd_v8float();
}

static inline v4cfloat readincr_v4(input_stream<caccfloat> * str){
  return get_scd_v4cfloat();
}

static inline v8float readincr_v8(input_stream<accfloat> * str){
  return get_scd_v8float();
}

template <typename streamTy, typename vectorTy>
requires(
    (std::is_same_v<streamTy, acc80> && 
    (std::is_same_v<vectorTy, v4acc80> || std::is_convertible_v<vectorTy, v4acc80>)) 
    ||
    (std::is_same_v<streamTy,cacc48> && 
    (std::is_same_v<vectorTy, v4cacc48> || std::is_convertible_v<vectorTy, v4cacc48>)) 
    ||
    (std::is_same_v<streamTy, caccfloat> && 
    (std::is_same_v<vectorTy, v4cfloat> || std::is_convertible_v<vectorTy, v4cfloat>)))
static inline void
writeincr_v4(output_stream<streamTy> *ss,  vectorTy value)
{
    return stream_utils::aie_stream_put_mcd<streamTy, vectorTy>(ss, value);
}

template <typename streamTy, typename vectorTy>
requires(
    (std::is_same_v<streamTy, accfloat> && 
    (std::is_same_v<vectorTy, v8float> || std::is_convertible_v<vectorTy, v8float>)) 
    ||
    (std::is_same_v<streamTy, acc48> && 
    (std::is_same_v<vectorTy, v8acc48> || std::is_convertible_v<vectorTy, v8acc48>)))
static inline void
writeincr_v8(output_stream<streamTy> *ss,  vectorTy value)
{
    return stream_utils::aie_stream_put_mcd<streamTy, vectorTy>(ss, value);
}

template <typename streamTy, typename vectorTy>
requires(
    (std::is_same_v<streamTy, cacc80> && 
    (std::is_same_v<vectorTy, v2cacc80> || std::is_convertible_v<vectorTy, v2cacc80>)))
static inline void
writeincr_v2(output_stream<streamTy> *ss,  vectorTy value)
{
    return stream_utils::aie_stream_put_mcd<streamTy, vectorTy>(ss, value);
}

static inline void
writeincr_v32int8(output_stream<acc48>* str, v32int8 value) {
  put_mcd(value);
}

static inline void
writeincr_v32uint8(output_stream<acc48>* str, v32uint8 value) {
  put_mcd(value);
}

static inline void
writeincr_v16int16(output_stream<acc48>* str, v16int16 value) {
  put_mcd(value);
}

static inline void
writeincr_v8cint16(output_stream<acc48>* str, v8cint16 value) {
  put_mcd(value);
}

static inline void
writeincr_v8int32(output_stream<acc48>* str, v8int32 value) {
  put_mcd(value);
}

static inline void
writeincr_v4cint32(output_stream<cacc48>* str, v4cint32 value) {
  put_mcd(value);
}

static inline void
writeincr_v4cfloat(output_stream<cacc48>* str, v4cfloat value) {
  put_mcd(value);
}

static inline void
writeincr_v8float(output_stream<acc48>* str, v8float value) {
  put_mcd(value);
}

template <typename streamTy, typename vectorTy>
requires(stream_utils::is_valid_vector_accum_type<streamTy, vectorTy>::value)
static inline void
writeincr(output_stream<streamTy> *ss,  vectorTy value)
{
    return stream_utils::aie_stream_put_mcd<streamTy, vectorTy>(ss, value);
}

#else // #if __AIEARCH__<20

// TLAST not yet support with get_ss_v<lane><type> instrinsics in r0p31
#define VECTOR_READ(ty, ln)                                                    \
  template <aie_stream_resource_in Resource = aie_stream_resource_in::none>    \
  static inline v ## ln ## ty readincr_v ## ln(input_stream_ ## ty * str) {    \
    return get_ss_v ## ln ## ty();                                             \
  }

#if __AIEARCH__==20

VECTOR_READ(int4, 32);
VECTOR_READ(int4, 64);
VECTOR_READ(int4, 128);
VECTOR_READ(int4, 256);
VECTOR_READ(uint4, 32);
VECTOR_READ(uint4, 64);
VECTOR_READ(uint4, 128);
VECTOR_READ(uint4, 256);
VECTOR_READ(int8, 16);
VECTOR_READ(int8, 32);
VECTOR_READ(int8, 64);
VECTOR_READ(int8, 128);
VECTOR_READ(uint8, 16);
VECTOR_READ(uint8, 32);
VECTOR_READ(uint8, 64);
VECTOR_READ(uint8, 128);
VECTOR_READ(int16,  8);
VECTOR_READ(int16, 16);
VECTOR_READ(int16, 32);
VECTOR_READ(int16, 64);
VECTOR_READ(uint16,  8);
VECTOR_READ(uint16, 16);
VECTOR_READ(uint16, 32);
VECTOR_READ(uint16, 64);
VECTOR_READ(bfloat16, 8);
VECTOR_READ(bfloat16, 16);
VECTOR_READ(bfloat16, 32);
VECTOR_READ(bfloat16, 64);
VECTOR_READ(int32, 4);
VECTOR_READ(int32, 8);
VECTOR_READ(int32, 16);
VECTOR_READ(int32, 32);
VECTOR_READ(uint32, 4);
VECTOR_READ(uint32, 8);
VECTOR_READ(uint32, 16);
VECTOR_READ(uint32, 32);
VECTOR_READ(float, 4);
VECTOR_READ(float, 8);
VECTOR_READ(float, 16);
VECTOR_READ(float, 32);
VECTOR_READ(cint16, 4);
VECTOR_READ(cint16, 8);
VECTOR_READ(cint16, 16);
VECTOR_READ(cint16, 32);
VECTOR_READ(cint32, 2);
VECTOR_READ(cint32, 4);
VECTOR_READ(cint32, 8);
VECTOR_READ(cint32, 16);

#else

VECTOR_READ(int4, 128);
VECTOR_READ(int4, 256);
VECTOR_READ(uint4, 128);
VECTOR_READ(uint4, 256);
VECTOR_READ(int8, 64);
VECTOR_READ(int8, 128);
VECTOR_READ(uint8, 64);
VECTOR_READ(uint8, 128);
VECTOR_READ(int16, 32);
VECTOR_READ(int16, 64);
VECTOR_READ(uint16, 32);
VECTOR_READ(uint16, 64);
VECTOR_READ(bfloat16, 32);
VECTOR_READ(int32, 16);
VECTOR_READ(int32, 32);
VECTOR_READ(uint32, 16);
VECTOR_READ(uint32, 32);
VECTOR_READ(cint16, 16);
VECTOR_READ(cint16, 32);
VECTOR_READ(cint32, 8);
VECTOR_READ(cint32, 16);

#endif

// put_ms(<type> a, int en, int tlast)
// The enable input is unused. This signature was chosen for put_ms
// intrinsic so it would look similar to put_mcd. But enable is unused
// since it is not required for this case.

#define VECTOR_WRITE(ty, ln)                                                   \
  template <aie_stream_resource_out Resource = aie_stream_resource_out::none>  \
  static inline void writeincr_v ## ln(output_stream_ ## ty * str,             \
                                      v ## ln ## ty value, bool tlast=false) { \
    put_ms(value, 0, tlast);                                                   \
  }                                                                            \
  template <aie_stream_resource_out Resource = aie_stream_resource_out::none>  \
  static inline void writeincr(output_stream_ ## ty * str, v ## ln ## ty value,\
                                                     bool tlast=false) {       \
    put_ms(value, 0, tlast);                                                   \
  }

#if __AIEARCH__==20

VECTOR_WRITE(int4, 32);
VECTOR_WRITE(int4, 64);
VECTOR_WRITE(int4, 128);
VECTOR_WRITE(int4, 256);
VECTOR_WRITE(uint4, 32);
VECTOR_WRITE(uint4, 64);
VECTOR_WRITE(uint4, 128);
VECTOR_WRITE(uint4, 256);
VECTOR_WRITE(int8, 16);
VECTOR_WRITE(int8, 32);
VECTOR_WRITE(int8, 64);
VECTOR_WRITE(int8, 128);
VECTOR_WRITE(uint8, 16);
VECTOR_WRITE(uint8, 32);
VECTOR_WRITE(uint8, 64);
VECTOR_WRITE(uint8, 128);
VECTOR_WRITE(int16,  8);
VECTOR_WRITE(int16, 16);
VECTOR_WRITE(int16, 32);
VECTOR_WRITE(int16, 64);
VECTOR_WRITE(uint16,  8);
VECTOR_WRITE(uint16, 16);
VECTOR_WRITE(uint16, 32);
VECTOR_WRITE(uint16, 64);
VECTOR_WRITE(bfloat16, 8);
VECTOR_WRITE(bfloat16, 16);
VECTOR_WRITE(bfloat16, 32);
VECTOR_WRITE(bfloat16, 64);
VECTOR_WRITE(int32, 4);
VECTOR_WRITE(int32, 8);
VECTOR_WRITE(int32, 16);
VECTOR_WRITE(int32, 32);
VECTOR_WRITE(uint32, 4);
VECTOR_WRITE(uint32, 8);
VECTOR_WRITE(uint32, 16);
VECTOR_WRITE(uint32, 32);
VECTOR_WRITE(float, 4);
VECTOR_WRITE(float, 8);
VECTOR_WRITE(float, 16);
VECTOR_WRITE(float, 32);
VECTOR_WRITE(cint16, 4);
VECTOR_WRITE(cint16, 8);
VECTOR_WRITE(cint16, 16);
VECTOR_WRITE(cint16, 32);
VECTOR_WRITE(cint32, 2);
VECTOR_WRITE(cint32, 4);
VECTOR_WRITE(cint32, 8);
VECTOR_WRITE(cint32, 16);

#else

VECTOR_WRITE(int4, 128);
VECTOR_WRITE(int4, 256);
VECTOR_WRITE(uint4, 128);
VECTOR_WRITE(uint4, 256);
VECTOR_WRITE(int8, 64);
VECTOR_WRITE(int8, 128);
VECTOR_WRITE(uint8, 64);
VECTOR_WRITE(uint8, 128);
VECTOR_WRITE(int16, 32);
VECTOR_WRITE(int16, 64);
VECTOR_WRITE(uint16, 32);
VECTOR_WRITE(uint16, 64);
VECTOR_WRITE(bfloat16, 32);
VECTOR_WRITE(int32, 16);
VECTOR_WRITE(int32, 32);
VECTOR_WRITE(uint32, 16);
VECTOR_WRITE(uint32, 32);
VECTOR_WRITE(cint16, 16);
VECTOR_WRITE(cint16, 32);
VECTOR_WRITE(cint32, 8);
VECTOR_WRITE(cint32, 16);

#endif

static inline v16acc32 readincr_v16(input_stream<acc32>* str) {
  return get_scd_v16acc32();
}

static inline v8acc64 readincr_v8(input_stream<acc64>* str) {
  return get_scd_v8acc64();
}

static inline v16accfloat readincr_v16(input_stream<accfloat>* str) {
  return get_scd_v16accfloat();
}

static inline v4cacc64 readincr_v4(input_stream<cacc64>* str) {
  return get_scd_v4cacc64();
}

static inline void writeincr_v16(output_stream<acc32>* str, v16acc32 value) {
  put_mcd(value);
}

static inline void writeincr_v8(output_stream<acc64>* str, v8acc64 value) {
  put_mcd(value);
}

static inline void
writeincr_v16(output_stream<accfloat>* str, v16accfloat value) {
  put_mcd(value);
}

static inline void writeincr_v4(output_stream<cacc64>* str, v4cacc64 value) {
  put_mcd(value);
}

template <typename streamTy, typename vectorTy>
requires(stream_utils::is_valid_vector_accum_type<streamTy, vectorTy>::value)
static inline void
writeincr(output_stream<streamTy> *ss,  vectorTy value)
{
    return stream_utils::aie_stream_put_mcd<streamTy, vectorTy>(ss, value);
}
#endif // #if __AIEARCH__<20


// Pktstream
#if __AIEARCH__<20
template <aie_stream_resource_in Resource = aie_stream_resource_in::none>
static inline int32 readincr(input_pktstream *str) {
  return get_ss(str->channel);
}

template <aie_stream_resource_in Resource = aie_stream_resource_in::none>
static inline int32 readincr(input_pktstream *str, bool &tlast) {
  int32 val = get_ss(str->channel);
  tlast = (str->channel==0) ? get_ss0_tlast() : get_ss1_tlast();
  return val;
}

template <aie_stream_resource_out Resource = aie_stream_resource_out::none,
          typename streamTy>
requires(std::is_same_v<streamTy, int32> || std::is_same_v<streamTy, uint32> ||
         std::is_same_v<streamTy, float>)
static inline void
writeincr(output_pktstream *str, streamTy value, bool tlast=false) {
  put_ms(str->channel, value, tlast);
}

static inline uint32_t generateHeader(unsigned int pcktType, unsigned int ID)
{
  return packet_header(ID, pcktType);
}

static inline uint32_t generateCtrlHeader(unsigned int returnID, unsigned int op,
                                        unsigned int numWords, unsigned int addr)
{
  return ctrl_packet_header(addr, numWords, op, returnID);
}

static inline void writeHeader(output_pktstream *str, unsigned int pcktType,
                                                            unsigned int ID)
{
    put_ms_packet_header(str->channel, ID, pcktType);
}

static inline void writeHeader(output_pktstream *str, unsigned int pcktType,
                                                unsigned int ID, bool tlast)
{
    put_ms_packet_header(str->channel, tlast, ID, pcktType);
}
static inline void writeCtrlHeader(output_pktstream *str, unsigned int returnID,
                      unsigned int op, unsigned int numWords, unsigned int addr)
{
    put_ms_ctrl_packet_header(str->channel, addr, numWords, op, returnID);
}

static inline void writeCtrlHeader(output_pktstream *str, unsigned int returnID,
          unsigned int op, unsigned int numWords, unsigned int addr, bool tlast)
{
    put_ms_ctrl_packet_header(str->channel, tlast, addr, numWords, op, returnID);
}

#else // #if __AIEARCH__<20
template <aie_stream_resource_in Resource = aie_stream_resource_in::none>
static inline int32 readincr(input_pktstream *str) {
  return get_ss();
}

template <aie_stream_resource_in Resource = aie_stream_resource_in::none>
static inline int32 readincr(input_pktstream *str, bool &tlast) {
  return get_ss(tlast);
}

template <aie_stream_resource_out Resource = aie_stream_resource_out::none>
static inline void writeincr(output_pktstream *str, int32 value) {
  put_ms(value);
}

template <aie_stream_resource_out Resource = aie_stream_resource_out::none>
static inline void writeincr(output_pktstream *str, int32 value, bool tlast) {
  put_ms(value, tlast);
}

static inline void writeHeader(output_pktstream *str, unsigned int pcktType,
                                                            unsigned int ID)
{
    put_ms_packet_header(ID, pcktType);
}

static inline void writeHeader(output_pktstream *str, unsigned int pcktType,
                                                unsigned int ID, bool tlast)
{
    put_ms_packet_header(tlast, ID, pcktType);
}

static inline void writeCtrlHeader(output_pktstream *str, unsigned int returnID,
                      unsigned int op, unsigned int numWords, unsigned int addr)
{
    put_ms_ctrl_packet_header(addr, numWords, op, returnID);
}

static inline void writeCtrlHeader(output_pktstream *str, unsigned int returnID,
          unsigned int op, unsigned int numWords, unsigned int addr, bool tlast)
{
    put_ms_ctrl_packet_header(tlast, addr, numWords, op, returnID);
}

#endif // #if __AIEARCH__<20

static inline int32 getPacketid(input_pktstream * in, int index) {
  return in->pktIds[index];
}

static inline int32 getPacketid(output_pktstream * out, int index) {
  return out->pktIds[index];
}

#endif // __STREAM_ME_CONCRETE_ACCESSORS_H__
