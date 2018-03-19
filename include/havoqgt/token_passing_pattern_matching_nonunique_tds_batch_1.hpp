#ifndef HAVOQGT_TOKEN_PASSING_PATTERN_MATCHING_NONUNIQUE_TDS_BATCH_1_HPP_INCLUDED
#define HAVOQGT_TOKEN_PASSING_PATTERN_MATCHING_NONUNIQUE_TDS_BATCH_1_HPP_INCLUDED

#include <array>
#include <deque>
#include <limits>

#include <havoqgt/visitor_queue.hpp>
#include <havoqgt/detail/visitor_priority_queue.hpp>

static bool enable_vertex_token_source_cache = false;
static bool path_checking_filter = false;
static uint64_t visitor_count;

namespace havoqgt { namespace mpi {

template<typename Visitor>
class tppm_queue_tds {

public:
  tppm_queue_tds() {}

  bool push(Visitor const& element) {
    data.push_back(element);
    return true;
  }

  void pop() {
    //data.pop_back();
    data.pop_front();
  }

  Visitor const& top() {
    //return data.back();
    return data.front();
  }

  size_t size() const {
    return data.size();;
  }

  bool empty() const {
    return data.empty();
  }

  void clear() {
    data.clear();
  }

protected:
  //std::vector<Visitor> data;
  std::deque<Visitor> data;
};

// token passing pattern matching visitor class
template<typename Graph, typename Vertex, typename BitSet>
class tppm_visitor_tds {

public:
  typedef typename Graph::vertex_locator vertex_locator;
  typedef typename Graph::edge_iterator eitr_type;

  tppm_visitor_tds() : 
    itr_count(0), 
    do_pass_token(false),
    is_init_step(true),
    ack_success(false),
    source_index_pattern_indices(0), 
    parent_pattern_index(0) {
      visited_vertices[itr_count] = vertex;  
    }

  tppm_visitor_tds(vertex_locator _vertex) :  
    vertex(_vertex), 
    itr_count(0),
    do_pass_token(false), 
    is_init_step(true),
    ack_success(false),
    source_index_pattern_indices(0), 
    parent_pattern_index(0) {
      visited_vertices[itr_count] = vertex;  
    }
  
  template <typename VertexLocatorArrayStatic> 
  tppm_visitor_tds(vertex_locator _vertex,
    vertex_locator _parent,  
    vertex_locator _target_vertex, 
    VertexLocatorArrayStatic& _visited_vertices, 
    size_t _itr_count, 
    size_t _max_itr_count, 
    size_t _source_index_pattern_indices, 
    size_t _parent_pattern_index, 
    bool _expect_target_vertex = true, 
    bool _do_pass_token = true, 
    bool _is_init_step = false, 
    bool _ack_success = false) : 
    vertex(_vertex),
    parent(_parent),
    target_vertex(_target_vertex), 
    itr_count(_itr_count), 
    max_itr_count(_max_itr_count), 
    expect_target_vertex(_expect_target_vertex), 
    do_pass_token(_do_pass_token), 
    is_init_step(_is_init_step),
    ack_success(_ack_success), 
    source_index_pattern_indices(_source_index_pattern_indices), 
    parent_pattern_index(_parent_pattern_index) {
      //if (itr_count == 0 && !_ack_success) { // probably never gets here
      //  visited_vertices[itr_count] = vertex;  
      //} else if (itr_count > 0 && itr_count <= max_itr_count && !_ack_success) {
      if (!_ack_success) {
        // copy to visited_vertices from the one received from the parent
        std::copy(std::begin(_visited_vertices), 
        std::end(_visited_vertices), // TODO: std::begin(_visited_vertices) + itr_count ? 
        std::begin(visited_vertices));  
        visited_vertices[itr_count + 1] = vertex; // Important : itr_count for the next hop
        // when read from visited_vertices use    
      }
    }  

