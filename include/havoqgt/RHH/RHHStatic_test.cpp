/*
 * Copyright (c) 2013, Lawrence Livermore National Security, LLC.
 * Produced at the Lawrence Livermore National Laboratory.
 * Written by Roger Pearce <rpearce@llnl.gov>.
 * LLNL-CODE-644630.
 * All rights reserved.
 *
 * This file is part of HavoqGT, Version 0.1.
 * For details, see https://computation.llnl.gov/casc/dcca-pub/dcca/Downloads.html
 *
 * Please also read this link – Our Notice and GNU Lesser General Public License.
 *   http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License (as published by the Free
 * Software Foundation) version 2.1 dated February 1999.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the IMPLIED WARRANTY OF MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the terms and conditions of the GNU General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * OUR NOTICE AND TERMS AND CONDITIONS OF THE GNU GENERAL PUBLIC LICENSE
 *
 * Our Preamble Notice
 *
 * A. This notice is required to be provided under our contract with the
 * U.S. Department of Energy (DOE). This work was produced at the Lawrence
 * Livermore National Laboratory under Contract No. DE-AC52-07NA27344 with the DOE.
 *
 * B. Neither the United States Government nor Lawrence Livermore National
 * Security, LLC nor any of their employees, makes any warranty, express or
 * implied, or assumes any liability or responsibility for the accuracy,
 * completeness, or usefulness of any information, apparatus, product, or process
 * disclosed, or represents that its use would not infringe privately-owned rights.
 *
 * C. Also, reference herein to any specific commercial products, process, or
 * services by trade name, trademark, manufacturer or otherwise does not
 * necessarily constitute or imply its endorsement, recommendation, or favoring by
 * the United States Government or Lawrence Livermore National Security, LLC. The
 * views and opinions of authors expressed herein do not necessarily state or
 * reflect those of the United States Government or Lawrence Livermore National
 * Security, LLC, and shall not be used for advertising or product endorsement
 * purposes.
 *
 */


/// How to use
/// $ g++ std=c++11 RHHStatic_test.cpp
/// $ ./a.out

#include <iostream>
#include <cstdint>
#include <cassert>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/adaptive_pool.hpp>
#include <stdio.h>
#include <time.h>
#include "RHHCommon.hpp"
#include "RHHMgrStatic.hpp"

 int main (void)
 {
  boost::interprocess::file_mapping::remove( "./tmp.dat" );
  boost::interprocess::managed_mapped_file mfile( boost::interprocess::create_only, "./tmp.dat", 1ULL<<28ULL );
  RHH::AllocatorsHolder holder = RHH::AllocatorsHolder(mfile.get_segment_manager());
  RHH::RHHMgrStatic<uint64_t, RHH::NoValueType> *rhh = new RHH::RHHMgrStatic<uint64_t, RHH::NoValueType>(holder);

  srand((unsigned int)time(NULL));

  uint64_t num_elems = 0;
  unsigned char dmy = 0;
  for (int i = 0; i < (1<<22); i++) {
    uint64_t key = rand() % 32768;
    bool result = rhh->insert_uniquely(holder, key, dmy, num_elems);
    std::cout << key << "\t" << result << std::endl;
    num_elems += result;
  }
  delete rhh;

  return 0;
}