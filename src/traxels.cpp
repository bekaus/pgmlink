#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <set>
#include <vector>
#include "pgmlink/traxels.h"
#include "pgmlink/field_of_view.h"

using namespace std;

namespace pgmlink {
  ////
  //// class Locator
  ////
  double Locator::coordinate_from(const FeatureMap& m, size_t idx) const {
    if(is_applicable(m)) {
      FeatureMap::const_iterator it;
      it = m.find(feature_name_);
      return it->second[idx];
    } else {
      throw invalid_argument("Locator::coordinate_from(): FeatureMap is not applicable");    
    }
  }
    

  ////
  //// class Traxel
  ////
  Traxel::Traxel(const Traxel& other): Id(other.Id), Timestep(other.Timestep), features(other.features) {
    locator_ = other.locator_->clone();
  }

  Traxel& Traxel::operator=(const Traxel& other) {
    Id = other.Id;
    Timestep = other.Timestep;
    features = other.features;
    // This gracefully handles self assignment
    Locator* temp = other.locator_->clone();
    delete locator_;
    locator_ = temp;
    return *this;
  }

  Traxel& Traxel::set_locator(Locator* l) {
    delete locator_;
    locator_ = l;
    return *this;
  }

  double Traxel::X() const {
    return locator_->X(features);
  }

  double Traxel::Y() const {
    return locator_->Y(features);
  }

  double Traxel::Z() const {
    return locator_->Z(features);
  }

  namespace {
    double dot(double x1,double y1,double z1, double x2,double y2,double z2) {
      return x1*x2 + y1*y2 + z1*z2;
    }
    double norm(double x,double y,double z) {
      return sqrt(dot(x,y,z, x,y,z));
    }
  }

  double Traxel::distance_to(const Traxel& other) const {
    return norm(other.X()-X(), other.Y()-Y(), other.Z()-Z());
  }

  double Traxel::angle(const Traxel& leg1, const Traxel& leg2) const {
    double x0 = this->X();
    double y0 = this->Y();
    double z0 = this->Z();

    double x1 = leg1.X();
    double y1 = leg1.Y();
    double z1 = leg1.Z();

    double x2 = leg2.X();
    double y2 = leg2.Y();
    double z2 = leg2.Z();

    double dx1 = x1 - x0;
    double dy1 = y1 - y0;
    double dz1 = z1 - z0;

    double dx2 = x2 - x0;
    double dy2 = y2 - y0;
    double dz2 = z2 - z0;

    return std::acos((dot(dx1, dy1, dz1, dx2, dy2, dz2))/(norm(dx1, dy1, dz1)*norm(dx2,dy2,dz2)));
  }

  std::ostream& operator<< (std::ostream &out, const Traxel &t) {
    out << "Traxel("<< t.Id << ", " << t.Timestep << ")";
    return out;
  }

  bool operator<(const Traxel& t1, const Traxel& t2) {
    if(t1.Timestep < t2.Timestep) {
	    return true;
    } else if(t1.Timestep == t2.Timestep) {
	    if(t1.Id < t2.Id) return true;
	    else return false;
    } else {
	return false;
    }
  }
  bool operator>(const Traxel& t1, const Traxel& t2) {
    return t2 < t1;
  }

  bool operator==(const Traxel& t1, const Traxel& t2) {
    return !(t2 < t1) && !(t1 < t2);
  }


  //
  // type TraxelStore
  //
  std::vector<double> bounding_box(const TraxelStore& ts) {
    std::vector<double> bb(8,0);
    double lt,lx,ly,lz,ut,ux,uy,uz;
    lt=0; lx=1; ly=2; lz=3; ut=4; ux=5; uy=6; uz=7;

    TraxelStoreByTimestep::iterator it=ts.get<by_timestep>().begin();
    bb[lt] = static_cast<double>(it->Timestep);
    bb[lx] = it->X();
    bb[ly] = it->Y();
    bb[lz] = it->Z();
    bb[ut] = static_cast<double>(it->Timestep);
    bb[ux] = it->X();
    bb[uy] = it->Y();
    bb[uz] = it->Z();
    ++it;

    for(;
	it!=ts.get<by_timestep>().end(); ++it) {
      bb[lt] = min(bb[lt], static_cast<double>(it->Timestep));
      bb[lx] = min(bb[lx], it->X());
      bb[ly] = min(bb[ly], it->Y());
      bb[lz] = min(bb[lz], it->Z());
      bb[ut] = max(bb[ut], static_cast<double>(it->Timestep));
      bb[ux] = max(bb[ux], it->X());
      bb[uy] = max(bb[uy], it->Y());
      bb[uz] = max(bb[uz], it->Z());
    }

    return bb;
  }

  namespace {
    template<typename ordered_index_t>
    set<typename ordered_index_t::key_type> keys_in(const ordered_index_t& i) {
      set<typename ordered_index_t::key_type> keys;
      typename ordered_index_t::key_from_value key = i.key_extractor();
      for(typename ordered_index_t::iterator it=i.begin();
	  it!=i.end(); ++it) {
	keys.insert(key(*it));
      }
      return keys;
    }
  }

  std::set<TraxelStoreByTimestep::key_type>
  timesteps(const TraxelStore& t) {
    return keys_in(t.get<by_timestep>());
  }

 TraxelStoreByTimestep::key_type
 earliest_timestep(const TraxelStore& t) {
    return *(timesteps(t).begin());
 }

 TraxelStoreByTimestep::key_type
 latest_timestep(const TraxelStore& t) {
    return *(timesteps(t).rbegin());
 }
  
  TraxelStore& add(TraxelStore& ts, const Traxel& t) {
    ts.get<by_timestep>().insert(t);
    return ts;
  }

  std::vector<std::vector<Traxel> > nested_vec_from(const TraxelStore& t) {
    // determine offset and range of timesteps
    TraxelStoreByTimestep::key_type offset = earliest_timestep(t);
    size_t range = latest_timestep(t) - offset + 1;

    // construct nested vec
    vector<vector<Traxel> > ret(range);
    TraxelStoreByTimestep::key_from_value key = t.get<by_timestep>().key_extractor();
    for(TraxelStoreByTimestep::iterator it=t.get<by_timestep>().begin();
	it!=t.get<by_timestep>().end(); ++it) {
      ret[key(*it) - offset].push_back(*it);
    }
    
    return ret;
  }

  size_t filter_by_fov( const TraxelStore& in, TraxelStore& out, const FieldOfView& fov ) {
    size_t n = 0;
    for(TraxelStore::iterator it = in.begin(); it != in.end(); ++it) {
      if(fov.contains(it->Timestep, it->X(), it->Y(), it->Z())) {
	  add(out, *it);
	  ++n;
	}
    }
    return n;
  }
} /* namespace pgmlink */

