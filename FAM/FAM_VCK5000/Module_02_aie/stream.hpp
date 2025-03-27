// 67d7842dbbe25473c3c32b93c0da8047785f30d78e8a024de1b57352245f9689
/*  (c) Copyright 2020 - 2022 Xilinx, Inc. All rights reserved.

    This file contains confidential and proprietary information
    of Xilinx, Inc. and is protected under U.S. and
    international copyright and other intellectual property
    laws.

    DISCLAIMER
    This disclaimer is not a license and does not grant any
    rights to the materials distributed herewith. Except as
    otherwise provided in a valid license issued to you by
    Xilinx, and to the maximum extent permitted by applicable
    law: (1) THESE MATERIALS ARE MADE AVAILABLE "AS IS" AND
    WITH ALL FAULTS, AND XILINX HEREBY DISCLAIMS ALL WARRANTIES
    AND CONDITIONS, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
    BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, NON-
    INFRINGEMENT, OR FITNESS FOR ANY PARTICULAR PURPOSE; and
    (2) Xilinx shall not be liable (whether in contract or tort,
    including negligence, or under any other theory of
    liability) for any loss or damage of any kind or nature
    related to, arising under or in connection with these
    materials, including for any direct, or any indirect,
    special, incidental, or consequential loss or damage
    (including loss of data, profits, goodwill, or any type of
    loss or damage suffered as a result of any action brought
    by a third party) even if such damage or loss was
    reasonably foreseeable or Xilinx had been advised of the
    possibility of the same.

    CRITICAL APPLICATIONS
    Xilinx products are not designed or intended to be fail-
    safe, or for use in any application requiring fail-safe
    performance, such as life-support or safety devices or
    systems, Class III medical devices, nuclear facilities,
    applications related to the deployment of airbags, or any
    other applications that could lead to death, personal
    injury, or severe property or environmental damage
    (individually and collectively, "Critical
    Applications"). Customer assumes the sole risk and
    liability of any use of Xilinx products in Critical
    Applications, subject only to applicable laws and
    regulations governing limitations on product liability.

    THIS COPYRIGHT NOTICE AND DISCLAIMER MUST BE RETAINED AS
    PART OF THIS FILE AT ALL TIMES.                       */


#pragma once

#ifndef __AIE_API_AIE_ADF_STREAM_HPP__
#define __AIE_API_AIE_ADF_STREAM_HPP__

#include <adf.h>
#include "../aie.hpp"

namespace aie::detail::adf {

template <unsigned N, typename T, aie_stream_resource_in Resource>
struct stream_in_helper;

template <unsigned N, typename T, aie_stream_resource_out Resource>
struct stream_out_helper;

template <unsigned N, typename T, aie_stream_resource_in Resource>
struct stream_in_helper
{
    static constexpr unsigned stream_width = 128;

    using type = aie::vector<T, N>;

    static_assert(type::bits() >= stream_width);
    static_assert(type::bits() % stream_width == 0);

    static constexpr unsigned num_ops = type::bits() / stream_width;
    static constexpr unsigned elems_per_op = N / num_ops;

#if __AIE_ARCH__ == 10 || __AIE_ARCH__ == 20
    static type readincr(input_stream<T> *w)
    {
        type ret;

        #pragma unroll
        for (unsigned i = 0; i < num_ops; ++i) {
            const aie::vector<int16, 8> tmp = ::readincr_v8<Resource>((input_stream_int16 *)w);
            ret.insert(i, aie::vector_cast<T>(tmp));
        }

        return ret;
    }
#endif

#if __AIE_ARCH__ == 10
    static type readincr(input_stream<T> *w, bool &tlast)
    {
        type ret;

        #pragma unroll
        for (unsigned i = 0; i < num_ops; ++i) {
            aie::vector<int16, 8> tmp;

            if (i < num_ops - 1)
                tmp = ::readincr_v8<Resource>((input_stream_int16 *)w);
            else
                tmp = ::readincr_v8<Resource>((input_stream_int16 *)w, tlast);

            ret.insert(i, aie::vector_cast<T>(tmp));
        }

        return ret;
    }
#endif
};

template <unsigned N, typename T, aie_stream_resource_out Resource>
struct stream_out_helper
{
    static constexpr unsigned stream_width = 128;

    using type = aie::vector<T, N>;

    static_assert(type::bits() >= stream_width);
    static_assert(type::bits() % stream_width == 0);

    static constexpr unsigned num_ops = type::bits() / stream_width;
    static constexpr unsigned elems_per_op = N / num_ops;

