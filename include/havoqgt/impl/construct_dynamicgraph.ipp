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

#ifndef HAVOQGT_MPI_IMPL_DELEGATE_PARTITIONED_GRAPH_IPP_INCLUDED
#define HAVOQGT_MPI_IMPL_DELEGATE_PARTITIONED_GRAPH_IPP_INCLUDED


/**
 * \file
 * Implementation of delegate_partitioned_graph and internal classes.
 */

 namespace havoqgt {
  namespace mpi {

  template <typename SegementManager>
    const char construct_dynamicgraph<SegementManager>::kNoValue = 0;
/**
 * Constructor
 *
 * @param seg_allocator       Reference to segment allocator
 */
 template <typename SegementManager>
 construct_dynamicgraph<SegementManager>::
 construct_dynamicgraph (
  mapped_t& asdf,
  SegmentAllocator<void>& seg_allocator,
  const DataStructureMode mode,
  const uint64_t n )
 : asdf_(asdf)
 , seg_allocator_(seg_allocator)
 , data_structure_type_(mode)
 , kLowDegreeThreshold(n)
 {

  switch(data_structure_type_) {
    case kUseVecVecMatrix:
    adjacency_matrix_vec_vec_ = new adjacency_matrix_vec_vec_t(seg_allocator_);
    init_vec = new uint64_vector_t(seg_allocator_);
    break;

    case kUseMapVecMatrix:
    adjacency_matrix_map_vec_ = new adjacency_matrix_map_vec_t(seg_allocator_);
    break;

    case kUseRHHAsArray:
    rhh_single_array = new rhh_single_array_t(seg_allocator_);
    break;

    case kUseRHHAsMatrix:
    std::srand(1);
    std::cout << "Random seed is 1" << std::endl;
    // std::srand(std::time(0));
    // std::cout << "Random seed is time" << std::endl;
    rhh_matrix_ = new rhh_matrix_t(seg_allocator_);
    break;

    case kUseHybridDegreeAwareModel:
    std::srand(1);
    std::cout << "Random seed is 1" << std::endl;
    alloc_holder = new RHH::AllocatorsHolder(seg_allocator_.get_segment_manager());
    hybrid_matrix = new RHH::RHHMain<uint64_t, uint64_t>(*alloc_holder, 2ULL);
    break;

    default:
    std::cerr << "Unknown data structure type" << std::endl;
    assert(false);
    exit(-1);
  }

  // io_info_ = new IOInfo();
  total_exectution_time_  = 0.0;

#if DEBUG_DUMPUPDATEREQUESTANDRESULTS == 1
  fout_debug_insertededges_.open(kFnameDebugInsertedEdges+"_raw");
#endif

}

/**
 * Deconstructor
 */
template <typename SegementManager>
 construct_dynamicgraph<SegementManager>::
 ~construct_dynamicgraph()
 {

  if (data_structure_type_ == kUseRHHAsMatrix) {
    rhh_matrix_->free(seg_allocator_);
  }
  if (data_structure_type_ == kUseHybridDegreeAwareModel) {
    hybrid_matrix->free(*alloc_holder);
  }

  delete adjacency_matrix_vec_vec_; /// kUseVecVecMatrix
  delete init_vec; /// kUseVecVecMatrix
  delete adjacency_matrix_map_vec_; /// kUseMapVecMatrix
  delete rhh_single_array; /// kUseRHHAsArray
  delete rhh_matrix_; /// kUseRHHAsMatrix
  delete alloc_holder; /// kUseHybridDegreeAwareModel
  delete hybrid_matrix; /// kUseHybridDegreeAwareModel

  // delete io_info_;

#if DEBUG_DUMPUPDATEREQUESTANDRESULTS == 1
  fout_debug_insertededges_.close();
#endif

}


/**
 * Add edges into vector-vector adjacency matrix using
 * boost:interprocess:vector with from and unsorted sequence of edges.
 *
 * @param edges               input edges to partition
*/
template <typename SegementManager>
template <typename Container>
 void construct_dynamicgraph<SegementManager>::
 add_edges_adjacency_matrix_vector_vector(Container req_itr, size_t length)
 {
  // TODO: make initializer
  if (adjacency_matrix_vec_vec_->size() == 0) {
    adjacency_matrix_vec_vec_->resize(1, *init_vec);
  }

  // io_info_->reset_baseline();
  double time_start = MPI_Wtime();
  for (size_t k = 0; k < length; ++k, ++req_itr) {
    const auto &edge = req_itr->edge;

    while (adjacency_matrix_vec_vec_->size() <= edge.first) {
      adjacency_matrix_vec_vec_->resize(adjacency_matrix_vec_vec_->size() * 2, *init_vec);
    }
    uint64_vector_t& adjacency_list_vec = adjacency_matrix_vec_vec_->at(edge.first);
#if WITHOUT_DUPLICATE_INSERTION == 1
    /// add a edge without duplication
    if (boost::find<uint64_vector_t>(adjacency_list_vec, edge.second) == adjacency_list_vec.end()) {
      adjacency_list_vec.push_back(edge.second);
    }
#else
    adjacency_list_vec.push_back(edge.second);
#endif
  }
  flush_pagecache();
  double time_end = MPI_Wtime();

  std::cout << "TIME: Execution time (sec.) =\t" << time_end - time_start << std::endl;
  total_exectution_time_ += time_end - time_start;

  // io_info_->log_diff();

}

template <typename SegmentManager>
template <typename Container>
void construct_dynamicgraph<SegmentManager>::
add_edges_adjacency_matrix_map_vector(Container req_itr, size_t length)
{

  // io_info_->reset_baseline();
  double time_start = MPI_Wtime();
  for (size_t k = 0; k < length; ++k, ++req_itr) {
    const auto &edge = req_itr->edge;
    auto value = adjacency_matrix_map_vec_->find(edge.first);
    if (value == adjacency_matrix_map_vec_->end()) { // new vertex
      uint64_vector_t vec(1, edge.second, seg_allocator_);
      adjacency_matrix_map_vec_->insert(map_value_vec_t(edge.first, vec));
    } else {
      uint64_vector_t& adjacency_list_vec = value->second;

#if WITHOUT_DUPLICATE_INSERTION == 1
      // add a edge without duplication
      if (boost::find<uint64_vector_t>(adjacency_list_vec, edge.second) != adjacency_list_vec.end() )
        continue;
#endif

      adjacency_list_vec.push_back(edge.second);

    }
  }
  flush_pagecache();
  double time_end = MPI_Wtime();

  std::cout << "TIME: Execution time (sec.) =\t" << time_end - time_start << std::endl;

  total_exectution_time_ += time_end - time_start;
  // io_info_->log_diff();

}

template <typename SegmentManager>
template <typename Container>
void construct_dynamicgraph<SegmentManager>::
add_edges_rhh_single_array(Container req_itr, size_t length)
{
  // io_info_->reset_baseline();
  double time_start = MPI_Wtime();
  for (size_t k = 0; k < length; ++k, ++req_itr) {
    const auto& edge = req_itr->edge;
#if DEBUG_DUMPUPDATEREQUESTANDRESULTS == 1
    fout_debug_insertededges_ << edge.first << "\t" << edge.second << std::endl;
#endif
#if WITHOUT_DUPLICATE_INSERTION == 1
    rhh_single_array->insert_unique(edge.first, edge.second);
#else
    rhh_single_array->insert(edge.first, edge.second);
#endif
  }
  flush_pagecache();
  double time_end = MPI_Wtime();

  std::cout << "TIME: Execution time (sec.) =\t" << time_end - time_start << std::endl;

  total_exectution_time_ += time_end - time_start;
  // io_info_->log_diff();
}


template <typename SegmentManager>
template <typename Container>
void construct_dynamicgraph<SegmentManager>::
add_edges_rhh_matrix(Container req_itr, size_t length)
{
  uint64_t count_inserted = 0;
  uint64_t count_deleted = 0;

  // io_info_->reset_baseline();
  double time_start = MPI_Wtime();
  for (size_t k = 0; k < length; ++k, ++req_itr) {

    const uint64_t source_vtx = req_itr->edge.first;
    const uint64_t target_vtx = req_itr->edge.second;

#if DEBUG_DUMPUPDATEREQUESTANDRESULTS == 1
    fout_debug_insertededges_ << source_vtx << "\t" << target_vtx << "\t0" << std::endl;
#endif

    if (req_itr->is_delete) {
      count_deleted += rhh_matrix_->erase(seg_allocator_, source_vtx, target_vtx);
#if DEBUG_DUMPUPDATEREQUESTANDRESULTS == 1
      fout_debug_insertededges_ << source_vtx << "\t" << target_vtx << "\t1" << std::endl;
#endif
    } else {
      count_inserted += rhh_matrix_->insert_uniquely(seg_allocator_, source_vtx, target_vtx);
    }


  } // End of a edges insertion loop
  flush_pagecache();
  double time_end = MPI_Wtime();

  std::cout << "TIME: Execution time (sec.) =\t" << time_end - time_start << std::endl;
  std::cout << "Count: # inserted edges =\t" << count_inserted << std::endl;
  std::cout << "Count: # deleted edges =\t" << count_deleted << std::endl;
  // io_info_->log_diff();
  total_exectution_time_ += time_end - time_start;
}


template <typename SegmentManager>
template <typename Container>
void construct_dynamicgraph<SegmentManager>::
add_edges_degree_aware_hybrid(Container req_itr, const size_t length)
{
  uint64_t count_inserted = 0;
  uint64_t count_deleted = 0;
  double time_start = MPI_Wtime();
  for (size_t k = 0; k < length; ++k, ++req_itr) {

    const uint64_t source_vtx = req_itr->edge.first;
    const uint64_t target_vtx = req_itr->edge.second;

#if DEBUG_DUMPUPDATEREQUESTANDRESULTS == 1
    fout_debug_insertededges_ << source_vtx << "\t" << target_vtx << "\t0" << std::endl;
#endif

    if (req_itr->is_delete) {
#if DEBUG_DUMPUPDATEREQUESTANDRESULTS == 1
      fout_debug_insertededges_ << source_vtx << "\t" << target_vtx << "\t1" << std::endl;
#endif
      count_deleted += hybrid_matrix->delete_item(*alloc_holder, source_vtx, target_vtx);
    } else {
      count_inserted += hybrid_matrix->insert_uniquely(*alloc_holder, source_vtx, target_vtx);
    }
  } // End of a edges insertion loop
  flush_pagecache();
  double time_end = MPI_Wtime();

  std::cout << "TIME: Execution time (sec.) =\t" << time_end - time_start << std::endl;
  std::cout << "Count: # inserted edges =\t" << count_inserted << std::endl;
  std::cout << "Count: # deleted edges =\t" << count_deleted << std::endl;

  total_exectution_time_ += time_end - time_start;
}


template <typename SegmentManager>
void construct_dynamicgraph<SegmentManager>::
print_profile()
{
  std::cout << "TIME: Total Execution time (sec.) =\t" << total_exectution_time_ << std::endl;
  // io_info_->log_diff(true);

  // std::cout << "WITHOUT_DUPLICATE_INSERTION is " << WITHOUT_DUPLICATE_INSERTION << std::endl;
  std::cout << "DEBUG_DUMPUPDATEREQUESTANDRESULTS is " << DEBUG_DUMPUPDATEREQUESTANDRESULTS << std::endl;
#if DEBUG_DUMPUPDATEREQUESTANDRESULTS == 1
  std::cout << "kFnameDebugInsertedEdges is " << kFnameDebugInsertedEdges << std::endl;
#endif

  if (data_structure_type_ == kUseRHHAsArray || data_structure_type_ == kUseRHHAsMatrix) {
    std::cout << "USE_SEPARATE_HASH_ARRAY is " << USE_SEPARATE_HASH_ARRAY << std::endl;
    std::cout << "USE_TOMBSTONE is " << USE_TOMBSTONE << std::endl;
    //std::cout << "# elements in Robin-Hood-Hashing = " << rhh_single_array->size() << std::endl;
  }

  if (data_structure_type_ == kUseRHHAsMatrix) {
    std::cout << "\n<Status of the data structure>" << std::endl;
    rhh_matrix_->disp_status();
    std::cout << "USE_SEPARATE_HASH_ARRAY is " << USE_SEPARATE_HASH_ARRAY << std::endl;
    std::cout << "USE_TOMBSTONE is " << USE_TOMBSTONE << std::endl;
    std::cout << "--------------------" << std::endl;
  }

  if (data_structure_type_ == kUseHybridDegreeAwareModel) {
    std::cout << "\n<Status of the data structure>" << std::endl;
    hybrid_matrix->disp_profileinfo();
  }

#if DEBUG_DUMPUPDATEREQUESTANDRESULTS == 1
  std::ofstream tmp;
  tmp.open(kFnameDebugInsertedEdges+"_graph", std::ios::trunc);
  tmp.close();

  if (data_structure_type_ == kUseMapVecMatrix) {
    std::ofstream fout;
    fout.open(kFnameDebugInsertedEdges+"_graph", std::ios::out | std::ios::app);
    for (auto itr = adjacency_matrix_map_vec_->begin(); itr != adjacency_matrix_map_vec_->end(); ++itr) {
      uint64_vector_t& adjacency_list_vec = (*itr).second;
      for (auto itr2 = adjacency_list_vec.begin(); itr2 != adjacency_list_vec.end(); ++itr2) {
        fout << (*itr).first << "\t" << *itr2 << std::endl;
      }
    }
    fout.close();
  }

  if (data_structure_type_ == kUseRHHAsArray ) {
    std::cout << "# elements in Robin-Hood-Hashing = " << rhh_single_array->size() << std::endl;
    rhh_single_array->dump_elements(kFnameDebugInsertedEdges+"_graph");
    //rhh_single_array->disp_elements();
  }

  if (data_structure_type_ == kUseRHHAsMatrix) {
    std::ofstream fout;
    fout.open(kFnameDebugInsertedEdges+"_graph", std::ios::out | std::ios::app);
    rhh_matrix_->dump_elements(kFnameDebugInsertedEdges+"_graph");
    rhh_matrix_->dump_probedistance(kFnameDebugInsertedEdges+"_probedistance");
    fout.close();
  }
  if (data_structure_type_ == kUseHybridDegreeAwareModel) {
    std::ofstream fout;
    fout.open(kFnameDebugInsertedEdges+"_graph", std::ios::out);
    hybrid_matrix->fprint_elems(fout);
    fout.close();
  }
#endif

#if DEBUG_DETAILPROFILE == 1
  if (data_structure_type_ == kUseHybridDegreeAwareModel) {
    std::ofstream fout;
    fout.open("/l/ssd/g_value_length.log", std::ios::out);
    hybrid_matrix->fprint_value_lengths(fout);
    fout.close();
  }
  if (data_structure_type_ == kUseHybridDegreeAwareModel) {
    std::ofstream fout;
    fout.open("/l/ssd/g_adjlistprobedist.log", std::ios::out);
    hybrid_matrix->fprint_adjlists_prbdist(fout);
    fout.close();
  }
  if (data_structure_type_ == kUseHybridDegreeAwareModel) {
    std::ofstream fout;
    fout.open("/l/ssd/g_adjlistdepth.log", std::ios::out);
    hybrid_matrix->fprint_adjlists_depth(fout);
    fout.close();
  }
#endif
}

template <typename SegmentManager>
void construct_dynamicgraph<SegmentManager>::
reset_profile()
{
  total_exectution_time_ = 0.0;
}

} // namespace mpi
} // namespace havoqgt


#endif //HAVOQGT_MPI_IMPL_DELEGATE_PARTITIONED_GRAPH_IPP_INCLUDED