  template<typename AlgData>
  bool pre_visit(AlgData& alg_data) const {
    visitor_count++;
    if(!std::get<13>(alg_data)[vertex]) {
      return false;
    }

    auto vertex_data = std::get<0>(alg_data)[vertex];
    auto& pattern = std::get<1>(alg_data);
    auto& pattern_indices = std::get<2>(alg_data);
    //auto& pattern_graph = std::get<4>(alg_data);
    auto g = std::get<11>(alg_data); // graph
    // std::get<12>(alg_data); // vertex_token_source_set   
    // std::get<13>(alg_data); // vertex_active
    // std::get<14>(alg_data); // template_vertices
    // std::get<15>(alg_data); // vertex_active_edges_map
    // std::get<16>(alg_data); // pattern_selected_vertices
    // std::get<17>(alg_data); // paths_result_file
    // std::get<18>(alg_data); // pattern_enumeration_indices 
     
    int mpi_rank = havoqgt_env()->world_comm().rank();

    // Test
    //if (mpi_rank == 320 && visitor_count > 10000) { 
      //std::cout << "MPI rank : " << mpi_rank << " pre-visit count : " 
      //  << visitor_count << std::endl;        
      //visitor_count = 0;
    //} 
    // Test

    if (ack_success) {
      // TODO: if vertex == target_vertex
      // it is in the token_source map
      // !bitset.none
      // if delegate, return true otherwise false
      return true; // Important : must return true to handle delegates 
    } 
   
    if(enable_vertex_token_source_cache) { 
    // verify if this vertex have already forwarded a token from the originating vertex
    if (!is_init_step && max_itr_count > itr_count) {
      auto find_token_source_forwarded = std::get<12>(alg_data)[vertex].find(g->locator_to_label(target_vertex));
      if (find_token_source_forwarded != std::get<12>(alg_data)[vertex].end()) {
        return false;
      }
    }
    } //enable_vertex_token_source_cache

/* 
    // TODO: use vertex_active
    // TODO: only start token passing from the controller 
    auto find_vertex = std::get<5>(alg_data).find(g->locator_to_label(vertex));
    if (find_vertex == std::get<5>(alg_data).end()) {    
      return false;
    }
*/    
    if (!do_pass_token && is_init_step && itr_count == 0) { // probably it never gets here
      // TODO: this is probbaly wrong
//      if (vertex.is_delegate() && (g->master(vertex) != mpi_rank)) {
//        return false;
//      }

//      if (!(find_vertex->second.vertex_pattern_index == pattern_indices[0] && vertex_data == pattern[0])) {
      if(vertex_data == pattern[0]) { 
        return true;
      } else {
        return false; 
      } 
    } else if (!is_init_step) { // relay token
       // target_vertex cannot relay the token
       //if (do_pass_token && (max_itr_count > itr_count) &&
       //  (g->locator_to_label(vertex) == g->locator_to_label(target_vertex))) {
       //  return false;
       //}  

       auto new_itr_count = itr_count + 1;
       auto next_pattern_index = source_index_pattern_indices + new_itr_count; // expected next pattern_index
       // TODO: notice next_pattern_index is redundent now, same as new_itr_count; just use new_itr_count       
       auto vertex_pattern_index = 0; //find_vertex->second.vertex_pattern_index;

       // TODO: read vertex_pattern_index from template_vertices
       // vertex_data == pattern[next_pattern_index] and vertex_pattern_index == pattern_indices[next_pattern_index] must hold  
       // otherwise return false
       // verify vertex data  
       // verify if received from a valid parent
       if (vertex_data != pattern[next_pattern_index] && 
         parent_pattern_index != pattern_indices[next_pattern_index - 1]) {
         return false;
       }

       bool match_found = false; 
       
       BitSet vertex_template_vertices(std::get<14>(alg_data)[vertex]); // template_vertices
  
       if (vertex_template_vertices.none()) {
         return false;  
       }
 
       // TODO: vertex_template_vertices.test(pattern_indices[next_pattern_index])
       for (size_t i = 0; i < vertex_template_vertices.size(); i++) {  
         if (vertex_template_vertices.test(i)) {
           if (i == pattern_indices[next_pattern_index]) {
             match_found = true;
             vertex_pattern_index = i; 
             break;  
           }    
         }   
       }
 
       if (!match_found) {
         return false;
       }    
   
/*//--       if (vertex.is_delegate() && g->master(vertex) != mpi_rank) { // delegate but not the controller
         if (vertex_data == pattern[next_pattern_index]) {
           vertex_pattern_index = pattern_indices[next_pattern_index];
         } else {
           return false;
         } 
//--       } else {
//--         auto find_vertex = std::get<5>(alg_data).find(g->locator_to_label(vertex));
//--         if (find_vertex == std::get<5>(alg_data).end()) {
//--           return false;
//--         }
//--         vertex_pattern_index = find_vertex->second.vertex_pattern_index;
         
//--       }*/

       // verify if received from a valid parent // TODO: remove redundent checks
       if (vertex_data == pattern[next_pattern_index] &&
         vertex_pattern_index == pattern_indices[next_pattern_index]) {
         if (vertex_data == pattern[next_pattern_index] && 
           parent_pattern_index == pattern_indices[next_pattern_index - 1]) { 

//+           if (vertex.is_delegate() && g->master(vertex) != mpi_rank) { // delegate but not the controller
//+             return true;
//+           }  

           if (do_pass_token && (max_itr_count > itr_count)) {
             // OK to forwarded a token from a source, now update vertex_token_source_set
             
             if (enable_vertex_token_source_cache) { 
             auto find_token_source_forwarded = std::get<12>(alg_data)[vertex].find(g->locator_to_label(target_vertex));
             if (find_token_source_forwarded == std::get<12>(alg_data)[vertex].end()) {
               auto insert_status = std::get<12>(alg_data)[vertex].insert(g->locator_to_label(target_vertex));
               if(!insert_status.second) {
                 std::cerr << "Error: failed to add an element to the set." << std::endl;
                 return false;
               }
               // std::cout << g.locator_to_label(vertex) << " adding " << g.locator_to_label(target_vertex)
	       //  << " to the vertex set" << std::endl; // Test     
	     } else {
               std::cerr << "Error: unexpected item in the set." << std::endl;
               return false;
             }
             } // enable_vertex_token_source_cache

             // TDS 
             //std::get<18>(alg_data); // pattern_enumeration_indices
             //std::cout << g->locator_to_label(vertex) << " pattern_enumeration_indices " 
             //  << new_itr_count << ", " 
             //  << std::get<18>(alg_data)[new_itr_count] << std::endl; // Test
             if (std::get<18>(alg_data)[new_itr_count] == new_itr_count) {
               // duplicate is not expected in visited_vertices  
               for (size_t i = 0; i < new_itr_count; i++) { // Important : must be < not <=
                 if (g->locator_to_label(visited_vertices[i]) == g->locator_to_label(vertex)) {
                   //break;
                   //std::cout << g->locator_to_label(vertex) << " returning false from pre_visit ... " << std::endl; // Test 
                   return false;
                 }
               }  
             } else if (std::get<18>(alg_data)[new_itr_count] < new_itr_count) {
               // vertex is expected in visited_vertices
               if (g->locator_to_label(visited_vertices[std::get<18>(alg_data)[new_itr_count]]) 
                 != g->locator_to_label(vertex)) {
                 return false;
               }
             } else {
               std::cerr << "Error: invalid value A." << std::endl;
               return false;
             }          

           } 

           //TODO: max_itr_count == itr_count
          
           // TDS checks 
           // duplicate is not expected in visited_vertices
           // verify if vertex was already visited
           //if (g.locator_to_label(visited_vertices[new_itr_count]) == g.locator_to_label(vertex)) {
           // return false;
           //}
           //for (size_t i = 0; i < new_itr_count; i++) {
           //  if (g->locator_to_label(visited_vertices[i]) == g->locator_to_label(vertex)) {
           //    break;
           //    return false;
           //  }
           //}   
           //
           //std::cout << g->locator_to_label(vertex) << " returning true from pre_visit ... " << std::endl; // Test 

           return true; // delegate forwarding to the controller 
         } else {
           return false;
         }    
       } else {
         return false;
       }  
    } else {
      return false;  
    }
//++    return true;
    return false; // Test
  }