    static void writeincr(output_stream<T> *w, type value)
    {
        #pragma unroll
        for (unsigned i = 0; i < num_ops; ++i)
            ::writeincr<Resource>(w, value.template extract<elems_per_op>(i));
    }

    static void writeincr(output_stream<T> *w, type value, bool tlast)
    {
        #pragma unroll
        for (unsigned i = 0; i < num_ops; ++i) {
            if (i < num_ops - 1)
                ::writeincr<Resource>(w, value.template extract<elems_per_op>(i));
            else
                ::writeincr<Resource>(w, value.template extract<elems_per_op>(i), tlast);
        }
    }
};

template <typename AccumTag, unsigned N>
struct cascade_stream_helper
{
    using native_tag = aie::detail::accum_native_type_t<AccumTag>;
    using       type = aie::accum<native_tag, N>;

#if __AIE_ARCH__ == 10
    static constexpr unsigned compute_num_ops()
    {
        if      constexpr (type::accum_bits() % 48 == 0) // acc48/cacc48
            return type::bits() / 384;
        else if constexpr (type::accum_bits() % 80 == 0) // acc80/cacc80
            return type::bits() / 320;
    }

    static constexpr unsigned             num_ops = compute_num_ops();
    static constexpr unsigned        elems_per_op = N / num_ops;
    static constexpr unsigned native_elems_per_op = elems_per_op;
#elif __AIE_ARCH__ == 20
    static constexpr unsigned compute_native_elems_per_op()
    {
        if      constexpr (type::value_bits() == 32)
            return 16;
        else if constexpr (type::value_bits() == 64)
            return 8;
        else if constexpr (type::value_bits() == 128)
            return 4;
    }

    static constexpr unsigned cascade_width = 512;
    static constexpr unsigned             num_ops = cascade_width > type::bits()? 1 : type::bits() / cascade_width;
    static constexpr unsigned        elems_per_op = N / num_ops;
    static constexpr unsigned native_elems_per_op = compute_native_elems_per_op();
#endif

    static void writeincr(output_stream<AccumTag> *_w, type value)
    {
        output_stream<native_tag> *w = (output_stream<native_tag> *) _w;

        if constexpr (native_elems_per_op > elems_per_op) {
            ::writeincr(w, value.template grow<native_elems_per_op>().data);
        }
        else {
            #pragma unroll
            for (unsigned i = 0; i < num_ops; ++i)
                ::writeincr(w, value.template extract<elems_per_op>(i).data);
        }
    }

    static type readincr(input_stream<AccumTag> *_w)
    {
        input_stream<native_tag> *w = (input_stream<native_tag> *) _w;
        type ret;

        #pragma unroll
        for (unsigned i = 0; i < num_ops; ++i) {
#if __AIE_ARCH__ == 10
            if      constexpr (elems_per_op == 2)
                ret.template insert<2>(i, readincr_v2(w));
            else if constexpr (elems_per_op == 4)
                ret.template insert<4>(i, readincr_v4(w));
            else if constexpr (elems_per_op == 8)
                ret.template insert<8>(i, readincr_v8(w));
#elif __AIE_ARCH__ == 20
            if      constexpr (native_elems_per_op == 4) {
                const aie::accum<native_tag, 4> tmp = readincr_v4(w);

                if constexpr (native_elems_per_op > elems_per_op)
                    ret.template insert<N>(i, tmp.template extract<N>(0));
                else
                    ret.template insert<4>(i, tmp);
            }
            else if      constexpr (native_elems_per_op == 8) {
                const aie::accum<native_tag, 8> tmp = readincr_v8(w);

                if constexpr (native_elems_per_op > elems_per_op)
                    ret.template insert<N>(i, tmp.template extract<N>(0));
                else
                    ret.template insert<8>(i, tmp);
            }
            else if constexpr (native_elems_per_op == 16) {
                const aie::accum<native_tag, 16> tmp = readincr_v16(w);

                if constexpr (native_elems_per_op > elems_per_op)
                    ret.template insert<N>(i, tmp.template extract<N>(0));
                else
                    ret.template insert<16>(i, tmp);
            }
#endif
        }

        return ret;
    }
};

#if __AIE_ARCH__ == 10
template <unsigned N>
struct cascade_stream_helper<accfloat, N>
{
    using AccumTag = accfloat;
    using     type = aie::accum<AccumTag, N>;

