//
// Created by Iwabuchi, Keita @ LLNL on 7/18/17.
//
/// Build: g++ -O3 -std=c++11 extract_graph_from_wikipedia_graph.cpp -o ./extract_graph_from_wikipedia_graph
/// Run: ./extract_graph_from_wikipedia_graph edge_list_output_file_prefix time_stamp original_id_list_output_file_name input_file_names
///      input files are in /p/lscratchf/havoqgtu/wikipedia-enwiki-20151201/output/task_XX/n_edgelist_with_id. XX is a number
///      Example: ./extract_graph_from_wikipedia_graph  /l/ssd/edge_list 0 /l/ssd/original_id_list /p/lscratchf/havoqgtu/wikipedia-enwiki-20151201/output/task_*/n_edgelist_with_id
/// Outputs:
///      edge_list_output_file: each line is a pair of source and target vertex's sequential IDs
///      original_id_list_output_file: each line is an original ID sorted by its sequential ID with ascending order

#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <algorithm>

uint64_t convert_to_sequential_id(std::unordered_map<uint64_t, uint64_t> &id_map, uint64_t &next_sequential_id,
                                  const uint64_t id)
{
  if (id_map.count(id) == 0) {
    id_map[id] = next_sequential_id++;
  }
  return id_map[id];
}

bool sort_second_key_ascending_order(const std::pair<uint64_t, uint64_t> &i, const std::pair<uint64_t, uint64_t> &j)
{
  return (i.second < j.second);
}

int main(int argc, char *argv[])
{
  /// ----- parse command line arguments ----- ///
  const std::string edge_list_output_file_prefix = argv[1];
  const uint64_t time_stamp = std::atoll(argv[2]);
  const std::string original_id_list_output_file_name(argv[3]);

  std::vector<std::string> input_file_name_list;
  for (int i = 4; i < argc; ++i) {
    input_file_name_list.push_back(std::string(argv[i]));
    std::cout << argv[i] << " ";
  }
  std::cout << std::endl;


  /// ----- extract edges and convert original IDs to sequential (zero-based) IDs ----- ///
  std::unordered_map<uint64_t, uint64_t> id_table; /// key: original ID, value: sequential ID
  {
    uint64_t next_sequential_id(0);
    int file_count(0);
    for (const std::string input_file_name : input_file_name_list) {
      std::ifstream fin(input_file_name);

      const std::string fout_name(edge_list_output_file_prefix + "_" + std::to_string(file_count) + "_of_" +
                                  std::to_string(input_file_name_list.size()));
      std::ofstream fout(fout_name);
      std::cout << "Extract edges from " << input_file_name << " and dump into " << fout_name << std::endl;

      uint64_t original_src_id;
      uint64_t original_trg_id;
      uint64_t hashed_src_name;
      uint64_t hashed_trg_name;
      uint64_t time_added;
      uint64_t time_deleted;
      std::string line;
      while (std::getline(fin, line)) {
        std::stringstream ss(line);
        ss >> original_src_id >> original_trg_id >> hashed_src_name >> hashed_trg_name >> time_added >> time_deleted;

        const uint64_t sequential_src_id(convert_to_sequential_id(id_table, next_sequential_id, original_src_id));
        const uint64_t sequential_trg_id(convert_to_sequential_id(id_table, next_sequential_id, original_trg_id));

        /// dump the edge if it matches to at least one of the following conditions
        /// 1) the given time stamp is equal to 0 (time_stamp == 0)
        /// 2) it added before the time stamp (time_added <= time_stamp ) and deleted after the time stamp (time_deleted > time_stamp) or never deleted (time_deleted == 0)
        if (time_stamp == 0 || (time_added <= time_stamp && (time_stamp <= time_deleted || time_deleted == 0))) {
          fout << sequential_src_id << " " << sequential_trg_id << "\n";
        }
      } /// Loop over all lines in an input file
      fout.close();
      ++file_count;
    } /// Loop over all input files
  }


  /// ----- Dump the id_table into a file ----- ///
  /// \brief Dump the original IDs. The IDs are sorted by their sequential_id
  {
    /// ----- sort the vector with second key (sequential_id) ----- ///
    std::cout << "Sort IDs"<< std::endl;
    std::vector<std::pair<uint64_t, uint64_t>> id_pair_vector;
    for (const auto itr : id_table) {
      const uint64_t original_id(itr.first);
      const uint64_t sequential_id(itr.second);
      id_pair_vector.push_back(std::make_pair(original_id, sequential_id));
    }
    std::sort(id_pair_vector.begin(), id_pair_vector.end(), sort_second_key_ascending_order);

    /// ----- Dump original IDs ----- ///
    std::cout << "Dump original IDs into " << original_id_list_output_file_name << std::endl;
    std::ofstream fout(original_id_list_output_file_name);
    for (const auto itr : id_pair_vector) {
      const uint64_t original_id(itr.first);
      fout << original_id << "\n";
    }
    fout.close();
  }

  std::cout << "Done" << std::endl;

  return 0;
}