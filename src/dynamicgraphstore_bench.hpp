/*
 * Written by Keita Iwabuchi.
 * LLNL / TokyoTech
 */

#ifndef DYNAMICGRAPHSTORE_BENCH_HPP
#define DYNAMICGRAPHSTORE_BENCH_HPP

#include <iostream>
#include <utility>
#include <algorithm>
#include <functional>

#ifdef __linux__
#ifndef _GNU_SOURCE
  #define _GNU_SOURCE
#endif
  #include <fcntl.h>
#endif

#include <boost/interprocess/managed_mapped_file.hpp>

#include <havoqgt/rmat_edge_generator.hpp>
#include <havoqgt/environment.hpp>
#include <havoqgt/parallel_edge_list_reader.hpp>

#include <havoqgt/graphstore/graphstore_utilities.hpp>
#include <havoqgt/graphstore/graphstore_common.hpp>
#include <havoqgt/graphstore/rhh/rhh_common.hpp>

#include <havoqgt/graphstore/graphstore_rhhda.hpp>
#include <havoqgt/graphstore/graphstore_baseline.hpp>

#define VERBOSE 0

#define DEBUG_MODE 0
#if DEBUG_MODE
std::ofstream ofs_edges;
#endif

#define SORT_BY_CHUNK 0

/// --- typenames --- ///
using mapped_file_type     = boost::interprocess::managed_mapped_file;
using segment_manager_type = boost::interprocess::managed_mapped_file::segment_manager;


template<typename vertex_id_type>
struct EdgeUpdateRequest
{
  EdgeUpdateRequest(std::pair<vertex_id_type, vertex_id_type> _edge, bool _is_delete)
  {
    edge = _edge;
    is_delete = _is_delete;
  }

  std::pair<vertex_id_type, vertex_id_type> edge;
  bool is_delete;
};

template<typename vertex_id_type>
bool edgerequest_asc(const EdgeUpdateRequest<vertex_id_type>& left, const EdgeUpdateRequest<vertex_id_type>& right ) {
  return left.edge.first < right.edge.first;
}
template<typename vertex_id_type>
using request_vector_type = std::vector<EdgeUpdateRequest<vertex_id_type>>;


void print_usages(segment_manager_type *const segment_manager)
{
  const size_t usages = segment_manager->get_size() - segment_manager->get_free_memory();
  std::cout << "Usage: segment size (GiB) =\t"<< static_cast<double>(usages) / (1ULL << 30) << "\n";
  std::cout << "----------------------------" << std::endl;
  size_t mem_unit, totalram, freeram, usedram, bufferram, totalswap, freeswap;
  if (graphstore::utility::get_system_memory_usages(&mem_unit, &totalram, &freeram, &usedram, &bufferram, &totalswap, &freeswap)) {
    std::cout << "Usage: mem_unit:\t" << mem_unit << "\n";
    std::cout << "Usage: totalram(GiB):\t" << static_cast<double>(totalram) / (1<<30ULL) << "\n";
    std::cout << "Usage: freeram(GiB):\t" << static_cast<double>(freeram) / (1<<30ULL) << "\n";
    std::cout << "Usage: usedram(GiB):\t" << static_cast<double>(usedram) / (1<<30ULL) << "\n";
    std::cout << "Usage: bufferram(GiB):\t" << static_cast<double>(bufferram) / (1<<30ULL) << "\n";
    std::cout << "Usage: totalswap(GiB):\t" << static_cast<double>(totalswap) / (1<<30ULL) << "\n";
    std::cout << "Usage: freeswap(GiB):\t" << static_cast<double>(freeswap) / (1<<30ULL) << "\n";
    std::cout << "----------------------------" << std::endl;
  }
  size_t size, resident, share, text, lib, data, dt;
  if (graphstore::utility::get_my_memory_usages(&size, &resident, &share, &text, &lib, &data, &dt)) {
    std::cout << "Usage: VmSize(GiB):\t" << static_cast<double>(size) / (1<<30ULL) << "\n";
    std::cout << "Usage: VmRSS(GiB):\t" << static_cast<double>(resident) / (1<<30ULL) << "\n";
    std::cout << "Usage: sharedpages(GiB):\t" << static_cast<double>(share) / (1<<30ULL) << "\n";
    std::cout << "Usage: text(GiB):\t" << static_cast<double>(text) / (1<<30ULL) << "\n";
    std::cout << "Usage: library(GiB):\t" << static_cast<double>(lib) / (1<<30ULL) << "\n";
    std::cout << "Usage: data+stack(GiB):\t" << static_cast<double>(data) / (1<<30ULL) << "\n";
    std::cout << "Usage: dirtypages(GiB):\t" << static_cast<double>(dt) / (1<<30ULL) << "\n";
    std::cout << "----------------------------" << std::endl;
  }
}


