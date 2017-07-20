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

#ifndef HAVOQGT_BIAS_RANDWALK_HPP_INCLUDED
#define HAVOQGT_BIAS_RANDWALK_HPP_INCLUDED


#include <havoqgt/visitor_queue.hpp>
#include <havoqgt/detail/visitor_priority_queue.hpp>
#include <random>

namespace havoqgt { 

template<typename Graph, typename MTData>
class bias_randwalk_visitor {
public:
  typedef typename Graph::vertex_locator                 vertex_locator;
  bias_randwalk_visitor() { }

  //template<typename MTData>
  bias_randwalk_visitor(vertex_locator _vertex, vertex_locator _from, MTData _meta_data)
    : vertex(_vertex)
    , from(_from)
    , meta_data(_meta_data) { }

  bias_randwalk_visitor(vertex_locator _vertex)
    : vertex(_vertex)
    , from(_vertex) {}
  //  , meta_data(0) { }

  template<typename AlgData>
  bool pre_visit(AlgData& alg_data) const {
      return true;
  }
  
  template<typename VisitorQueueHandle, typename AlgData>
  bool init_visit(Graph& g, VisitorQueueHandle vis_queue, AlgData& alg_data) const {
      	    for(auto eitr = g.edges_begin(vertex); eitr != g.edges_end(vertex); ++eitr) {
		//copy_edge_visitor new_visitor(eitr.target(), *vitr, std::get<0>(alg_data)[*vitr] );
		if( vertex == std::get<5>(alg_data)) {
		for(int i=0; i<100; i++) {
		bias_randwalk_visitor new_visitor(eitr.target(), vertex, 0 );
		vis_queue->queue_visitor(new_visitor); 
		}
                std::cout<<" in init_visit of "<< g.locator_to_label(vertex) << " to " << g.locator_to_label(eitr.target()) <<std::endl;
                }
	    }
	    std::get<7>(alg_data)[vertex]=std::discrete_distribution<>({0.05, 0.9, 0.1});
	    //if(g.locator_to_label(vertex)==0)
	    //   std::get<7>(alg_data)=std::discrete_distribution<>({0.05, 0.9, 0.1});
	    return true;
        
  }

