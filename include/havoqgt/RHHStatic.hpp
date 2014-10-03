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

#ifndef HAVOQGT_MPI_RHHSTATIC_HPP_INCLUDED
#define HAVOQGT_MPI_RHHSTATIC_HPP_INCLUDED

#define DEBUG(msg) do { std::cerr << "DEG: " << __FILE__ << "(" << __LINE__ << ") " << msg << std::endl; } while (0)
#define DEBUG2(x) do  { std::cerr << "DEG: " << __FILE__ << "(" << __LINE__ << ") " << #x << " =\t" << x << std::endl; } while (0)
#define DISP_VAR(x) do  { std::cout << #x << " =\t" << x << std::endl; } while (0)

namespace RHH {
  
  enum UpdateErrors {
    kSucceed,
    kDuplicated,
    kReachingFUllCapacity,
    kLongProbedistance
  };
  
  /// --------------------------------------------------------------------------------------- ///
  ///                                RHH static class
  /// --------------------------------------------------------------------------------------- ///
  template <typename KeyType, typename ValueType, uint64_t Capacity>
  class RHHStatic {
    
  public:

    /// ---------  Typedefs and Enums ------------ ///
    typedef RHHStatic<KeyType, ValueType, Capacity> RHHStaticType;
    typedef unsigned char PropertyBlockType;
    typedef unsigned char ProbeDistanceType;
    static const ProbeDistanceType kLongProbedistanceThreshold = 32LL;

    
    ///  ------------------------------------------------------ ///
    ///              Constructor / Destructor
    ///  ------------------------------------------------------ ///
    
    /// --- Constructor --- //
    RHHStatic()
    : m_next_(nullptr)
    {
      DEBUG("RHHStatic constructor");
      for (uint64_t i=0; i < Capacity; i++) {
        m_property_block_[i] = kEmptyValue;
      }
    }
    
    /// --- Copy constructor --- //
    
    /// --- Move constructor --- //
    // XXX: ???
    RHHStatic(RHHStatic&& old_obj)
    {
      DEBUG("RHHStatic move-constructor");
      // m_property_block_ = old_obj.m_property_block_;
      // old_obj.m_property_block_ = NULL;
    }
    
    /// --- Destructor --- //
    ~RHHStatic()
    {
      DEBUG("RHHStatic destructor");
    }
    
    /// ---  Move assignment operator --- //
    // XXX: ???
    RHHStatic &operator=(RHHStatic&& old_obj)
    {
      DEBUG("RHHStatic move-assignment");
      // m_pos_head_ = old_obj.m_pos_head_;
      // old_obj.m_pos_head_ = nullptr;
      return *this;
    }
    
    
    ///  ------------------------------------------------------ ///
    ///              Public Member Functions
    ///  ------------------------------------------------------ ///
    UpdateErrors insert_uniquely(KeyType key, ValueType val)
    {
      ProbeDistanceType dist = 0;
      const int64_t pos_key = find_key(key, &dist, true);
      
      if (pos_key != kInvaridIndex) {
        return kDuplicated;
      }
      
      return insert_helper(std::move(key), std::move(val));
    }
    
    bool erase(KeyType key)
    {
      ProbeDistanceType dist = 0;
      const int64_t pos = find_key(key, &dist,  false);
      if (pos != kInvaridIndex) {
        delete_key(pos);
        return true;
      }
      
      if (m_next_ != nullptr) {
        RHHStaticType *next_rhh = reinterpret_cast<RHHStaticType *>(m_next_);
        return next_rhh->erase(key);
      }
      
      return false;
    }
    
  private:
    /// ---------  Typedefs and Enums ------------ ///
    typedef uint64_t HashType;
    
    static const PropertyBlockType kTombstoneMask     = 0x80; /// mask value to mark as deleted
    static const PropertyBlockType kProbedistanceMask = 0x7F; /// mask value to extract probe distance
    static const PropertyBlockType kEmptyValue        = 0x7F; /// value repsents cleared space
    static const int64_t kInvaridIndex = -1LL;
    static const uint64_t kMask = Capacity - 1ULL;
    
    
    ///  ------------------------------------------------------ ///
    ///              Private Member Functions
    ///  ------------------------------------------------------ ///
    /// ------ Private member functions: algorithm core ----- ///
    inline HashType hash_key(KeyType& key)
    {
#if 1
      return static_cast<HashType>(key);
#else
      std::hash<KeyType> hash_fn;
      return hash_fn(key);
#endif
    }
    
