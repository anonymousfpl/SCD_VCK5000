//
// Copyright (C) 2024, Advanced Micro Devices, Inc. All rights reserved.
// SPDX-License-Identifier: MIT
//

#pragma once

#include <adf.h>
#include <adf/stream/types.h>
#include <aie_api/aie.hpp>

using namespace adf;


class conv_stage2 {
private:
    int m_xoff;  

public:
    conv_stage2(int xoff);

  // Run:
  void run(     input_stream_cfloat * __restrict  inputx0,
                       input_stream_cfloat * __restrict  inputx1,
                       output_stream_cfloat * __restrict  outputy );

  static void registerKernelClass( void )
  {
    REGISTER_FUNCTION( conv_stage2::run );
  }
};