    static constexpr unsigned       num_ops = (type::bits() < 256)? 1 : type::bits() / 256;
    static constexpr unsigned  elems_per_op = N / num_ops;

    static void writeincr(output_stream<AccumTag> *_w, type value)
    {
        if constexpr (type::bits() < 256) {
            cascade_stream_helper<AccumTag, N * 2>::writeincr(_w, value.template grow<N * 2>());
        }
        else {
            output_stream<acc48> *w = (output_stream<acc48> *) _w;

            #pragma unroll
            for (unsigned i = 0; i < num_ops; ++i)
                ::writeincr_v8float(w, value.template extract<elems_per_op>(i).data);
        }
    }

    static type readincr(input_stream<AccumTag> *_w)
    {
        if constexpr (type::bits() < 256) {
            return cascade_stream_helper<AccumTag, N * 2>::readincr(_w).template extract<N>(0);
        }
        else {
            input_stream<acc48> *w = (input_stream<acc48> *) _w;
            type ret;

            #pragma unroll
            for (unsigned i = 0; i < num_ops; ++i)
                ret.template insert<8>(i, ::readincr_v8float(w));

            return ret;
        }
    }
};

template <unsigned N>
struct cascade_stream_helper<caccfloat, N>
{
    using AccumTag = caccfloat;
    using     type = aie::accum<AccumTag, N>;

    static constexpr unsigned       num_ops = (type::bits() < 256)? 1 : type::bits() / 256;
    static constexpr unsigned  elems_per_op = N / num_ops;

    static void writeincr(output_stream<AccumTag> *_w, type value)
    {
        if constexpr (type::bits() < 256) {
            cascade_stream_helper<AccumTag, N * 2>::writeincr(_w, value.template grow<N * 2>());
        }
        else {
            output_stream<cacc48> *w = (output_stream<cacc48> *) _w;

            #pragma unroll
            for (unsigned i = 0; i < num_ops; ++i)
                ::writeincr_v4cfloat(w, value.template extract<elems_per_op>(i).data);
        }
    }

    static type readincr(input_stream<AccumTag> *_w)
    {
        if constexpr (type::bits() < 256) {
            return cascade_stream_helper<AccumTag, N * 2>::readincr(_w).template extract<N>(0);
        }
        else {
            input_stream<cacc48> *w = (input_stream<cacc48> *) _w;
            type ret;

            #pragma unroll
            for (unsigned i = 0; i < num_ops; ++i)
                ret.template insert<4>(i, ::readincr_v4cfloat(w));

            return ret;
        }
    }
};

#endif

}

/**
 * @ingroup group_adf
 *
 * Writes a vector into the output AXI stream. TLAST is not asserted.
 *
 * @tparam Resource Stream out resource. This information can be leveraged by the compiler to schedule stream writes in parallel when possible
 * @param w Output stream
 * @param value Vector to be written
 */
template <aie_stream_resource_out Resource = aie_stream_resource_out::none, typename T, unsigned N>
void              writeincr (output_stream<T> *w, const aie::vector<T, N> &value)             {        aie::detail::adf::stream_out_helper<N, T, Resource>::writeincr(w, value);        }
/**
 * @ingroup group_adf
 *
 * Writes a vector into the output AXI stream. An additional argument says whether TLAST is asserted.
 *
 * @tparam Resource Stream out resource. This information can be leveraged by the compiler to schedule stream writes in parallel when possible
 * @param w Output stream
 * @param value Vector to be written
 * @param tlast TLAST is asserted if true
 */
template <aie_stream_resource_out Resource = aie_stream_resource_out::none, typename T, unsigned N>
void              writeincr (output_stream<T> *w, const aie::vector<T, N> &value, bool tlast) {        aie::detail::adf::stream_out_helper<N, T, Resource>::writeincr(w, value, tlast); }

/**
 * @ingroup group_adf
 *
 * Reads a vector from the input AXI stream.
 *
 * @tparam N Size of the vector to be read
 * @tparam Resource Stream in resource. This information can be leveraged by the compiler to schedule stream reads in parallel when possible
 * @param w Input stream
 */
template <unsigned N, aie_stream_resource_in Resource = aie_stream_resource_in::none, typename T>
aie::vector<T, N> readincr_v( input_stream<T> *w)                                             { return aie::detail::adf::stream_in_helper<N, T, Resource>::readincr(w);                 }

