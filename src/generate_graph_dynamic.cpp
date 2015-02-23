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

#include <havoqgt/construct_dynamicgraph.hpp>
#include <havoqgt/rmat_edge_generator.hpp>
#include <havoqgt/upper_triangle_edge_generator.hpp>
#include <havoqgt/gen_preferential_attachment_edge_list.hpp>
#include <havoqgt/environment.hpp>
#include <iostream>
#include <assert.h>
#include <deque>
#include <utility>
#include <algorithm>
#include <functional>
#include <boost/interprocess/managed_mapped_file.hpp>
#include <boost/interprocess/allocators/allocator.hpp>

#include <sys/sysinfo.h>

#define SORT_BY_CHUNK 0

namespace hmpi = havoqgt::mpi;
using namespace havoqgt::mpi;
typedef bip::managed_mapped_file mapped_t;
typedef mapped_t::segment_manager segment_manager_t;
typedef hmpi::construct_dynamicgraph<segment_manager_t> graph_type;
typedef boost::container::vector<EdgeUpdateRequest> request_vector_type;

void print_usages(segment_manager_t *const segment_manager)
{
  const size_t usages = segment_manager->get_size() - segment_manager->get_free_memory();
  std::cout << "Usage: segment size =\t"<< usages << std::endl;
  struct sysinfo info;
  sysinfo(&info);
  std::cout << "Usage: mem_unit:\t" << info.mem_unit << std::endl;
  std::cout << "Usage: totalram(GiB):\t" << (double)info.totalram / (1<<30ULL) << std::endl;
  std::cout << "Usage: freeram(GiB):\t" << (double)info.freeram / (1<<30ULL) << std::endl;
  std::cout << "Usage: usedram(GiB):\t" << (double)(info.totalram - info.freeram) / (1<<30ULL) << std::endl;
  std::cout << "Usage: bufferram(GiB):\t" << (double)info.bufferram / (1<<30ULL) << std::endl;
  std::cout << "Usage: totalswap(GiB):\t" << (double)info.totalswap / (1<<30ULL) << std::endl;
  std::cout << "Usage: freeswap(GiB):\t" << (double)info.freeswap / (1<<30ULL) << std::endl;
}

double sort_requests(request_vector_type& requests)
{
  const double time_start1 = MPI_Wtime();
  std::sort(requests.begin(), requests.end(), edgerequest_asc);
  return (MPI_Wtime() - time_start1);
}

template <typename Edges_itr>
void generate_insertion_requests(Edges_itr& edges_itr, const uint64_t chunk_size, request_vector_type& requests)
{
  assert(requests.size() == 0);
  requests.reserve(chunk_size);
  const double time_start = MPI_Wtime();
  for (uint64_t i = 0; i < chunk_size; ++i, ++edges_itr) {
    EdgeUpdateRequest request(*edges_itr, false);
    requests.push_back(request);
  }
  std::cout << "TIME: Generate edges into DRAM (sec.) =\t" << MPI_Wtime() - time_start << std::endl;
  std::cout << "Status: # insetion requests =\t"<< requests.size() << std::endl;
#if SORT_BY_CHUNK
  const double elapsed_time = sort_requests(requests);
  std::cout << "TIME: Sorting chunk (sec.) =\t" << elapsed_time << std::endl;
 #endif
}

template <typename Edges>
void apply_edges_update_requests(graph_type *const graph, Edges& edges, segment_manager_t *const segment_manager, const uint64_t chunk_size)
{
  std::cout << "-- Disp status of before generation --" << std::endl;
  print_usages(segment_manager);

  const uint64_t num_edges = edges.size();
  const uint64_t num_requests = num_edges;
  const uint64_t num_loops = num_requests / chunk_size;
  auto edges_itr = edges.begin();
  for (uint64_t i = 0; i < num_loops; ++i) {
    std::cout << "[" << i+1 << " / " << num_loops << "] : chunk_size =\t" << chunk_size << std::endl;
    request_vector_type update_request_vec = request_vector_type();
    generate_insertion_requests(edges_itr, chunk_size, update_request_vec);
    graph->add_edges_adjacency_matrix(update_request_vec.begin(), chunk_size);
    print_usages(segment_manager);
  }
  graph->print_profile();
}


