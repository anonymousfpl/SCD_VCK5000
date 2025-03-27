//
// Copyright (C) 2024, Advanced Micro Devices, Inc. All rights reserved.
// SPDX-License-Identifier: MIT
//

#pragma once

#include <adf.h>
#include <aie_api/aie.hpp>

using namespace adf;


class fam_stage1 {
private:
    int m_xoff; 
public:
    fam_stage1(int xoff);

  // Run:
  void run( input_stream_cfloat* __restrict inputx0,
                        output_buffer_1d<cfloat,2048>& __restrict outputy );

  static void registerKernelClass( void )
  {
    REGISTER_FUNCTION( fam_stage1::run );
  }
};