/**
 * @ingroup group_adf
 *
 * Reads a vector from the input AXI stream and returns whether TLAST has been asserted.
 *
 * @tparam N        Size of the vector to be read
 * @tparam Resource Stream in resource. This information can be leveraged by the compiler to schedule stream reads in parallel when possible
 * @param w     Input stream
 * @param tlast Output argument that tells whether TLAST has been asserted.
 */
template <unsigned N, aie_stream_resource_in Resource = aie_stream_resource_in::none, typename T = void>
aie::vector<T, N> readincr_v( input_stream<T> *w, bool &tlast)                                { return aie::detail::adf::stream_in_helper<N, T, Resource>::readincr(w, tlast);          }

/**
 * @ingroup group_adf
 *
 * Writes an accumulator into the output cascade stream.
 *
 * @param w Output stream
 * @param value Accumulator to be written
 */
template <aie::AccumElemBaseType T, unsigned N>
void              writeincr(output_stream<T> *w,  const aie::accum<T, N> &value)              {        aie::detail::adf::cascade_stream_helper< T, N>::writeincr(w, value);   }

/**
 * @ingroup group_adf
 *
 * Reads an accumulator from the input cascade stream.
 *
 * @param w Input stream
 */
template <unsigned N, aie::AccumElemBaseType T>
aie::accum<T, N>  readincr_v( input_stream<T> *w)                                             { return aie::detail::adf::cascade_stream_helper<T, N>::readincr(w);            }

template <unsigned N, typename T> [[deprecated("Use writeincr instead")]] void writeincr_v(output_stream<T> *w, const aie::vector<T, N> &value)             { writeincr<N, T>(w, value); }
template <unsigned N, typename T> [[deprecated("Use writeincr instead")]] void writeincr_v(output_stream<T> *w, const aie::vector<T, N> &value, bool tlast) { writeincr<N, T>(w, value); }

template <aie::AccumElemBaseType T, unsigned N> [[deprecated("Use writeincr instead")]]
void writeincr_v(output_stream<T> *w,  const aie::accum<T, N> &value)                         { writeincr(w, value);    }

namespace aie {

/**
 * @ingroup group_adf
 *
 * Type used to wrap objects read/written from/to streams that also encapsulates TLAST information. When passed into
 * output AXI streams, this object will say whether TLAST is asserted after the write operation. When read from input
 * AXI streams, the object will say whether TLAST was asserted by the writer.
 */
template <typename T, typename Bool>
struct tlast
{
    using value_type = T;

    T value;
    Bool t_last;

    tlast(const tlast &) = delete;
    tlast &operator=(const tlast &) = delete;

    /**
     * Constructor used to initialize the object. For input AXI streams, both arguments are references to the variables
     * where to store the read value and TLAST. For output AXI streams, these are the value and TLAST to be written.
     */
    __aie_inline
    constexpr tlast(T value, Bool t_last) : value(value), t_last(t_last)
    {
    }
};

// Constructor deduction guides

// For input AXI streams, return an object that stores the references to the variables where to store the read value
// and TLAST
template <typename T>
tlast(T &, bool &) -> tlast<T &, bool &>;

// For output AXI streams, return an object that stores the value and TLAST to be written on the stream
template <typename T>
tlast(const T &, bool) -> tlast<const T &, bool>;

namespace detail {

// Helpers to determine whether a type is a tlast object

template <typename T>
struct is_tlast
{
    static constexpr bool value = false;
};

template <typename T, typename Bool>
struct is_tlast<tlast<T, Bool>>
{
    static constexpr bool value = true;
};

// Determine whether T is a tlast object
template <typename T>
static constexpr bool is_tlast_v = is_tlast<T>::value;

template <typename T, typename T2>
struct is_tlast_type
{
    static constexpr bool value = false;
};

template <typename T, typename Bool, typename T2>
struct is_tlast_type<tlast<T, Bool>, T2>
{
    static constexpr bool value = std::is_same_v<detail::utils::remove_all_t<T>, T2>;
};

template <typename T, unsigned N, typename Bool, typename T2>
struct is_tlast_type<tlast<vector<T, N> &, Bool>, T2>
{
    static constexpr bool value = std::is_same_v<T, T2>;
};

template <typename T, unsigned N, typename Bool, typename T2>
struct is_tlast_type<tlast<const vector<T, N> &, Bool>, T2>
{
    static constexpr bool value = std::is_same_v<T, T2>;
};

// Determine whether T is a tlast object of values of type T2. This includes both T2 values or vectors of T2
template <typename T, typename T2>
static constexpr bool is_tlast_type_v = is_tlast_type<T, T2>::value;

}

/**
 * @ingroup group_adf
 *
 * @interface aie::TLast
 * Concept for tlast objects passed to << and >> operators with streams.
 */
template <typename T, typename T2>
concept TLast = detail::is_tlast_v<T> && detail::is_tlast_type_v<detail::utils::remove_all_t<T>, T2>;

}

