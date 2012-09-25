#include <exception>
#include <string>
#include <map>
#include <utility>

#include <boost/python.hpp>
#include <boost/python/suite/indexing/map_indexing_suite.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>
#include <boost/python/return_arg.hpp>

#include <vigra/numpy_array.hxx>
#include <vigra/numpy_array_converters.hxx>
#include <vigra/multi_array.hxx>

#include "../include/pgmlink/track.h"
#include "../include/pgmlink/traxels.h"
#include "../include/pgmlink/field_of_view.h"

namespace Tracking {
    using namespace std;
    using namespace vigra;

    // extending Traxel
    void set_intmaxpos_locator(Traxel& t) {
      Locator* l = new IntmaxposLocator();
      t.set_locator(l); // takes ownership of pointer
    }

    void set_x_scale(Traxel& t, double s) {
      t.locator()->x_scale = s;
    }
    void set_y_scale(Traxel& t, double s) {
      t.locator()->y_scale = s;
    }
    void set_z_scale(Traxel& t, double s) {
      t.locator()->z_scale = s;
    }

    void add_feature_array(Traxel& t, string key, size_t size) {
	    t.features[key] = feature_array(size, 0);
     }

     float get_feature_value(Traxel& t, string key, MultiArrayIndex i) {
	FeatureMap::const_iterator it = t.features.find(key);
	if(it == t.features.end()) {
	   throw std::runtime_error("key not present in feature map");
	}
	if( !(static_cast<size_t>(i) < it->second.size())) {
	     throw std::runtime_error("index out of range");
	 }

	 return it->second[i];
     }

     void set_feature_value(Traxel& t, string key, MultiArrayIndex i, float value) {
	FeatureMap::iterator it = t.features.find(key);
	if(it == t.features.end()) {
	   throw std::runtime_error("key not present in feature map");
	}
	if( !(static_cast<size_t>(i) < it->second.size())) {
	   throw std::runtime_error("index out of range");
	}
	it->second[i] = value;
     }

     // extending Traxels
     void add_traxel(Traxels& ts, const Traxel& t) {
	 ts[t.Id] = t;
     }

  // extending TraxelStore
  void add_traxel_to_traxelstore(TraxelStore& ts, const Traxel& t) {
    add(ts, t);
  }

  void add_Traxels_to_traxelstore(TraxelStore& ts, const Traxels& traxels) {
    for(Traxels::const_iterator it = traxels.begin(); it!= traxels.end(); ++it){
	add(ts, it->second);
    }
  }
}


BOOST_PYTHON_MODULE( ctracking )
{
    using namespace boost::python;
    using namespace Tracking;

    // field_of_view.h
    class_< FieldOfView >("FieldOfView")
      .def(init<double, double, double, double, double, double, double, double>(
	   args("lt","lx","ly","lz","ut","ux","uy","uz")))
      .def("set_boundingbox", &FieldOfView::set_boundingbox, return_self<>())
      //.def("contains", &FieldOfView::contains)
      //.def("lower_bound", &FieldOfView::lower_bound, return_value_policy<copy_const_reference>())
      //.def("upper_bound", &FieldOfView::upper_bound, return_value_policy<copy_const_reference>())
      //.def("spatial_margin", &FieldOfView::spatial_margin)
      //.def("temporal_margin", &FieldOfView::temporal_margin)
    ;
    
    // traxels.h
    class_< feature_array >("feature_array");

    class_< ComLocator >("ComLocator")
      .def_readwrite("x_scale", &ComLocator::x_scale)
      .def_readwrite("y_scale", &ComLocator::y_scale)
      .def_readwrite("z_scale", &ComLocator::z_scale)
    ;

    class_< IntmaxposLocator >("IntmaxposLocator")
      .def_readwrite("x_scale", &ComLocator::x_scale)
      .def_readwrite("y_scale", &ComLocator::y_scale)
      .def_readwrite("z_scale", &ComLocator::z_scale)
    ;

    class_< std::map<std::string,feature_array> >("FeatureMap")
	.def(map_indexing_suite<std::map<std::string,feature_array> >())
    ;
    class_<Traxel>("Traxel")
	.def_readwrite("Id", &Traxel::Id)
	.def_readwrite("Timestep", &Traxel::Timestep)
        //.def("set_locator", &Traxel::set_locator, return_self<>())
        .def("set_x_scale", &set_x_scale)
        .def("set_y_scale", &set_y_scale)
        .def("set_z_scale", &set_z_scale)
        .def("set_intmaxpos_locator", &set_intmaxpos_locator, args("self"))
        .def("X", &Traxel::X)
        .def("Y", &Traxel::Y)
        .def("Z", &Traxel::Z)
	.def_readwrite("features", &Traxel::features)
        .def("add_feature_array", &add_feature_array, args("self","name", "size"), "Add a new feature array to the features map; initialize with zeros. If the name is already present, the old feature array will be replaced.")
	.def("get_feature_value", &get_feature_value, args("self", "name", "index"))
	.def("set_feature_value", &set_feature_value, args("self", "name", "index", "value"))
    ;

    class_<map<unsigned int, Traxel> >("Traxels")
	.def("add_traxel", &add_traxel)
	.def("__len__", &Traxels::size)
    ;

    class_< std::vector<double> >("VectorOfDouble")
    .def(vector_indexing_suite< std::vector<double> >() )
    ;

    class_<TraxelStore>("TraxelStore")
      .def("add", &add_traxel_to_traxelstore)
      .def("add_from_Traxels", &add_Traxels_to_traxelstore)
      .def("bounding_box", &bounding_box)
      ;

    // track.h
    class_<vector<Event> >("EventVector")
	.def(vector_indexing_suite<vector<Event> >())
    ;

    class_<vector<vector<Event> > >("NestedEventVector")
	.def(vector_indexing_suite<vector<vector<Event> > >())
    ;

    class_<map<unsigned int, bool> >("DetectionMap")
      .def(map_indexing_suite<map<unsigned int, bool> >())
    ;

    class_<vector<map<unsigned int, bool> > >("DetectionMapsVector")
      .def(vector_indexing_suite<vector<map<unsigned int, bool> > >())
    ;

    class_<MrfTracking>("MrfTracking", 
			init<string,double,double,double,double,bool,double,double,bool,bool,double,double,double>(
								     args("random_forest_filename", "appearance", "disappearance", "detection", "misdetection", "use_random_forest", "opportunity_cost", "forbidden_cost", "with_constraints", "fixed_detections", "mean_div_dist", "min_angle", "ep_gap")))
      .def("__call__", &MrfTracking::operator())
      .def("detections", &MrfTracking::detections) 
    ;

    enum_<Event::EventType>("EventType")
	.value("Move", Event::Move)
	.value("Division", Event::Division)
	.value("Appearance", Event::Appearance)
	.value("Disappearance", Event::Disappearance)
	.value("Void", Event::Void)
    ;

    class_<vector<unsigned int> >("IdVector")
	.def(vector_indexing_suite<vector<unsigned int> >())
    ;
    class_<Event>("Event")
	.def_readonly("type", &Event::type)
	.def_readonly("traxel_ids", &Event::traxel_ids)
	.def_readonly("energy", &Event::energy)
    ;
}