void fallocate(const char* const fname, size_t size, mapped_file_type& asdf)
{
#ifdef __linux__
    int fd  = open(fname, O_RDWR);
    assert(fd != -1);
    /// posix_fallocate dosen't work on XFS ?
    /// (dosen't actually expand the file size ?)
    int ret = fallocate(fd, 0, 0, size);
    assert(ret == 0);
    close(fd);
    asdf.flush();
#else
#warning fallocate() is not supported
#endif
}

template<typename vertex_id_type>
double sort_requests(request_vector_type<vertex_id_type>& requests)
{
  const double time_start1 = MPI_Wtime();
  std::sort(requests.begin(), requests.end(), edgerequest_asc);
  return (MPI_Wtime() - time_start1);
}

template <typename edgelist_itr_type, typename vertex_id_type>
void generate_insertion_requests(edgelist_itr_type& edgelist_itr,
                                 edgelist_itr_type& edgelist_itr_last,
                                 const size_t chunk_size,
                                 request_vector_type<vertex_id_type>& requests,
                                 const size_t delete_ratio)
{
  const int mpi_rank = havoqgt::havoqgt_env()->world_comm().rank();

  assert(requests.size() == 0);
  requests.reserve(chunk_size);

  const double time_start = MPI_Wtime();
  for (size_t cnt = 0; edgelist_itr != edgelist_itr_last && cnt < chunk_size; ++edgelist_itr, ++cnt) {
    const bool is_delete = (rand() % 100 < delete_ratio);
    EdgeUpdateRequest<vertex_id_type> request(*edgelist_itr, is_delete);
    requests.push_back(request);
    // std::cerr << edges_itr->first << " " << edges_itr->second << "\n";
  }
  havoqgt::havoqgt_env()->world_comm().barrier();
  if (mpi_rank == 0) std::cout << "TIME: Generate edges into DRAM (sec.) =\t" << MPI_Wtime() - time_start << std::endl;
  std::cout << mpi_rank << ": Status: # generated insetion requests =\t"<< requests.size() << std::endl;
#if SORT_BY_CHUNK
  const double elapsed_time = sort_requests(requests);
  havoqgt::havoqgt_env()->world_comm().barrier();
  if (mpi_rank == 0) std::cout << "TIME: Sorting chunk (sec.) =\t" << elapsed_time << std::endl;
 #endif
}

void flush_dimmap()
{
  std::cout << "flush di-mmap" << std::endl;
  std::ofstream flusher("/sys/class/di-mmap-runtimeA/flush_buffer");
  flusher << "1" << std::endl;
}

void sync_dimmap()
{
  std::cout << "sync di-mmap" << std::endl;
  std::ofstream flusher("/proc/di-mmap-runtimeA-tuning");
  flusher << "mmap_sync_buffers" << std::endl;
}

void flush_mmmap(mapped_file_type& mapped_file)
{
  std::cout << "flush mmap" << std::endl;
  mapped_file.flush();
}

void sync_mmap()
{
#if _BSD_SOURCE || _XOPEN_SOURCE >= 500
  std::cout << "sync mmap: sync(2)" << std::endl;
  sync();
#else
#warning sync(2) is not supported
#endif
}

void mapped_file_madvice(mapped_file_type& mapped_file)
{
  std::cout << "Call adise_dontneed" << std::endl;
  boost::interprocess::mapped_region::advice_types advise = boost::interprocess::mapped_region::advice_types::advice_dontneed;
  assert(false);
  /// assert(mapped_file.advise(advise));
}