/**
 * @ingroup group_adf
 *
 * Writes a value into the output AXI stream. TLAST is not asserted.
 *
 * @param w Output stream
 * @param v Value to be written
 */
template <typename T>
__aie_inline
constexpr output_stream<T> &operator<<(output_stream<T> &w, const T &v)
{
    if constexpr (aie::detail::type_bits_v<T> < 32) {
        if constexpr (aie::detail::is_floating_point_v<T>) {
            float tmp = v;
            writeincr((output_stream<float> *)&w, tmp);
        }
        else {
            int tmp = v;
            writeincr((output_stream<int> *)&w, tmp);
        }
    }
    else {
        writeincr(&w, v);
    }

    return w;
}

/**
 * @ingroup group_adf
 *
 * Writes a value or vector into the output AXI stream. The tlast object says whether TLAST is asserted.
 *
 * @param w  Output stream
 * @param tl Object that contains the value or vector to be written and whether TLAST is asserted.
 */
template <typename T, aie::TLast<T> TL>
__aie_inline
constexpr output_stream<T> &operator<<(output_stream<T> &w, const TL &tl)
{
    using value_type = aie::detail::utils::remove_all_t<typename TL::value_type>;

    if constexpr (aie::detail::is_vector_v<value_type>) {
        writeincr(&w, tl.value, tl.t_last);
    }
    else {
        if constexpr (aie::detail::type_bits_v<T> < 32) {
            if constexpr (aie::detail::is_floating_point_v<T>) {
                float tmp = tl.value;
                writeincr((output_stream<float> *)&w, tmp, tl.t_last);
            }
            else {
                int tmp = tl.value;
                writeincr((output_stream<int> *)&w, tmp, tl.t_last);
            }
        }
        else {
            writeincr(&w, tl.value, tl.t_last);
        }
    }

    return w;
}

/**
 * @ingroup group_adf
 *
 * Writes a value into the output AXI stream. TLAST is not asserted.
 *
 * @param w Output stream
 * @param v Value to be written
 */
template <typename T>
__aie_inline
constexpr output_stream<T> &operator<<(output_stream<T> *w, const T &v)
{
    *w << v;
    return w;
}

/**
 * @ingroup group_adf
 *
 * Writes a value into the output AXI stream. The tlast object says whether TLAST is asserted.
 *
 * @param w  Output stream
 * @param tl Object that contains the value to be written and whether TLAST is asserted.
 */
template <typename T, aie::TLast<T> TL>
__aie_inline
constexpr output_stream<T> *operator<<(output_stream<T> *w, const TL &tl)
{
    *w << tl;
    return w;
}

/**
 * @ingroup group_adf
 *
 * Writes a vector into the output AXI stream. TLAST is not asserted.
 *
 * @param w Output stream
 * @param v Vector to be written
 */
template <typename T, unsigned N>
__aie_inline
constexpr output_stream<T> &operator<<(output_stream<T> &w, const aie::vector<T, N> &v)
{
    writeincr(&w, v);
    return w;
}

/**
 * @ingroup group_adf
 *
 * Writes a vector into the output AXI stream. TLAST is not asserted.
 *
 * @param w Output stream
 * @param v Vector to be written
 */
template <typename T, unsigned N>
__aie_inline
constexpr output_stream<T> *operator<<(output_stream<T> *w, const aie::vector<T, N> &v)
{
    *w << v;
    return w;
}

/**
 * @ingroup group_adf
 *
 * Writes an accumulator into the output cascade stream
 *
 * @param w   Output stream
 * @param acc Accumulator to be written
 */
template <aie::AccumElemBaseType T, unsigned N>
__aie_inline
constexpr output_stream<T> *operator<<(output_stream<T> *w, const aie::accum<T, N> &acc)
{
    writeincr(w, acc);
    return w;
}

/**
 * @ingroup group_adf
 *
 * Writes an accumulator into the output cascade stream
 *
 * @param w   Output stream
 * @param acc Accumulator to be written
 */
template <aie::AccumElemBaseType T, unsigned N>
__aie_inline
constexpr output_stream<T> &operator<<(output_stream<T> &w, const aie::accum<T, N> &acc)
{
    writeincr(&w, acc);
    return w;
}

