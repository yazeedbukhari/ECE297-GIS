#ifndef SAMIRISTHEGOAT_H
#define SAMIRISTHEGOAT_H

#include "m4.h"
#include "ezgl/application.hpp"
#include "ezgl/graphics.hpp"
#include <sstream>
#include <string>
#include <thread>
#include <set>
#include <list>
#include <queue>
#include "LatLon.h"

#define BIGNUMBER 0x3F3F3F3F
#define SOURCE_EDGE -1
struct Intersection_data {
   ezgl::point2d xy_loc; 
   LatLon position;
   std::string name;
   bool highlight = false;
   bool path = false;
};
struct POI_data {
   ezgl::point2d xy_loc; 
   LatLon position;
   std::string name;
   std::string type;
};
struct featureStruct {
   std::vector <ezgl::point2d>  featurePoints;
   int numFeaturePoints;
   double area;
   std::string type;
   double max_x;
   double max_y;
   double min_x;
   double min_y;
};
struct citiesBool {
   bool toronto = false;
   bool beijing = false;
   bool cairo = false;
   bool capeTown = false;
   bool goldenHorseShoe = false;
   bool hamilton = false;
   bool hongKong = false;
   bool iceland = false;
   bool interLaken = false;
   bool kyiv = false;
   bool london = false;
   bool newDelhi = false;
   bool newYork = false;
   bool rio = false;
   bool saintHelena = false;
   bool singapore = false;
   bool sydney = false;
   bool tehran = false;
   bool tokyo = false;
};

struct poiTypeFilterBool {
   bool Food = false;
   bool Education = false;
   bool Transportation = false;
   bool Financial = false;
   bool Healthcare = false;
   bool Entertainment = false;
   bool Public = false;
   bool All = false;
};
struct WaveElem {
   IntersectionIdx nodeID;
   StreetSegmentIdx edgeID;
   double travelTime;
   double totalTimeEstimation;
   WaveElem(int n, int e, double time, double timeEstimation){
      nodeID = n;
      edgeID = e;
      travelTime = time;
      totalTimeEstimation = timeEstimation;
   }
};
struct Node {
   double bestTime = BIGNUMBER;
   StreetSegmentIdx reachingEdge;

};
struct timeWaveElemComparator {
   bool operator()(const WaveElem& a, const WaveElem& b) const {
      return std::greater<double>()(a.totalTimeEstimation, b.totalTimeEstimation);
   }
};
struct CourierPath {
   CourierSubPath courierSubPath;
   double subPathTime;
   std::string destType;
}; 
struct DeliveryInfBool{
    //The intersection id where the item-to-be-delivered is picked-up.
    IntersectionIdx pickUp;
    bool pickUpBool = false;
    //The intersection id where the item-to-be-delivered is dropped-off.
    IntersectionIdx dropOff;
    bool dropOffBool = false;
};
double x_from_lon(float lon);
double y_from_lat(float lat);
double lon_from_x(float x);
double lat_from_y(float y);
bool areaCompare(featureStruct f1, featureStruct f2);
std::string getOSMWayTagValue(OSMID wayOSMID, std::string key);
void displayPath(std::vector <StreetSegmentIdx> streetSegmentPathVector, ezgl::renderer *g);




extern std::vector<Intersection_data> intersections_xyposname;
extern ezgl::application* applicationPtr;
extern std::unordered_map< OSMID , std::vector <std::pair<std::string, std::string>>>OSMNodesandTags;
extern std::unordered_map< OSMID , std::vector <std::pair<std::string, std::string>>>OSMWaysandTags;
extern std::vector <std::vector <ezgl::point2d>> streetSegmentIdx_point2dxyCurvepoints;
extern std::vector<StreetSegmentInfo> street_segment_info;
extern std::vector<POI_data> poi_information;
extern std::vector <featureStruct> Features;
extern std::vector <int> cityIndexes;
extern std::vector <Node> nodes;
extern std::vector <bool> pathGlobalBool;
extern std::vector <StreetSegmentIdx> pathGlobal;
extern double max_lat;
extern double min_lat;
extern double max_lon;
extern double min_lon;
extern double avg_lat;
extern float max_speed_limit;

extern std::stringstream directionsText;

#endif //SAMIRISTHEGOAT_H
