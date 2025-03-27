//
// Copyright (C) 2024, Advanced Micro Devices, Inc. All rights reserved.
// SPDX-License-Identifier: MIT
//

#pragma once

#include <adf.h>
#include <adf/stream/types.h>
#include <aie_api/aie.hpp>

using namespace adf;


class norm {
private:
    int m_xoff;  

public:
    norm(int xoff);

  // Run:
  void run( input_buffer_1d<float,1024,0>& __restrict inputxR, 
                input_buffer_1d<float,1024,0>& __restrict inputxI, 
                output_buffer_1d<cfloat,1024>& __restrict outputy0,
                output_buffer_1d<cfloat,1024>& __restrict outputy1 );

  static void registerKernelClass( void )
  {
    REGISTER_FUNCTION( norm::run );
  }
};

