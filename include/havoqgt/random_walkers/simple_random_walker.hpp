
#ifndef _SIMPLE_RANDOM_WALKER_HPP
#define _SIMPLE_RANDOM_WALKER_HPP

#include <unordered_set>
#include <iostream>
#include <tuple>

template <typename MetaDataType, typename VertexType, typename RandomEdgeContainer>
class simple_random_walker {
public:
  using vertex_locator = VertexType;
  using metadata_type = MetaDataType;

  typedef std::pair< uint64_t, uint64_t> interval;

  simple_random_walker() : id(0) {}
  
  simple_random_walker( uint64_t _id, vertex_locator _start_from, interval _started_at
			,interval _cur_time
			,uint64_t _cost = 0
			,uint64_t _steps = 0
		      )
    : id(_id), start_from(_start_from), cost(_cost),
      cur_time(_cur_time), started_at(_started_at), 
      steps(_steps) { }

  bool is_complete(vertex_locator vertex) const {
    return (local_targets.find(vertex) != local_targets.end() ) || steps >= simple_random_walker::max_steps;
  }

  //Function returning the next state for the random walkers
  std::tuple<bool, vertex_locator, simple_random_walker> next(vertex_locator cur_vertex ) const{
    uint64_t edge_cost = 0;
    vertex_locator edge_target;
    auto op = [&edge_cost,&edge_target]( metadata_type& metadata, vertex_locator target)->void {
      edge_cost = metadata.redirect ? 0 : 1;
      edge_target = target;
    };
    std::pair<bool, interval> next = random_edge_container->template get_random_weighted_edge(op, cur_time, cur_vertex);
    if( !next.first ) return std::make_tuple( false, vertex_locator(),  simple_random_walker() );

    /*    if( steps == 0) {
      simple_random_walker next_rw(id, start_from, next.second, next.second, cost+edge_cost, steps+1);
      return std::make_tuple( true, edge_target, next_rw);
      }*/
    simple_random_walker next_rw(id, start_from, started_at, next.second, cost + edge_cost, steps + 1);
    return std::make_tuple( true , edge_target, next_rw );
  }

  friend std::ostream& operator<<(std::ostream& o, const simple_random_walker& rw) {
    return o << rw.id
	     << " "  << rw.started_at.first << " " << rw.started_at.second
             << " " << rw.cur_time.first   << " " << rw.cur_time.second
	     << " " << rw.cost << " " << rw.steps;
  }  
  // Just making them public
  uint64_t id;
  vertex_locator start_from;
  uint64_t cost;
  uint64_t steps;
  interval cur_time;
  interval started_at;
  static std::unordered_set<vertex_locator, typename vertex_locator::hasher> local_targets;
  static uint64_t max_steps;
  static RandomEdgeContainer* random_edge_container;
};

template<typename MetaDataType, typename VertexType, typename RandomEdgeContainer>
std::unordered_set< VertexType, typename VertexType::hasher> simple_random_walker<MetaDataType, VertexType, RandomEdgeContainer>::local_targets;

template<typename MetaDataType, typename VertexType, typename RandomEdgeContainer>
uint64_t simple_random_walker<MetaDataType, VertexType, RandomEdgeContainer>::max_steps;

template<typename MetaDataType, typename VertexType, typename RandomEdgeContainer>
RandomEdgeContainer *simple_random_walker<MetaDataType, VertexType, RandomEdgeContainer>::random_edge_container;
 
#endif