  template<typename VisitorQueueHandle, typename AlgData>
  bool init_visit(Graph& g, VisitorQueueHandle vis_queue,
    AlgData& alg_data) const {
    if(!std::get<13>(alg_data)[vertex]) {
      return false;
    } else { 
      return visit(g, vis_queue, alg_data);
    }
  }

  template<typename VisitorQueueHandle, typename AlgData>
  bool visit(Graph& g, VisitorQueueHandle vis_queue, AlgData& alg_data) const {
    //visitor_count++;
    if(!std::get<13>(alg_data)[vertex]) {
      return false;
    }

    int mpi_rank = havoqgt_env()->world_comm().rank();

    if (vertex.is_delegate() && (g.master(vertex) != mpi_rank)) {  
      // Important : it should never get here   
      std::cerr << "Error: Controller forwarded visitor to delegates." << std::endl;
      return false;
    }  

    if (ack_success) {
      // token source receiving ack_success  
      auto find_token_source = std::get<6>(alg_data).find(g.locator_to_label(vertex)); // token_source_map
      if (find_token_source == std::get<6>(alg_data).end()) {
        std::cerr << "Error: did not find the expected item in the map." << std::endl;
        //return true; // false ?           
        return false;
      }
      find_token_source->second = 1; //true;   
//    }
 
      std::get<9>(alg_data) = 1; // true; // pattern_found

      //std::cerr << "ACK_SUCCESS " << g.locator_to_label(vertex) << " " << itr_count << std::endl;	// Test
         
//++      return true; // Important : must return true to handle delegates 
      return false;
    } 

    // if vertex is a delegate  
    if (!is_init_step && vertex.is_delegate() && (g.master(vertex) != mpi_rank)) { 
      // verify if this vertex has already forwarded a token from the originating vertex
      
      if (enable_vertex_token_source_cache) {        
      if (!is_init_step && max_itr_count > itr_count) {
        auto find_token_source_forwarded = std::get<12>(alg_data)[vertex].find(g.locator_to_label(target_vertex));
        if (find_token_source_forwarded != std::get<12>(alg_data)[vertex].end()) {
          return false; 
        } 		
      }
      } // enable_vertex_token_source_cache

    }

    /*
    // TODO: verify if this vertex is alive
    auto find_vertex = std::get<5>(alg_data).find(g.locator_to_label(vertex));
    if (find_vertex == std::get<5>(alg_data).end()) {
      return false;
    } //else { // Test
      //std::cout << find_vertex->first << " " << find_vertex->second.vertex_pattern_index 
      //<< " " << std::get<0>(alg_data)[vertex] << std::endl;
    //} // Test
    */
    
    auto vertex_data = std::get<0>(alg_data)[vertex];
    auto& pattern = std::get<1>(alg_data);
    auto& pattern_indices = std::get<2>(alg_data);   
    //auto& pattern_graph = std::get<4>(alg_data);

    auto pattern_cycle_length = std::get<7>(alg_data);   
    auto pattern_valid_cycle = std::get<8>(alg_data);
    //auto& pattern_found = std::get<9>(alg_data);
    //auto& edge_metadata = std::get<10>(alg_data); 
    // std::get<11>(alg_data) // graph
    // std::get<12>(alg_data); // vertex_token_source_set
    // std::get<13>(alg_data); // vertex_active 
    // std::get<14>(alg_data); // template_vertices
    // std::get<15>(alg_data); // vertex_active_edges_map
    // std::get<16>(alg_data); // pattern_selected_vertices
    // std::get<17>(alg_data); // paths_result_file
    // std::get<18>(alg_data); // pattern_enumeration_indices 

    if (!do_pass_token && is_init_step && itr_count == 0) {
      // create visitors only for the token source vertices
       
      // Test 
//      for(eitr_type eitr = g.edges_begin(vertex); eitr != g.edges_end(vertex); ++eitr) {
//        vertex_locator neighbor = eitr.target();
//        if (g.locator_to_label(neighbor) != 46) { // TODO: put all the valid vertices form label propagation in a queue and pop here and create visitors like this 
//          continue;    
//        }  
        // 65 66 67 66 67 69 68 66 67 66 67 : 0 1 2 3 4 5 6 3 4 1 2 
//        tppm_visitor new_visitor(neighbor, g.label_to_locator(46), 0, 7, 1, pattern_indices[1], true, true, true); 
//        vis_queue->queue_visitor(new_visitor);              
//      }
      // Test
       
      // TODO: this is probbaly wrong
      // for delegate vertices only start token passing from the controller   
//      if (vertex.is_delegate() && (g.master(vertex) != mpi_rank)) {
//        return false;
//      }

      //if (!(find_vertex->second.vertex_pattern_index == pattern_indices[0] && vertex_data == pattern[0])) {
      if (vertex_data != pattern[0]) { 
        return false;  
      }

      //std::cout << g.locator_to_label(vertex) << " Invoking visitor." << std::endl; // Test 

      // pattern_selected_vertices and vertex_token_source_set 
      if (std::get<16>(alg_data) && std::get<12>(alg_data)[vertex].empty()) { 
        return false;          
      }  

      BitSet vertex_template_vertices(std::get<14>(alg_data)[vertex]); // template_vertices
 
      if (vertex_template_vertices.none() || !vertex_template_vertices.test(pattern_indices[0])) {
        return false;  
      }

      // nonunique metadata
      if (path_checking_filter) { 
      // initiate tokens only from vertices with multiple template vertex matches 
      if (!pattern_valid_cycle)  { // path checking
        if (!(vertex_template_vertices.test(pattern_indices[0]) 
          && vertex_template_vertices.test(pattern_indices[pattern_indices.size() -1]) ) ) {
          return false;
        }        
        //std::cout << vertex_template_vertices << std::endl; // Test     
      } 
      } 
      // nonunique metadata    
 
      // token passing constraints go here
//      tppm_visitor new_visitor(vertex, g.label_to_locator(28), 0, 2, 0, pattern_indices[0], true, true, true); 
//      vis_queue->queue_visitor(new_visitor);
//      return true;
      
      //std::cout << "Found source vertex " << g.locator_to_label(vertex) << " vertex_data " << vertex_data << std::endl; // Test
      
      // Important: only the token_source_map on the controller contains the source vertex (wrong I think)
//      if (!(vertex.is_delegate() && g.master(vertex) != mpi_rank)) {
      
      // local, controller and delegates are added to the token_source_map 
      if (!std::get<16>(alg_data)) { // pattern_selected_vertices // the map was populate in the previous iteration    
 
        auto find_token_source = std::get<6>(alg_data).find(g.locator_to_label(vertex)); // token_source_map
        if (find_token_source == std::get<6>(alg_data).end()) {
          auto insert_status = std::get<6>(alg_data).insert({g.locator_to_label(vertex), false});
          if(!insert_status.second) {
            std::cerr << "Error: failed to add an element to the map." << std::endl;
            return false;   
          } 
          //std::cout << "Instrting " << vertex_data << " to token_source_map" << std::endl; // Test    
        }

      }  	
//      }

      // initiate token passing from the source vertex
//--      for(eitr_type eitr = g.edges_begin(vertex); eitr != g.edges_end(vertex); ++eitr) {
//--        vertex_locator neighbour = eitr.target();
      
      for (auto& item : std::get<15>(alg_data)[vertex]) { // vertex_active_edges_map
        vertex_locator neighbour = g.label_to_locator(item.first);
 
        //std::get<10>(alg_data)[eitr] = 240; // Test
        //int neighbour_edge_data = std::get<10>(alg_data)[eitr]; // Test

        //std::cout << "found source vertex " << g.locator_to_label(vertex) 
        //<< " vertex_data " << vertex_data 
        //<< " neighbour count : " << std::get<15>(alg_data)[vertex].size() 
        //<< " neighbour " << g.locator_to_label(neighbour)
        //<< std::endl;  
        //<< " edge data " << neighbour_edge_data << std::endl; // Test
        
        // token passing constraints go here
//        tppm_visitor new_visitor(neighbour, g.label_to_locator(4902294), 0, 2, 0, pattern_indices[0], true, true, false);
//        tppm_visitor new_visitor(neighbour, g.label_to_locator(932746), 0, (pattern_indices.size() - 2), 0, pattern_indices[0], true, true, false); // Test              

        // loop detection - path back to the source vertex is valid
        //tppm_visitor new_visitor(neighbour, vertex, 0, 2, 0, pattern_indices[0], true, true, false);

        if (std::get<16>(alg_data)) { // pattern_selected_vertices
          // vertex_token_source_set // the set was populate in the previous iteration 
          // std::cout << "Token source: " << g.locator_to_label(vertex) << " " << std::get<12>(alg_data)[vertex].size() << std::endl; // Test
          for (auto v : std::get<12>(alg_data)[vertex]) {
            tppm_visitor_tds new_visitor(neighbour, vertex, g.label_to_locator(v), visited_vertices, 0, pattern_cycle_length, 0, pattern_indices[0], pattern_valid_cycle, true, false);
            vis_queue->queue_visitor(new_visitor);
          }        
        } else { 
          // template driven search    
//          tppm_visitor new_visitor(neighbour, vertex, 0, (pattern_indices.size() - 2), 0, pattern_indices[0], true, true, false);
          tppm_visitor_tds new_visitor(neighbour, vertex, vertex, visited_vertices, 0, pattern_cycle_length, 0, pattern_indices[0], pattern_valid_cycle, true, false);
        
          // loop detection - path back to the source vertex is invalid
          //tppm_visitor new_visitor(neighbour, vertex, 0, 2, 0, pattern_indices[0], false, true, false);
          vis_queue->queue_visitor(new_visitor);
        } 
      }
//++      return true;
      return false; // controller not forwarding visitor to delegates
 
    } /*else if ((find_vertex->second.vertex_pattern_index == pattern_indices[0]) && itr_count == 0 && is_init_step) {
      // initiate token passing from the source vertex
      //std::cout << "found source vertex " << g.locator_to_label(vertex) << " vertex_data " << vertex_data << std::endl; // Test
      for(eitr_type eitr = g.edges_begin(vertex); eitr != g.edges_end(vertex); ++eitr) {
        vertex_locator neighbor = eitr.target();
        tppm_visitor new_visitor(neighbor, target_vertex, itr_count, max_itr_count, 
          source_index_pattern_indices, parent_pattern_index, true, true, false);
        vis_queue->queue_visitor(new_visitor);
      }  
      return true;
 
    }*/ else if (!is_init_step) { // else if      

      bool do_forward_token = false;
      auto new_itr_count = itr_count + 1; // Important : itr_count is the itr_count of the parent 
      auto next_pattern_index = source_index_pattern_indices + new_itr_count; // expected next pattern_index  
      auto vertex_pattern_index = 0; //find_vertex->second.vertex_pattern_index;
      //auto vertex_pattern_index = pattern_indices[source_index_pattern_indices + new_itr_count]; // TODO: read from the map
      
      // vertex_data == pattern[next_pattern_index] and vertex_pattern_index == pattern_indices[next_pattern_index] must hold  
      // otherwise return false
      if (vertex_data != pattern[next_pattern_index]) {
        return false;
      } 

      bool match_found = false;

      BitSet vertex_template_vertices(std::get<14>(alg_data)[vertex]); // template_vertices

      if (vertex_template_vertices.none()) {
        return false;
      }

      // verify template vertex match  
      // TODO: replace the loop by vertex_template_vertices.test(pattern_indices[next_pattern_index])
      for (size_t i = 0; i < vertex_template_vertices.size(); i++) { 
        if (vertex_template_vertices.test(i)) {
          if (i == pattern_indices[next_pattern_index]) {
            match_found = true;
            vertex_pattern_index = i;
            break; 
          }
        }
      }

      if (!match_found) {
        return false;
      }
  
/*//--      if (vertex.is_delegate() && g.master(vertex) != mpi_rank) { // delegate but not the controller
        if (vertex_data == pattern[next_pattern_index]) {
          vertex_pattern_index = pattern_indices[next_pattern_index];  
        } else {
          return false;  
        }  
//--      } else {
//--        auto find_vertex = std::get<5>(alg_data).find(g.locator_to_label(vertex));
//--        if (find_vertex == std::get<5>(alg_data).end()) {
//--          return false;
//--        }
//--        vertex_pattern_index = find_vertex->second.vertex_pattern_index;
//--      }*/      
      
      // TODO: verify next_pattern_index < pattern_indices.size() before anythin else

      //int mpi_rank = havoqgt_env()->world_comm().rank(); // Test
      //if (mpi_rank == 51) { 
      //  std::cout << g.locator_to_label(vertex) << " vertex_pattern_index "
      //  << vertex_pattern_index << " received from parent_pattern_index " 
      //  << parent_pattern_index << " itr_count " << itr_count 
      //  << " max_itr_count " << max_itr_count << " next_pattern_index " << next_pattern_index
      //  << std::endl; // Test	 
      //} // Test  

      if (max_itr_count > itr_count) {
        // verify if received from a valid parent       
        // are vertex_data and vertex_pattern_index valid
//        if (vertex_data == pattern[pattern_indices[next_pattern_index]] && 
	// TODO: Important: for token passing only        
        if (vertex_data == pattern[next_pattern_index] &&
          vertex_pattern_index == pattern_indices[next_pattern_index]) {
          // verify if received from a valid parent
//          for (auto e = pattern_graph.vertices[vertex_pattern_index]; 
//            e < pattern_graph.vertices[vertex_pattern_index + 1]; e++) {  
//            if (pattern_graph.edges[e] == parent_pattern_index) {
//              do_forward_token = true; 
//              break; 
//            }  
//          } // for      
          if (parent_pattern_index == pattern_indices[next_pattern_index - 1]) { 
            do_forward_token = true; 
          }   
        } // if    

        // duplicate is not expected in visited_vertices         
        // verify if vertex was already visited
        //if (g.locator_to_label(visited_vertices[new_itr_count]) == g.locator_to_label(vertex)) {
        //  return false;          
        //}
        //for (size_t i = 0; i < new_itr_count; i++) {
        //  if (g.locator_to_label(visited_vertices[i]) == g.locator_to_label(vertex)) {
        //    break;
        //    return false;
        //  }    
        //}
        
        // TDS 
        //std::get<18>(alg_data); // pattern_enumeration_indices
        if (std::get<18>(alg_data)[new_itr_count] == new_itr_count) {
          // duplicate is not expected in visited_vertices  
          for (size_t i = 0; i < new_itr_count; i++) { // Important : must be < not <=
            if (g.locator_to_label(visited_vertices[i]) == g.locator_to_label(vertex)) {
              //break;
              return false;
            }
          }  
        } else if (std::get<18>(alg_data)[new_itr_count] < new_itr_count) {
          // vertex is expected in visited_vertices
          if (g.locator_to_label(visited_vertices[std::get<18>(alg_data)[new_itr_count]]) 
            != g.locator_to_label(vertex)) {
            return false;
          }
        } else {
          std::cerr << "Error: invalid value B." << std::endl;
          return false;
        }         
 
      } else if (max_itr_count == itr_count) {
        // are vertex_data and vertex_pattern_index valid
        //bool match_found = false;
        match_found = false;
 
        // verify if received from a valid parent 
//        if (vertex_data == pattern[pattern_indices[next_pattern_index]] &&
        // TODO: Important: for token passing only        
        if (vertex_data == pattern[next_pattern_index] &&        
          vertex_pattern_index == pattern_indices[next_pattern_index]) { 
          // verify if received from a valid parent
//          for (auto e = pattern_graph.vertices[vertex_pattern_index];
//            e < pattern_graph.vertices[vertex_pattern_index + 1]; e      return false;) {
//            if (pattern_graph.edges[e] == parent_pattern_index) {
//              match_found = true;
//              break;
//            }
//          } // for
          if (parent_pattern_index == pattern_indices[next_pattern_index - 1]) {
            match_found = true; 
          }      
        } // if

        // path checking
        if (match_found && !expect_target_vertex) {
          if (g.locator_to_label(vertex) == g.locator_to_label(target_vertex)) {
            //std::cout << "found invalid cycle - vertex " <<  " | parent_pattern_index "
            //  <<  parent_pattern_index <<  " | " << g.locator_to_label(target_vertex)
            //  <<  " vertex_pattern_index " << vertex_pattern_index << " itr "
            //  << itr_count << std::endl; // Test  
            return false; //true; 
                      
          } else {
            //std::cout << "found valid path - vertex " <<  " | parent_pattern_index "
            //  <<  parent_pattern_index <<  " | " << g.locator_to_label(target_vertex)
            //  <<  " vertex_pattern_index " << vertex_pattern_index << " itr "
            //  << itr_count << std::endl; // Test 
               
            // valid path found, send ack to the token source so it could update its state
            tppm_visitor_tds new_visitor(target_vertex, vertex, target_vertex, visited_vertices, itr_count, max_itr_count,
              source_index_pattern_indices, 0, expect_target_vertex, false, false, true);
            vis_queue->queue_visitor(new_visitor); 

            // write to the paths_result_file  
            std::get<17>(alg_data) << "[" << mpi_rank << "], "; 
            for (size_t i = 0; i <= new_itr_count ; i++) { 
              std::get<17>(alg_data) << g.locator_to_label(visited_vertices[i]) << ", ";  
            }
            std::get<17>(alg_data) << "[" << g.locator_to_label(vertex) << "]\n"; 
 
            return false; //true;            
          }  
        } 
 
        // cycle checking
        // is this the target vertex
        else if (g.locator_to_label(vertex) == g.locator_to_label(target_vertex)
          && (g.locator_to_label(vertex) == g.locator_to_label(visited_vertices[0]))
          && match_found && expect_target_vertex) {

          // duplicate is not expected in visited_vertices // TODO: may not need this here
          //for (size_t i = 1; i < new_itr_count; i++) { // Important : i = 1
          //  if (g.locator_to_label(visited_vertices[i]) == g.locator_to_label(vertex)) {
          //    break;
          //    return false;
          //  }
          //} 

          // found cycle 
          //std::cout << "found valid cycle - vertex " << g.locator_to_label(vertex) <<  " | parent_pattern_index " 
          //<<  parent_pattern_index <<  " | " << g.locator_to_label(target_vertex) 
          //<<  " vertex_pattern_index " << vertex_pattern_index << " itr " 
          //<< itr_count << std::endl; // Test

          //return false; // TODO: true ?	

          // TODO: this is probbaly wrong
          // Important: only the token_source_map on the controller contains the source vertex (wrong I think)
//          if (vertex.is_delegate() && (g.master(vertex) != mpi_rank)) {
//            std::get<9>(alg_data) = 1; // true; // pattern_found
//            return true;		        
//          }  
//          if (!(vertex.is_delegate() && g.master(vertex) != mpi_rank)) {
          auto find_token_source = std::get<6>(alg_data).find(g.locator_to_label(vertex)); // token_source_map
          if (find_token_source == std::get<6>(alg_data).end()) {
            std::cerr << "Error: did not find the expected item in the map." << std::endl;
            //return true; // false ?           
            return false;
          }

          find_token_source->second = 1; //true;   
//          }
 
          std::get<9>(alg_data) = 1; // true; // pattern_found

          // Test
          /*std::cout << itr_count << " " << g.locator_to_label(visited_vertices[new_itr_count]) << " "  
            << g.locator_to_label(target_vertex) << std::endl;  
          for (size_t i = 0; i < visited_vertices.size(); i++) {
            std::cout << "[" << i << " : " << g.locator_to_label(visited_vertices[i]) << "], ";  
          } 	
          std::cout << std::endl;*/
   
          // write to the paths_result_file  
          std::get<17>(alg_data) << "[" << mpi_rank << "], "; 
          for (size_t i = 0; i <= new_itr_count ; i++) { 
            std::get<17>(alg_data) << g.locator_to_label(visited_vertices[i]) << ", ";  
          }
          std::get<17>(alg_data) << "[" << g.locator_to_label(vertex) << "]\n"; 
  
          // Test

//++          return true; // Important : must return true to handle delegates
	  return false;
        } 

        //else if (g.locator_to_label(vertex) == g.locator_to_label(target_vertex) 
        //  && match_found && !expect_target_vertex) {
          // TODO: TBA 
          //std::cout << "found invalid cycle - vertex " <<  " | parent_pattern_index "
          //<<  parent_pattern_index <<  " | " << g.locator_to_label(target_vertex)
          //<<  " vertex_pattern_index " << vertex_pattern_index << " itr "
          //<< itr_count << std::endl; // Test  
        //  return true; 
        //} 

        else {
          // reached max iteration but did not find the target vertex or a loop
          //std::cout << "At " << g.locator_to_label(vertex) <<  ", did not find target " 
          //<< g.locator_to_label(target_vertex) <<  " after " << itr_count 
          //<< " iterations" <<std::endl; // Test
          return false;//true;//false;  
        }   
      } else {
        std::cerr << "Error: wrong code branch." << std::endl;    
        return false;
      }   

      if (!do_forward_token) {
        return false;
      } //else {
        //return false; // Test 
      //} 

      // all good, forward along the token
      
      // if vertex is a delegate
/*++      if (vertex.is_delegate() && (g.master(vertex) != mpi_rank)) {
        // forwarded a token from a source, now update vertex_token_source_set 
        auto find_token_source_forwarded = std::get<12>(alg_data)[vertex].find(g.locator_to_label(target_vertex));
        if (find_token_source_forwarded == std::get<12>(alg_data)[vertex].end()) {
          auto insert_status = std::get<12>(alg_data)[vertex].insert(g.locator_to_label(target_vertex));		
	  if(!insert_status.second) {
            std::cerr << "Error: failed to add an element to the set." << std::endl;
            return false;
          }
          //std::cout << g.locator_to_label(vertex) << " adding " << g.locator_to_label(target_vertex) 
          //  << " to the vertex set" << std::endl; // Test 
        } else {
          std::cerr << "Error: unexpected item in the set." << std::endl;
	  return false;
        }        
      } // if vertex is a delegate 
*/

      //if (vertex_pattern_index == 2) // Test 
        //std::cout << g.locator_to_label(vertex) << " vertex_pattern_index " 
        //<< vertex_pattern_index << " " << new_itr_count << " forwarding ... " 
        //<< g.locator_to_label(target_vertex) << std::endl; // Test
      //size_t max_nbr_count = 0; // Test 
//      for(eitr_type eitr = g.edges_begin(vertex); eitr != g.edges_end(vertex); ++eitr) {
//        vertex_locator neighbor = eitr.target();

      for (auto item : std::get<15>(alg_data)[vertex]) { // vertex_active_edges_map
        vertex_locator neighbour = g.label_to_locator(item.first);
   
        // do not forward the token to the parent the vertex received it from
        //if (g.locator_to_label(neighbour) == g.locator_to_label(parent)) {
        //  continue;
        //} 
        
 	//std::cout << g.locator_to_label(vertex) << " "
        // << " neighbour count : " << std::get<15>(alg_data)[vertex].size() << " "   
 	// << (new_itr_count + 1) << " " << std::get<18>(alg_data)[new_itr_count + 1] << " "
        // << g.locator_to_label(visited_vertices[std::get<18>(alg_data)[new_itr_count + 1]]) << " "
 	// << g.locator_to_label(neighbour) << std::endl; // Test

        // penultimate hop, only forward to the target_vertex 
        if (max_itr_count == new_itr_count) {
          if (expect_target_vertex && (g.locator_to_label(target_vertex) != g.locator_to_label(neighbour))) {
            // cycle checking
            continue;       
          } else if (!expect_target_vertex && (g.locator_to_label(target_vertex) == g.locator_to_label(neighbour))) {
            // path checking
            continue;
          } else if (!expect_target_vertex && (g.locator_to_label(target_vertex) != g.locator_to_label(neighbour))) {
            // path checking
            match_found = false;

            // TDS 
            //std::get<18>(alg_data); // pattern_enumeration_indices
            if (std::get<18>(alg_data)[new_itr_count + 1] == new_itr_count + 1) {
              // duplicate is not expected in visited_vertices  
              for (size_t i = 0; i <= new_itr_count; i++) {
                if (g.locator_to_label(visited_vertices[i]) == g.locator_to_label(neighbour)) {                
                  match_found = true; 
                  break;
                }
              }  
            } else if (std::get<18>(alg_data)[new_itr_count + 1] < new_itr_count + 1) {
              // neighbour is expected in visited_vertices
              if (g.locator_to_label(visited_vertices[std::get<18>(alg_data)[new_itr_count + 1]]) 
                != g.locator_to_label(neighbour)) {
                match_found = true;              
              }  
            } else {
              std::cerr << "Error: invalid value C." << std::endl;
              //return false;
              match_found = true;
            }       
           
            if (match_found) {
              continue;
            }
            
          } 
        } else if (max_itr_count > itr_count) {
          match_found = false;
   
          // TODO: do not forward to the target_vertex   

          // duplicate is not expected in visited_vertices 
          //for (size_t i = 0; i <= new_itr_count; i++) {
          //  if (g.locator_to_label(visited_vertices[i]) == g.locator_to_label(neighbour)) {
          //    match_found = true;      
          //    break;     
          //  }  
          //}
          //if (match_found) {
          //  continue;
          //}
          
          // TDS 
          //std::get<18>(alg_data); // pattern_enumeration_indices
          if (std::get<18>(alg_data)[new_itr_count + 1] == new_itr_count + 1) {
            // duplicate is not expected in visited_vertices  
            for (size_t i = 0; i <= new_itr_count; i++) {
              if (g.locator_to_label(visited_vertices[i]) == g.locator_to_label(neighbour)) {                
                match_found = true; 
                break;
              }
            }  
          } else if (std::get<18>(alg_data)[new_itr_count + 1] < new_itr_count + 1) {
            // neighbour is expected in visited_vertices
            if (g.locator_to_label(visited_vertices[std::get<18>(alg_data)[new_itr_count + 1]]) 
              != g.locator_to_label(neighbour)) {
              match_found = true;              
            }  
          } else {
            std::cerr << "Error: invalid value D." << std::endl;
            //return false;
            match_found = true;
          }       
           
          if (match_found) {
            continue;
          }      
             
        } else {
          std::cerr << "Error: wrong code branch." << std::endl;
          return false;
        }  

        //std::cout << g.locator_to_label(vertex) << " vertex_pattern_index " 
        //  << vertex_pattern_index << " " << new_itr_count << " forwarding to " 
        //  << g.locator_to_label(neighbour) << std::endl; // Test 
        
        tppm_visitor_tds new_visitor(neighbour, vertex, target_vertex, visited_vertices, 
          new_itr_count, max_itr_count, source_index_pattern_indices, vertex_pattern_index, 
          expect_target_vertex); 
          // vertex_pattern_index = parent_pattern_index for the neighbours 
        vis_queue->queue_visitor(new_visitor);
        // Test
        //if (max_nbr_count == 20) {
        //  break;     
        //} else {
        //  max_nbr_count++;
        //}  
        // Test    
      }
	     
//++      return true;
      return false; // controller not forwarding visitor to delegates

      // else if
    } else {
      return false;
    }
    return false;		 
  }

