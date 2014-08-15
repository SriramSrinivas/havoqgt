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

#ifndef HAVOQGT_MPI_DELEGATE_PARTITIONED_GRAPH_HPP_INCLUDED
#define HAVOQGT_MPI_DELEGATE_PARTITIONED_GRAPH_HPP_INCLUDED


#include <limits>
#include <utility>
#include <stdint.h>
#include <functional>

#include <boost/unordered_set.hpp>
#include <boost/unordered_map.hpp>
#include <boost/container/map.hpp>
#include <boost/interprocess/containers/map.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/allocators/allocator.hpp>

#include <havoqgt/mpi.hpp>
#include <havoqgt/utilities.hpp>
#include <havoqgt/cache_utilities.hpp>
#include <havoqgt/detail/iterator.hpp>
#include <havoqgt/impl/edge_partitioner.hpp>
#include <havoqgt/impl/edge_node_identifier.hpp>

#ifndef PROCESSES_PER_NODE
 #define PROCESSES_PER_NODE 24
 #warning using default processer node of 24.
#endif

#ifndef EDGE_PASS_PARTITIONS
 #define EDGE_PASS_PARTITIONS 4
 #warning using default edge pass partitions of four.
#endif

#ifndef EDGE_CHUNK_SIZE
 #define EDGE_CHUNK_SIZE 1024*64
 #warning using default send chunk size of 2^16
#endif

#ifdef DEBUG
 #warning Debug MACRO is for delegate_partitioned_graph.
 #define IS_DEBUGING true
#else
 #define IS_DEBUGING false
#endif

namespace havoqgt {
namespace mpi {

namespace bip = boost::interprocess;

/**
 * Delegate partitioned graph using MPI for communication.
 *
 * @todo Test using simple deterministic patterns.
 * @todo Add edge_data
 * @todo Make vertex_iterator a random access iterator
 * @todo Add invalid bit or state to vertex_locator
 * @todo Verify low-degree CSR creation:  ipp line 167
 * @todo Boostify controller locator
 */
template <typename SegementManager>
class delegate_partitioned_graph {
 public:
   template<typename T>
   using SegmentAllocator = bip::allocator<T, SegementManager>;

  //////////////////////////////////////////////////////////////////////////////
  // Class Objects
  //////////////////////////////////////////////////////////////////////////////
  /// Object that uniquely locates a vertex, both MPI rank and local offset.
  class vertex_locator;
  /// Edge Iterator class for delegate partitioned graph
  class edge_iterator;
  /// Vertex Iterator class for delegate partitioned graph
  class vertex_iterator;
  /// Vertex Data storage
  template <typename T, typename SegManagerOther>
  class vertex_data;
  /// Edge Data storage
  template <typename T, typename SegManagerOther>
  class edge_data;
  /// Stores information about owned vertices
  class vert_info;

  enum ConstructionState { New, MetaDataGenerated, EdgeStorageAllocated,
    LowEdgesPartitioned, HighEdgesPartitioned, GraphReady};

  //////////////////////////////////////////////////////////////////////////////
  // Public Member Functions
  //////////////////////////////////////////////////////////////////////////////
  /// Constructor that initializes given and unsorted sequence of edges
  template <typename Container>
  delegate_partitioned_graph(const SegmentAllocator<void>& seg_allocator,
                             MPI_Comm mpi_comm,
                             Container& edges, uint64_t max_vertex,
                             uint64_t delegate_degree_threshold,
                             std::function<void()> dont_need_graph);

  void print_graph_statistics();

  /// Converts a vertex_locator to the vertex label
  uint64_t locator_to_label(vertex_locator locator) const;

  /// Converts a vertex label to a vertex_locator
  vertex_locator label_to_locator(uint64_t label) const;

  /// Returns a begin iterator for edges of a vertex
  edge_iterator edges_begin(vertex_locator locator) const;

  /// Returns an end iterator for edges of a vertex
  edge_iterator edges_end(vertex_locator locator) const;

  /// Returns the degree of a vertex
  uint64_t degree(vertex_locator locator) const;

