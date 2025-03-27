//
// Copyright (C) 2024, Advanced Micro Devices, Inc. All rights reserved.
// SPDX-License-Identifier: MIT
//

#pragma once

#include <adf.h>
#include <adf/stream/types.h>
#include <aie_api/aie.hpp>

using namespace adf;


class channel1 {
private:
    int m_xoff;  

public:
    channel1(int xoff);

  // Run:
  void run( input_buffer_1d<cfloat,1024,0>& __restrict  inputx0,
                       output_stream_cfloat * __restrict  outputy0,
                       output_stream_cfloat * __restrict  outputy1 );

  static void registerKernelClass( void )
  {
    REGISTER_FUNCTION( channel1::run );
  }
};