  template<typename VisitorQueueHandle, typename AlgData>
  bool visit(Graph& g, VisitorQueueHandle vis_queue, AlgData& alg_data) const {
      std::cout<<"in visit function of randwalk"<<g.locator_to_label(vertex)<<" from "<<g.locator_to_label(from)<<std::endl;
	    //if(g.locator_to_label(vertex)==0)
	      // std::get<7>(alg_data)=std::discrete_distribution<>({0.05, 0.9, 0.1});
      // increment visit count of this vertex
      std::get<1>(alg_data)[vertex]++;
 	std::cout<<"vertex data "<<std::get<1>(alg_data)[vertex]<<std::endl;
      // increment visit count of this incoming edge
      for(auto eitr = g.edges_begin(vertex); eitr != g.edges_end(vertex); ++eitr) {
        auto neighbor = eitr.target();
	if( g.locator_to_label(neighbor) == g.locator_to_label(from) ) 
	    std::get<0>(alg_data)[eitr] ++; 
      }
      // find the next vertex to walk
      //vertex_locator next = g.label_to_locator(std::get<3>(alg_data)[vertex](gen));
      //uint64_t next; 
      vertex_locator next;
      std::uniform_int_distribution<> unit_dis;
     // auto direction = disc_dis(std::get<7>(alg_data));
     std::cout<<"start to walk"<<std::endl;
     auto dir = std::get<7>(alg_data)[vertex](std::get<6>(alg_data));
     //auto dir = std::get<7>(alg_data)(std::get<6>(alg_data));
     std::cout<<"dir is "<<dir<<std::endl;
     int ver;
     bool flag=0;
     while(!flag)
      switch (dir){
	case 0: //go upstream
		std::cout<<"go upstream"<<std::endl;
		if (std::get<3>(alg_data)[vertex].size()<1) {
		   std::cout<<"unfornately no ancestor"<<std::endl;
		   return true;
                }
		std::cout<<"in unit "<<std::get<3>(alg_data)[vertex].size()<<std::endl;
		unit_dis = std::uniform_int_distribution<>(0, std::get<3>(alg_data)[vertex].size()-1);
		ver = unit_dis(std::get<6>(alg_data));
		std::cout<<"done unit "<<ver<<std::endl;
		//next = std::get<4>(alg_data)[uni_dis(gen)];
		next = std::get<3>(alg_data)[vertex][ver];
		flag=1;
		break;
	case 1: // go downstream
		//std::uniform_int_distribution<> uni_dis(0, downstream.size()-1);
		std::cout<<"go downstream"<<std::endl;
		if (std::get<2>(alg_data)[vertex].size()<1) {
		   std::cout<<"unfornately no decendents"<<std::endl;
		   return true;
                }
		std::cout<<"in unit "<<std::get<2>(alg_data)[vertex].size()<<std::endl;
		unit_dis = std::uniform_int_distribution<>(0, std::get<2>(alg_data)[vertex].size()-1);
		//next = std::get<3>(alg_data)[uni_dis(gen)];
		next = std::get<2>(alg_data)[vertex][unit_dis(std::get<6>(alg_data))];
		flag=1;
		break;
 	case 2: 
		//std::uniform_int_distribution<> uni_dis(0, curstream.size()-1);
		std::cout<<"go curstream"<<std::endl;
		std::cout<<"in unit "<<std::get<4>(alg_data)[vertex].size()<<std::endl;
		unit_dis = std::uniform_int_distribution<>(0, std::get<4>(alg_data)[vertex].size()-1);
		//next = std::get<5>(alg_data)[uni_dis(gen)];
		next = std::get<4>(alg_data)[vertex][unit_dis(std::get<6>(alg_data))];
		flag=1;
		break;
      }
     std::cout<<"next is "<<g.locator_to_label(next)<<std::endl;
     //std::cout<<"next is "<<next<<std::endl;

      if(meta_data+1<100) {
         //bias_randwalk_visitor new_visitor(g.label_to_locator(next), vertex, meta_data+1);
         bias_randwalk_visitor new_visitor(next, vertex, meta_data+1);
	 vis_queue->queue_visitor(new_visitor); 
 
      }
     
    return true; 
      	
  }


  friend inline bool operator>(const bias_randwalk_visitor& v1, const bias_randwalk_visitor& v2) {
    return false;
  }

  vertex_locator   vertex;
  vertex_locator  from;
  MTData meta_data;
};

template <typename TGraph,  typename EdgeData, typename VerData, typename QueueType>
void bias_randwalk(TGraph* g, QueueType& upstream, QueueType& downstream, QueueType& curstream, EdgeData& edge_data, VerData& vertex_data, auto source) {

  typedef  bias_randwalk_visitor<TGraph, uint16_t>    visitor_type;
  //auto upstream, downstream, curstream;
  typename TGraph::template vertex_data<std::discrete_distribution<>, std::allocator<std::discrete_distribution<>>> dir(*g);
  //typename std::discrete_distribution<> dir;
 
  std::random_device rd;
  std::mt19937 gen(rd());

 // for(auto citr = g->controller_begin(); citr != g->controller_end(); ++citr) {
 //   cc_data[*citr] = *citr;
 // } 
  
  //for(auto vir = g->vertices_begin(); vir!=g->vertices_end(); vir++)
  //  std::cout<<"vir level "<<g->locator_to_label(*vir)<<std::endl;

  auto alg_data = std::forward_as_tuple(edge_data, vertex_data, downstream, upstream, curstream, source, gen, dir);
  //auto alg_data = std::forward_as_tuple(level_data, edge_data, vertex_data, prob_data);
  auto vq = create_visitor_queue<visitor_type, detail::visitor_priority_queue>(g, alg_data); 
  vq.init_visitor_traversal_new();
  //vq.init_visitor_traversal(source);
}



} //end namespace havoqgt




#endif //HAVOQGT_BIAS_RANDWALK_HPP_INCLUDED