/**
 * @ingroup group_adf
 *
 * Reads a value from the input AXI stream.
 *
 * @param w Input stream
 * @param v Value read from stream. Output argument.
 */
template <typename T>
__aie_inline
constexpr input_stream<T> &operator>>(input_stream<T> &w, T &v)
{
    // AXI stream width is 32b. Scalar reads for types smaller than 32b are promoted to 32b
    if constexpr (aie::detail::type_bits_v<T> < 32) {
        if constexpr (aie::detail::is_floating_point_v<T>) {
            float tmp = readincr((input_stream<float> *)&w);

            v = tmp;
        }
        else {
            int tmp = readincr((input_stream<int> *)&w);

            v = tmp;
        }
    }
    else {
        v = readincr(&w);
    }

    return w;
}

/**
 * @ingroup group_adf
 *
 * Reads a value or vector from the input AXI stream. The tlast object says whether TLAST is asserted.
 *
 * @param w  Input stream
 * @param tl Object that contains the value or vector read from stream. Output argument.
 */
template <typename T, aie::TLast<T> TL>
__aie_inline
constexpr input_stream<T> &operator>>(input_stream<T> &w, const TL &tl)
{
    using value_type = aie::detail::utils::remove_all_t<typename TL::value_type>;

    if constexpr (aie::detail::is_vector_v<value_type>) {
        tl.value = readincr_v<value_type::size()>(&w, tl.t_last);
    }
    else {
        // AXI stream width is 32b. Scalar writes for types smaller than 32b are promoted to 32b
        if constexpr (aie::detail::type_bits_v<T> < 32) {
            if constexpr (aie::detail::is_floating_point_v<T>) {
                const float tmp = readincr((input_stream<float> *)&w, tl.t_last);

                tl.value = tmp;
            }
            else {
                const int tmp = readincr((input_stream<int> *)&w, tl.t_last);

                tl.value = tmp;
            }
        }
        else {
            tl.value = readincr(&w, tl.t_last);
        }
    }

    return w;
}

/**
 * @ingroup group_adf
 *
 * Reads a value from the input AXI stream.
 *
 * @param w Input stream
 * @param v Value read from stream. Output argument.
 */
template <typename T>
__aie_inline
constexpr input_stream<T> &operator>>(input_stream<T> *w, T &v)
{
    *w >> v;
    return w;
}

/**
 * @ingroup group_adf
 *
 * Reads a value or vector from the input AXI stream. The tlast object says whether TLAST is asserted.
 *
 * @param w  Input stream
 * @param tl Object that contains the value or vector read from stream. Output argument.
 */
template <typename T, aie::TLast<T> TL>
__aie_inline
constexpr input_stream<T> *operator>>(input_stream<T> *w, const TL &tl)
{
    *w >> tl;
    return w;
}

/**
 * @ingroup group_adf
 *
 * Reads a vector from the input AXI stream.
 *
 * @param w Input stream
 * @param v Vector read from stream. Output argument.
 */
template <unsigned N, typename T>
__aie_inline
constexpr input_stream<T> &operator>>(input_stream<T> &w, aie::vector<T, N> &v)
{
    v = readincr_v<N>(&w);
    return w;
}

/**
 * @ingroup group_adf
 *
 * Reads a vector from the input AXI stream.
 *
 * @param w Input stream
 * @param v Vector read from stream. Output argument.
 */
template <unsigned N, typename T>
__aie_inline
constexpr input_stream<T> *operator>>(input_stream<T> *w, aie::vector<T, N> &v)
{
    *w >> v;
    return w;
}

/**
 * @ingroup group_adf
 *
 * Reads an accumulator from the input cascade stream.
 *
 * @param w   Input stream
 * @param acc Accumulator read from stream. Output argument.
 */
template <unsigned N, aie::AccumElemBaseType T>
__aie_inline
constexpr input_stream<T> *operator>>(input_stream<T> *w, aie::accum<T, N> &acc)
{
    acc = readincr_v<N>(w);
    return w;
}

/**
 * @ingroup group_adf
 *
 * Reads an accumulator from the input cascade stream.
 *
 * @param w   Input stream
 * @param acc Accumulator read from stream. Output argument.
 */
template <unsigned N, aie::AccumElemBaseType T>
__aie_inline
constexpr input_stream<T> &operator>>(input_stream<T> &w, aie::accum<T, N> &acc)
{
    acc = readincr_v<N>(&w);
    return w;
}

#endif