  /// Returns the local degree of a vertex
  uint64_t local_degree(vertex_locator locator) const;

  /// Returns a begin iterator for all local vertices
  vertex_iterator vertices_begin() const;

  /// Returns an end iterator for all local vertices
  vertex_iterator vertices_end() const;

  /// Tests if vertex label is a delegate
  bool is_label_delegate(uint64_t label) const;

  /// Creates vertex_data of type T
  template <typename T, typename SegManagerOther>
  vertex_data<T, SegManagerOther>* create_vertex_data(
      SegManagerOther*,
      const char *obj_name = nullptr) const;

  /// Creates vertex_data of type T, with initial value
  template <typename T, typename SegManagerOther>
  vertex_data<T, SegManagerOther>* create_vertex_data(
      const T& init, SegManagerOther*,
      const char *obj_name = nullptr) const;

  /// Creates edge_data of type T
  template <typename T, typename SegManagerOther>
  edge_data<T, SegManagerOther>* create_edge_data(
      SegManagerOther*,
      const char *obj_name = nullptr) const;

  /// Creates edge_data of type T, with initial value
  template <typename T, typename SegManagerOther>
  edge_data<T, SegManagerOther>* create_edge_data(
      const T& init, SegManagerOther*,
      const char *obj_name = nullptr) const;

  size_t num_local_vertices() const {
    return m_owned_info.size();
  }

  uint64_t max_vertex_id() {
    return m_max_vertex;
  }


  size_t num_delegates() const {
    return m_delegate_degree.size();
  }

  uint32_t master(const vertex_locator& locator) const {
    return locator.m_local_id % m_mpi_size;
  }

  typedef typename bip::vector<vertex_locator, SegmentAllocator<vertex_locator> >
      ::const_iterator controller_iterator;

  controller_iterator controller_begin() const {
    return m_controller_locators.begin();
  }

  controller_iterator controller_end()   const {
    return m_controller_locators.end();
  }

  inline bool operator==(delegate_partitioned_graph<SegementManager>&
      other) {
    return (m_mpi_size == other.m_mpi_size) &&
          (m_mpi_rank == other.m_mpi_rank) &&
          (m_owned_info == other.m_owned_info) &&
          (m_owned_targets == other.m_owned_targets) &&
          (m_delegate_info == other.m_delegate_info) &&
          (m_delegate_degree == other.m_delegate_degree) &&
          (m_delegate_label == other.m_delegate_label) &&
          (m_delegate_targets == other.m_delegate_targets) &&
          (m_map_delegate_locator != other.m_map_delegate_locator) &&
          (m_delegate_degree_threshold == other.m_delegate_degree_threshold) &&
          (m_controller_locators == other.m_controller_locators) &&
          (m_local_outgoing_count == other.m_local_outgoing_count) &&
          (m_local_incoming_count == other.m_local_incoming_count);
  }

  inline bool operator!=(delegate_partitioned_graph<SegementManager>&
      other) {
    return !(*this == other);
  }

 private:
  //////////////////////////////////////////////////////////////////////////////
  // Private Member Functions
  //////////////////////////////////////////////////////////////////////////////
  /// Synchronizes hub set amongst all processes.
  void sync_global_hub_set(const boost::unordered_set<uint64_t>& local_hubs,
                           boost::unordered_set<uint64_t>& global_hubs,
                           bool local_change);


  void initialize_low_meta_data(boost::unordered_set<uint64_t>& global_hub_set);
  void initialize_high_meta_data(boost::unordered_set<uint64_t>& global_hubs);

  void initialize_edge_storage();

  template <typename InputIterator>
  void partition_low_degree(InputIterator unsorted_itr,
                 InputIterator unsorted_itr_end,
                 boost::unordered_set<uint64_t>& global_hub_set);

  template <typename InputIterator>
  void count_high_degree_edges(InputIterator unsorted_itr,
                 InputIterator unsorted_itr_end,
                 boost::unordered_set<uint64_t>& global_hub_set);


