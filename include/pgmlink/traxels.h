/**
   @file
   @ingroup tracking
   @brief tracking datastructures
*/

#ifndef TRAXELS_H
#define TRAXELS_H

#include <set>
#include <string>
#include <iostream>
#include <ostream>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/map.hpp>

#include "pgmlink/pgmlink_export.h"

namespace pgmlink {
//
// feature data structures
//
typedef float feature_type;
typedef std::vector<feature_type> feature_array;
typedef std::map<std::string,feature_array> FeatureMap;



//
// retrieve spatial coordinates from features
//
class PGMLINK_EXPORT Locator {
 public:
 Locator( std::string fn,
	  double x_scale = 1.0,
	  double y_scale = 1.0,
	  double z_scale = 1.0 ) 
   : x_scale(x_scale), y_scale(y_scale), z_scale(z_scale), feature_name_(fn) {};
  virtual Locator* clone() = 0;
  virtual ~Locator() {};

  virtual bool is_applicable(const FeatureMap& m) const { return m.count(feature_name_)==1; };
  virtual double X(const FeatureMap&) const = 0;
  virtual double Y(const FeatureMap&) const = 0;
  virtual double Z(const FeatureMap&) const = 0;

  double x_scale, y_scale, z_scale;

 protected:
  std::string feature_name_;
  double coordinate_from(const FeatureMap&, size_t idx) const;

 private:
  // boost serialize
  friend class boost::serialization::access;
  template< typename Archive >
    void serialize( Archive&, const unsigned int /*version*/ );
};

class PGMLINK_EXPORT ComLocator : public Locator {
 public:
 ComLocator() : Locator("com") {};
  virtual ComLocator* clone() { return new ComLocator(*this); };
  double X(const FeatureMap& m) const { return x_scale * coordinate_from(m, 0); };
  double Y(const FeatureMap& m) const { return y_scale * coordinate_from(m, 1); };
  double Z(const FeatureMap& m) const { return z_scale * coordinate_from(m, 2); };

 private:
  // boost serialize
  friend class boost::serialization::access;
  template< typename Archive >
    void serialize( Archive&, const unsigned int /*version*/ );
};

class PGMLINK_EXPORT IntmaxposLocator : public Locator {
 public:
 IntmaxposLocator() : Locator("intmaxpos") {};
  virtual IntmaxposLocator* clone() { return new IntmaxposLocator(*this); };
  double X(const FeatureMap& m) const { return x_scale * coordinate_from(m, 1); };
  double Y(const FeatureMap& m) const { return y_scale * coordinate_from(m, 2); };
  double Z(const FeatureMap& m) const { return z_scale * coordinate_from(m, 3); };

 private:
  // boost serialize
  friend class boost::serialization::access;
  template< typename Archive >
    void serialize( Archive&, const unsigned int /*version*/ );  
};



//
// Traxel datatype
//
 class PGMLINK_EXPORT Traxel {
 public:
   // construction / assignment
   //takes ownership of locator pointer
   Traxel(unsigned int id = 0, int timestep = 0, FeatureMap fmap = FeatureMap(), Locator* l = new ComLocator()) :
	   Id(id), Timestep(timestep), features(fmap), locator_(l) {};
   Traxel(const Traxel& other);
   Traxel& operator=(const Traxel& other);
   ~Traxel() { delete locator_; };
   Traxel& set_locator(Locator*);
   Locator* locator() {return locator_;}
   
   // fields
   unsigned int Id; // id of connected component (aka "label")
   int Timestep; // traxel occured after
   FeatureMap features;
   
   // position according to locator
   double X() const;
   double Y() const;
   double Z() const;

   // relation to other traxels
   double distance_to(const Traxel& other) const;
   double angle(const Traxel& leg1, const Traxel& leg2) const;
   friend std::ostream& operator<< (std::ostream &out, const Traxel &t);

 private:
   // boost serialize for Traxel datatype
   friend class boost::serialization::access;
   template< typename Archive >
     void serialize( Archive&, const unsigned int /*version*/ );

   Locator* locator_;
 };