void segment_manager_zero_free_memory(segment_manager_type& segment_manager, mapped_file_type& mapped_file)
{
    std::cout << "Call segment_manager.zero_free_memory()" << std::endl;
    segment_manager.zero_free_memory();
    std::cout << "Call mapped_file.flush()" << std::endl;
    mapped_file.flush();
    std::cout << "Call sync" << std::endl;
    sync_mmap();
}





template <typename vertex_id_type, typename graphstore_type, typename edgelist_type>
void apply_edge_update_requests(mapped_file_type& mapped_file,
                                segment_manager_type *const segment_manager,
                                graphstore_type& graph_store,
                                edgelist_type& edges,
                                const uint64_t chunk_size,
                                const uint64_t edges_delete_ratio)
{
  int mpi_rank = havoqgt::havoqgt_env()->world_comm().rank();
  int mpi_size = havoqgt::havoqgt_env()->world_comm().size();
  havoqgt::havoqgt_env()->world_comm().barrier();

  if (mpi_rank == 0) std::cout << "-- Disp status of before generation --" << std::endl;
  if (mpi_rank == 0) print_usages(segment_manager);
  havoqgt::havoqgt_env()->world_comm().barrier();

  uint64_t loop_cnt = 0;
  auto edges_itr = edges.begin();
  auto edges_itr_end = edges.end();
  bool global_is_finished = false;
  request_vector_type<vertex_id_type> update_request_vec = request_vector_type<vertex_id_type>();

  size_t count_inserted = 0;
  size_t count_delete = 0;

  while (!global_is_finished) {
    if (mpi_rank == 0) std::cout << "\n[" << loop_cnt << "] : chunk_size =\t" << chunk_size << std::endl;

    update_request_vec.clear();
    generate_insertion_requests(edges_itr, edges_itr_end, chunk_size, update_request_vec, edges_delete_ratio);
    havoqgt::havoqgt_env()->world_comm().barrier();

    const double time_start = MPI_Wtime();
    unsigned char dummy = 0;
    for (auto request : update_request_vec) {
      auto edge = request.edge;
      if (request.is_delete) {
#if DEBUG_MODE
        ofs_edges << edge.first << " " << edge.second << " 1" << "\n";
#endif
        count_delete += graph_store.erase_edge(edge.first, edge.second);
      } else {
#if DEBUG_MODE
        ofs_edges << edge.first << " " << edge.second << " 0" << "\n";
#endif
        count_inserted += graph_store.insert_edge(edge.first, edge.second, dummy);
      }
    }
//    std::cout << "shrink to fit low table" << std::endl;
//    graph_store.shrink_to_fit_low_table();

//    std::cout << "rehash low table" << std::endl;
//    graph_store.opt();

    sync_mmap();
    havoqgt::havoqgt_env()->world_comm().barrier();

    const double time_end = MPI_Wtime();
    if (mpi_rank == 0) std::cout << "TIME: Execution time (sec.) =\t" << (time_end - time_start) << std::endl;
    if (mpi_rank == 0) print_usages(segment_manager);
    havoqgt::havoqgt_env()->world_comm().barrier();
#if VERBOSE
    for (int i = 0; i < mpi_size; ++i) {
      if (i == mpi_rank) {
        std::cout << "[" << mpi_rank << "]" << std::endl;
        graph_store.print_status();
      }
      havoqgt::havoqgt_env()->world_comm().barrier();
    }
#endif

    ++loop_cnt;

    const bool local_is_finished = (edges_itr == edges_itr_end);
    MPI_Allreduce(&local_is_finished, &global_is_finished, 1, MPI_C_BOOL, MPI_LAND, MPI_COMM_WORLD);
  }
  havoqgt::havoqgt_env()->world_comm().barrier();
  if (mpi_rank == 0) {
    std::cout << "\n-- All edge updations done --" << std::endl;
    print_usages(segment_manager);
  }
  havoqgt::havoqgt_env()->world_comm().barrier();
  for (int i = 0; i < mpi_size; ++i) {
    if (i == mpi_rank) {
      std::cout << "[" << mpi_rank << "] inserted edges : " << count_inserted << std::endl;
      std::cout << "[" << mpi_rank << "] deleted edges : " << count_delete << std::endl;
    }
  }

}



#endif // DYNAMICGRAPHSTORE_BENCH_HPP