  template <typename InputIterator>
  void partition_high_degree(InputIterator orgi_unsorted_itr,
    InputIterator unsorted_itr_end,
    std::map< uint64_t, std::deque<OverflowSendInfo> > &transfer_info);


  template <typename InputIterator>
  void count_edge_degrees(InputIterator unsorted_itr,
                 InputIterator unsorted_itr_end,
                 boost::unordered_set<uint64_t>& global_hub_set,
                 uint64_t delegate_degree_threshold);


  void send_high_info(std::vector< boost::container::map< uint64_t, uint64_t> >&
      maps_to_send, int maps_to_send_element_count);


  void send_vertex_info(uint64_t &high_vertex_count,
      uint64_t delegate_degree_threshold,
      std::vector<  boost::container::map<
          int, std::pair<uint64_t, uint64_t> >  >& maps_to_send,
      int maps_to_send_element_count);


  void calculate_overflow(std::map< uint64_t, std::deque<OverflowSendInfo> >
    &transfer_info);

  void generate_send_list(std::vector<uint64_t> &send_list, uint64_t num_send,
    int send_id,
    std::map< uint64_t, std::deque<OverflowSendInfo> > &transfer_info);

  void flush_graph();

  //////////////////////////////////////////////////////////////////////////////
  // Private Data Members
  //////////////////////////////////////////////////////////////////////////////
  const static uint64_t edge_chunk_size = EDGE_CHUNK_SIZE;
  const int processes_per_node = PROCESSES_PER_NODE;
  const int node_partitions = EDGE_PASS_PARTITIONS;
  int m_mpi_size;
  int m_mpi_rank;
  MPI_Comm m_mpi_comm;

  ConstructionState m_graph_state {New};

  std::function<void()> m_dont_need_graph;

  uint64_t m_max_vertex {0};
  uint64_t m_global_edge_count {0};
  uint64_t m_edges_high_count {0};
  uint64_t m_edges_low_count {0};

  bip::vector<uint32_t, SegmentAllocator<uint32_t> > m_local_outgoing_count;
  bip::vector<uint32_t, SegmentAllocator<uint32_t> > m_local_incoming_count;

  bip::vector<vert_info, SegmentAllocator<vert_info>> m_owned_info;
  bip::vector<uint32_t, SegmentAllocator<uint32_t>> m_owned_info_tracker;
  bip::vector<vertex_locator, SegmentAllocator<vertex_locator>> m_owned_targets;

  // Delegate Storage
  uint64_t m_delegate_degree_threshold;

  bip::vector< uint64_t, SegmentAllocator<uint64_t> > m_delegate_info;
  bip::vector< uint64_t, SegmentAllocator<uint64_t> > m_delegate_degree;
  bip::vector< uint64_t, SegmentAllocator<uint64_t> > m_delegate_label;
  bip::vector< vertex_locator, SegmentAllocator<vertex_locator> >
      m_delegate_targets;

  //Note: BIP only contains a map, not an unordered_map object.
  boost::unordered_map<
      uint64_t, vertex_locator, boost::hash<uint64_t>, std::equal_to<uint64_t>,
      SegmentAllocator< std::pair<uint64_t,vertex_locator> >
     > m_map_delegate_locator;

  bip::vector<vertex_locator, SegmentAllocator<vertex_locator> >
    m_controller_locators;
};  // class delegate_partitioned_graph



} // namespace mpi
} // namespace havoqgt


#include <havoqgt/impl/log_step.hpp>
#include <havoqgt/impl/vert_info.hpp>
#include <havoqgt/impl/edge_data.hpp>
#include <havoqgt/impl/edge_iterator.hpp>
#include <havoqgt/impl/vertex_data.hpp>
#include <havoqgt/impl/vertex_locator.hpp>
#include <havoqgt/impl/vertex_iterator.hpp>

#include <havoqgt/impl/delegate_partitioned_graph.ipp>

#endif //HAVOQGT_MPI_DELEGATE_PARTITIONED_GRAPH_HPP_INCLUDED