 // compare by (time,id) (Traxels can be used as keys (for instance in a std::map) )
 PGMLINK_EXPORT bool operator<(const Traxel& t1, const Traxel& t2);
 PGMLINK_EXPORT bool operator>(const Traxel& t1, const Traxel& t2);



 //
 // Traxel collections
 //
 
 // Traxel map
 typedef std::map<unsigned int, Traxel> Traxels;
 template<typename InputIterator>
   Traxels traxel_map_from_traxel_sequence(InputIterator begin, InputIterator end);
 
 // Traxel key-value store
 using namespace boost;
 using namespace boost::multi_index;

 //tags
 struct PGMLINK_EXPORT by_timestep {};
 struct PGMLINK_EXPORT by_timeid {};

 typedef PGMLINK_EXPORT multi_index_container<
   Traxel,
     indexed_by<
       ordered_non_unique<tag<by_timestep>, member<Traxel,int,&Traxel::Timestep> >,
       hashed_unique<
	   tag<by_timeid>,
	   composite_key<
	       Traxel,
	       member<Traxel, int, &Traxel::Timestep>,
	       member<Traxel, unsigned int, &Traxel::Id>
	   > 
	> 
     >
   > 
   TraxelStore;
 typedef PGMLINK_EXPORT TraxelStore::index<by_timestep>::type
   TraxelStoreByTimestep;
 typedef PGMLINK_EXPORT TraxelStore::index<by_timeid>::type
   TraxelStoreByTimeid;

 //
 // TraxelStore functions 
 //
 /**
  * Tight bounding box surrounding the Traxels in store.
  * @return lt lx ly lz ut ux uy uz 
  */
 PGMLINK_EXPORT std::vector<double> bounding_box(const TraxelStore&);

 // timesteps
 PGMLINK_EXPORT std::set<TraxelStoreByTimestep::key_type>
   timesteps(const TraxelStore&);

 PGMLINK_EXPORT TraxelStoreByTimestep::key_type
   earliest_timestep(const TraxelStore&);

 PGMLINK_EXPORT TraxelStoreByTimestep::key_type
   latest_timestep(const TraxelStore&);

 // io
 PGMLINK_EXPORT TraxelStore& add(TraxelStore&, const Traxel&);

 template<typename InputIt>
   TraxelStore& add(TraxelStore&, InputIt begin, InputIt end);

 std::vector<std::vector<Traxel> > nested_vec_from(const TraxelStore&);

 /** 
  * Filter by field of fiew. 
  * This function adds Traxels from in to out, that are contained in the field of fiew.
  * @return the number of Traxels in the field of view
  */
 class FieldOfView;
 PGMLINK_EXPORT size_t filter_by_fov( const TraxelStore& in, TraxelStore& out, const FieldOfView& );



/**/
/* implementation */
/**/

template< typename Archive >
void Locator::serialize( Archive& ar, const unsigned int /*version*/ ) {
  ar & x_scale;
  ar & y_scale;
  ar & z_scale;
  ar & feature_name_;
}
template< typename Archive >
void ComLocator::serialize( Archive& ar, const unsigned int /*version*/ ) {
  ar & boost::serialization::base_object<Locator>(*this);
}
template< typename Archive >
void IntmaxposLocator::serialize( Archive& ar, const unsigned int /*version*/ ) {
  ar & boost::serialization::base_object<Locator>(*this);
}

template< typename Archive >
void Traxel::serialize( Archive& ar, const unsigned int /*version*/ ) {
  ar.template register_type<ComLocator>();
  ar.template register_type<IntmaxposLocator>();

  ar & Id;
  ar & Timestep;
  ar & features;
  ar & locator_;
}

template<typename InputIterator>
Traxels traxel_map_from_traxel_sequence(InputIterator begin, InputIterator end) {
    Traxels ret;
    for(;begin != end; ++begin) {
	ret[begin->Id] = *begin;
    }
    return ret;
}

template<typename InputIt>
  TraxelStore& add(TraxelStore& ts, InputIt begin, InputIt end) {
  ts.get<by_timestep>().insert(begin, end);
  return ts;
}

} /* namespace pgmlink */


#endif /* TRAXELS_H */