  friend inline bool operator>(const tppm_visitor_tds& v1, const tppm_visitor_tds& v2) {
    //return false;
    if (v1.itr_count > v2.itr_count) {
      return true;
    } else if (v1.itr_count < v2.itr_count) {
      return false; 
    }
    if (v1.vertex == v2.vertex) {
      return false;
    }
    return !(v1.vertex < v2.vertex);
    
    /*if (v1.itr_count <= v2.itr_count) {
      return true;
    } else {
      return false;
    }*/ 
  }

  //friend inline bool operator<(const tppm_visitor& v1, const tppm_visitor& v2) {
    //return false;
    //if (v1.itr_count < v2.itr_count) {
    //  return true;
    //} else if (v1.itr_count > v2.itr_count) {
    //  return false;
    //}
    //if (v1.vertex == v2.vertex) {
    //  return false;
    //}
    //return !(v1.vertex < v2.vertex);    
  //}

  vertex_locator vertex;
  vertex_locator parent;  
  vertex_locator target_vertex; // for a cycle, this is also the originating vertex
  size_t itr_count; // TODO: change type // Important : itr_count of the parent 
  size_t max_itr_count; // equal to diameter - 1 of the pattern as itr_count is initialized to 0 // TODO: change type
  bool expect_target_vertex;
  bool do_pass_token;
  bool is_init_step;
  bool ack_success; 
  size_t source_index_pattern_indices; // index of the token source in the pattern_indices container 
  size_t parent_pattern_index; // TODO: change to the same type as in the pattern_graph
  std::array<vertex_locator,16> visited_vertices;
};

template <typename TGraph, typename Vertex, typename VertexMetaData, typename PatternData, 
  typename PatternIndices, typename PatternEnumerationIndices, 
  typename VertexRank, typename PatternGraph, 
  typename VertexStateMap, typename TokenSourceMap, typename EdgeMetaData, 
  typename VertexSetCollection, typename VertexActive, typename TemplateVertex, 
  typename VertexUint8MapCollection, typename BitSet>
void token_passing_pattern_matching(TGraph* g, VertexMetaData& vertex_metadata, 
  PatternData& pattern, PatternIndices& pattern_indices,
  PatternEnumerationIndices& pattern_enumeration_indices,  
  VertexRank& vertex_rank, PatternGraph& pattern_graph, VertexStateMap& vertex_state_map,
  TokenSourceMap& token_source_map, size_t pattern_cycle_length, bool pattern_valid_cycle, 
  std::vector<uint8_t>::reference pattern_found, EdgeMetaData& edge_metadata, 
  VertexSetCollection& vertex_token_source_set, VertexActive& vertex_active, 
  TemplateVertex& template_vertices, VertexUint8MapCollection& vertex_active_edges_map, 
  bool pattern_selected_vertices, std::ofstream& paths_result_file,
  uint64_t& message_count) { // TODO: bool& pattern_found does not work, why?
  //std::cout << "token_passing_pattern_matching_new.hpp" << std::endl;
  
  visitor_count = 0;

  typedef tppm_visitor_tds<TGraph, Vertex, BitSet> visitor_type;
  auto alg_data = std::forward_as_tuple(vertex_metadata, pattern, pattern_indices, vertex_rank, 
    pattern_graph, vertex_state_map, token_source_map, pattern_cycle_length, pattern_valid_cycle, pattern_found, 
    edge_metadata, g, vertex_token_source_set, vertex_active, template_vertices, vertex_active_edges_map, 
    pattern_selected_vertices, paths_result_file, pattern_enumeration_indices);
  auto vq = create_visitor_queue<visitor_type, /*havoqgt::detail::visitor_priority_queue*/tppm_queue>(g, alg_data);
  vq.init_visitor_traversal_new();
  //vq.init_visitor_traversal_new_batch();
  //vq.init_visitor_traversal_new_alt();
  MPI_Barrier(MPI_COMM_WORLD);

  message_count = visitor_count;
}

}} //end namespace havoqgt::mpi

#endif //HAVOQGT_TOKEN_PASSING_PATTERN_MATCHING_NONUNIQUE_TDS_BATCH_1_HPP_INCLUDED