    inline int64_t cal_desired_pos(HashType hash)
    {
      return hash & kMask;
    }
    
    inline ProbeDistanceType cal_probedistance(HashType hash, const int64_t slot_index)
    {
      return ((slot_index + (Capacity) - cal_desired_pos(hash)) & kMask);
    }
    
    inline PropertyBlockType& property(const int64_t ix)
    {
      return m_property_block_[ix];
    }
    
    inline PropertyBlockType property(const int64_t ix) const
    {
      return const_cast<RHHStaticType*>(this)->property(ix);
    }
    
    inline PropertyBlockType cal_property(HashType hash, const int64_t slot_index)
    {
      return cal_probedistance(hash, slot_index);
    }
    
    inline PropertyBlockType cal_property(const ProbeDistanceType probedistance)
    {
      return probedistance;
    }
    
    inline void delete_key(const int64_t positon)
    {
      property(positon) |= kTombstoneMask;
    }
    
    inline static bool is_deleted(const PropertyBlockType prop)
    {
      return (prop & kTombstoneMask) == kTombstoneMask;
    }
    
    inline static ProbeDistanceType extract_probedistance(PropertyBlockType prop)
    {
      return prop & kProbedistanceMask;
    }
    
    UpdateErrors insert_helper(KeyType &&key, ValueType &&val)
    {
      int64_t pos = cal_desired_pos(hash_key(key));
      ProbeDistanceType dist = 0;
      UpdateErrors err;
      while(true) {
        // if (dist >= kLongProbedistanceThreshold) {
        //   err = kLongProbedistance;
        //   break;
        // }
        
        PropertyBlockType existing_elem_property = property(pos);
        
        if(existing_elem_property == kEmptyValue)
        {
          return construct(pos, hash_key(key), std::move(key), std::move(val));
        }
        
        /// If the existing elem has probed less than or "equal to" us, then swap places with existing
        /// elem, and keep going to find another slot for that elem.
        if (extract_probedistance(existing_elem_property) <= dist)
        {
          if(is_deleted(existing_elem_property))
          {
            return construct(pos, hash_key(key), std::move(key), std::move(val));
          }
          m_property_block_[pos] = dist;
          std::swap(key, m_key_block_[pos]);
          std::swap(val, m_value_block_[pos]);
          dist = extract_probedistance(existing_elem_property);
        }
        
        pos = (pos+1) & kMask;
        ++dist;
      }
      
      if (m_next_ != nullptr)  {
        RHHStaticType *next_rhh = reinterpret_cast<RHHStaticType *>(m_next_);
        return next_rhh->insert_helper(std::move(key), std::move(val));
      }
      
      return err;
    }
    
    inline UpdateErrors construct(const int64_t ix, const HashType hash, KeyType&& key, ValueType&& val)
    {
      ProbeDistanceType probedist = cal_probedistance(hash, ix);
      m_property_block_[ix] = probedist;
      m_key_block_[ix] = std::move(key);
      m_value_block_[ix] = std::move(val);
      if (probedist >= kLongProbedistanceThreshold) {
        return kLongProbedistance;
        
      }
      return kSucceed;
    }
    
    /// ------ Private member functions: search ----- ///
    int64_t find_key(KeyType& key, ProbeDistanceType* dist, bool is_check_recursively)
    {
      const HashType hash = hash_key(key);
      int64_t pos = cal_desired_pos(hash);
      
      while(true) {
        ProbeDistanceType existing_elem_property = property(pos);
        if (existing_elem_property == kEmptyValue) { /// free space is found
          break;
        } else if (*dist > extract_probedistance(existing_elem_property)) {
          break;
        } else if (!is_deleted(existing_elem_property) && m_key_block_[pos] == key) {
          /// found !
          return pos;
        }
        pos = (pos+1) & kMask;
        *dist = *dist + 1;
      }
      
      /// Find a key from chained RHH
      if (is_check_recursively && m_next_ != nullptr)  {
        RHHStaticType *next_rhh = reinterpret_cast<RHHStaticType *>(m_next_);
        return next_rhh->find_key(key, dist, true);
      }
      
      return kInvaridIndex;
    }
    
    /// ------ Private member functions: utility ----- ///
    
    
    ///  ------------------------------------------------------ ///
    ///              Private Member Variables
    ///  ------------------------------------------------------ ///
  public:
    RHHStaticType* m_next_;
    PropertyBlockType m_property_block_[Capacity];
    KeyType m_key_block_[Capacity];
    ValueType m_value_block_[Capacity];
  };
};

#endif