int main(int argc, char** argv) {


  CHK_MPI(MPI_Init(&argc, &argv));
  {
    int mpi_rank(0), mpi_size(0);
    CHK_MPI( MPI_Comm_rank( MPI_COMM_WORLD, &mpi_rank) );
    CHK_MPI( MPI_Comm_size( MPI_COMM_WORLD, &mpi_size) );
    havoqgt::get_environment();

    if (mpi_rank == 0) {

      std::cout << "MPI initialized with " << mpi_size << " ranks." << std::endl;
      std::cout << "CMD line:";
      for (int i=0; i<argc; ++i) {
        std::cout << " " << argv[i];
      }
      std::cout << std::endl;
      havoqgt::get_environment().print();
    }
    MPI_Barrier(MPI_COMM_WORLD);

    uint64_t num_vertices = 1;
    uint64_t vert_scale;
    uint64_t edge_factor;
    double   pa_beta;
    uint64_t hub_threshold;
    uint32_t load_from_disk;
    uint32_t delete_file;
    uint64_t chunk_size_exp;
    uint64_t low_degree_threshold;
    uint64_t edges_delete_ratio = 0;
    std::string type;
    std::string fname_output;
    std::string fname_compare = "";
    std::string data_structure_type;

    if (argc < 13) {
      std::cerr << "usage: <RMAT/PA> <Scale> <Edge factor> <PA-beta> <hub_threshold> <file name>"
      << " <load_from_disk> <delete file on exit>"
      << " <chunk_size_exp> <VC_VC/MP_VC/RB_HS/RB_MP/RB_MX> <low_degree_threshold>"
      << " <file to compare to>"
      << " (argc:" << argc << " )." << std::endl;
      exit(-1);
    } else {
      int pos = 1;
      type = argv[pos++];
      vert_scale      = boost::lexical_cast<uint64_t>(argv[pos++]);
      edge_factor     = boost::lexical_cast<uint64_t>(argv[pos++]);
      pa_beta         = boost::lexical_cast<double>(argv[pos++]);
      hub_threshold   = boost::lexical_cast<uint64_t>(argv[pos++]);
      fname_output    = argv[pos++];
      delete_file     = boost::lexical_cast<uint32_t>(argv[pos++]);
      load_from_disk  = boost::lexical_cast<uint32_t>(argv[pos++]);
      chunk_size_exp  = boost::lexical_cast<uint64_t>(argv[pos++]);
      data_structure_type = argv[pos++];
      low_degree_threshold = boost::lexical_cast<uint64_t>(argv[pos++]);
      edges_delete_ratio = boost::lexical_cast<uint64_t>(argv[pos++]);
      if (pos < argc) {
        fname_compare = argv[pos++];
      }
    }
    num_vertices <<= vert_scale;

    if (mpi_rank == 0) {
      std::cout << "Building graph type: " << type << std::endl;
      std::cout << "Building graph Scale: " << vert_scale << std::endl;
      std::cout << "Building graph Edge factor: " << edge_factor << std::endl;
      std::cout << "Hub threshold = " << hub_threshold << std::endl;
      std::cout << "PA-beta = " << pa_beta << std::endl;
      std::cout << "File name = " << fname_output << std::endl;
      std::cout << "Load from disk = " << load_from_disk << std::endl;
      std::cout << "Delete on Exit = " << delete_file << std::endl;
      std::cout << "Chunk size exp = " << chunk_size_exp << std::endl;
      std::cout << "Data structure type: " << data_structure_type << std::endl;
      std::cout << "Low Degree Threshold = " << low_degree_threshold << std::endl;
      std::cout << "Edges Delete Ratio = " << edges_delete_ratio << std::endl;
      if (fname_compare != "") {
        std::cout << "Comparing graph to " << fname_compare << std::endl;
      }
    }

    std::stringstream fname;
    fname << fname_output << "_" << mpi_rank;

    if (load_from_disk  == 0) {
      remove(fname.str().c_str());
    }


    std::cout << "\n<<Construct segment>>" << std::endl;
    uint64_t graph_capacity = std::pow(2, 39);
    mapped_t  asdf(bip::open_or_create, fname.str().c_str(), graph_capacity);
#if 0
    boost::interprocess::mapped_region::advice_types advise = boost::interprocess::mapped_region::advice_types::advice_random;
    assert(asdf.advise(advise));
    std::cout << "Calling adise_randam" << std::endl;
#endif
    int fd  = open(fname.str().c_str(), O_RDWR);
    assert(fd != -1);
    int ret = posix_fallocate(fd,0,graph_capacity);
    assert(ret == 0);
    close(fd);
    asdf.flush();
    segment_manager_t* segment_manager = asdf.get_segment_manager();
    bip::allocator<void,segment_manager_t> alloc_inst(segment_manager);
    print_usages(segment_manager);

    graph_type *graph;
    if (data_structure_type == "VC_VC") {
      graph = segment_manager->construct<graph_type>
      ("graph_obj")
      (asdf, alloc_inst, graph_type::kUseVecVecMatrix, low_degree_threshold);
    } else if (data_structure_type == "MP_VC") {
      graph = segment_manager->construct<graph_type>
      ("graph_obj")
      (asdf, alloc_inst, graph_type::kUseMapVecMatrix, low_degree_threshold);
    } else if (data_structure_type == "RH_AR") {
      graph = segment_manager->construct<graph_type>
      ("graph_obj")
      (asdf, alloc_inst, graph_type::kUseRHHAsArray, low_degree_threshold);
    } if (data_structure_type == "RH_MX") {
      graph = segment_manager->construct<graph_type>
      ("graph_obj")
      (asdf, alloc_inst, graph_type::kUseRHHAsMatrix, low_degree_threshold);
    } else if (data_structure_type == "HY_DA") {
      graph = segment_manager->construct<graph_type>
      ("graph_obj")
      (asdf, alloc_inst, graph_type::kUseHybridDegreeAwareModel, low_degree_threshold);
    } else {
      std::cerr << "Unknown data structure type: " << data_structure_type << std::endl;
      exit(-1);
    }


    std::cout << "\n<<Update edges>>" << std::endl;
    if(type == "UPTRI") {
      uint64_t num_edges = num_vertices * edge_factor;
      havoqgt::upper_triangle_edge_generator uptri(num_edges, mpi_rank, mpi_size,
       false);

      apply_edges_update_requests(graph,
                                  uptri,
                                  segment_manager,
                                  static_cast<uint64_t>(std::pow(2, chunk_size_exp)));

    } else if(type == "RMAT") {
      uint64_t num_edges_per_rank = num_vertices * edge_factor / mpi_size;

      havoqgt::rmat_edge_generator rmat(uint64_t(5489) + uint64_t(mpi_rank) * 3ULL,
        vert_scale, num_edges_per_rank,
        0.57, 0.19, 0.19, 0.05, true, false);

      apply_edges_update_requests(graph,
                                  rmat,
                                  segment_manager,
                                  static_cast<uint64_t>(std::pow(2, chunk_size_exp)));

    } else if(type == "PA") {
      std::vector< std::pair<uint64_t, uint64_t> > input_edges;
      gen_preferential_attachment_edge_list(input_edges, uint64_t(5489), vert_scale, vert_scale+std::log2(edge_factor), pa_beta, 0.0, MPI_COMM_WORLD);

      apply_edges_update_requests(graph,
                                  input_edges,
                                  segment_manager,
                                  static_cast<uint64_t>(std::pow(2, chunk_size_exp)));

      {
        std::vector< std::pair<uint64_t, uint64_t> > empty(0);
        input_edges.swap(empty);
      }

    } else {
      std::cerr << "Unknown graph type: " << type << std::endl;  exit(-1);
    }
    MPI_Barrier(MPI_COMM_WORLD);


    std::cout << "\n<<Profile>>" << std::endl;
    for (int i = 0; i < mpi_size; i++) {
      if (i == mpi_rank) {

        print_usages(segment_manager);

        size_t usages = segment_manager->get_size() - segment_manager->get_free_memory();
        double percent = double(segment_manager->get_free_memory()) / double(segment_manager->get_size());
        std::cout << "[" << mpi_rank << "] "
        << (double)usages / (1<<30ULL) << " "
        << (double)segment_manager->get_free_memory()  / (1<<30ULL)
        << "/" << (double)segment_manager->get_size() / (1<<30ULL)
        << " = " << percent << std::endl;
      }
      MPI_Barrier(MPI_COMM_WORLD);
    }


    std::cout << "\n<<Delete segment and files>>" << std::endl;
    /// this function call graph_type's destructor
    segment_manager->destroy<graph_type>("graph_obj");

    if (delete_file) {
      std::cout << "Deleting Mapped File." << std::endl;
      bip::file_mapping::remove(fname.str().c_str());
    }

  } //END Main MPI
  CHK_MPI(MPI_Barrier(MPI_COMM_WORLD));

  std::cout << "Before MPI_Finalize." << std::endl;
  CHK_MPI(MPI_Finalize());
  std::cout << "FIN." << std::endl;

  return 0;
}
