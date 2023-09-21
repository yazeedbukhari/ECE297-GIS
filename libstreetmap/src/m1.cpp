/* 
 * Copyright 2023 University of Toronto
 *
 * Permission is hereby granted, to use this software and associated 
 * documentation files (the "Software") in course work at the University 
 * of Toronto, or for personal use. Other uses are prohibited, in 
 * particular the distribution of the Software either publicly or to third 
 * parties.
 *
 * The above copyright notice and this permission notice shall be included in 
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include <iostream>
#include "m1.h"
#include "StreetsDatabaseAPI.h"
#include <cmath>
#include <algorithm>
#include "OSMDatabaseAPI.h"
#include "ezgl/application.hpp"
#include "ezgl/graphics.hpp"
#include "samiristhegoat.h"
#include "ezgl/point.hpp"

std::string toLowerString(std::string);
std::string shortVersion(std::string input, int size);
void sortPOITypes(std::string type, int poiID);

// loadMap will be called with the name of the file that stores the "layer-2"
// map data accessed through StreetsDatabaseAPI: the street and intersection 
// data that is higher-level than the raw OSM data). 
// This file name will always end in ".streets.bin" and you 
// can call loadStreetsDatabaseBIN with this filename to initialize the
// layer 2 (StreetsDatabase) API.
// If you need data from the lower level, layer 1, API that provides raw OSM
// data (nodes, ways, etc.) you will also need to initialize the layer 1 
// OSMDatabaseAPI by calling loadOSMDatabaseBIN. That function needs the 
// name of the ".osm.bin" file that matches your map -- just change 
// ".streets" to ".osm" in the map_streets_database_filename to get the proper
// name.

//vector of intersections with vectors of street segments
std::vector <std::vector <StreetSegmentIdx>> intersection_street_segments;

//vector of streets with vectors of street segments 
std::vector <std::vector <StreetSegmentIdx>> street_street_segments;

//vector of streets with set of intersections
std::vector <std::set <IntersectionIdx>> street_intersections; 

//vector of streets with vectors of street lengths
std::vector <std::vector <double>> street_lengths;

//vector of streets segments with vectors of street segment lengths
std::vector<double> street_segment_length;

//vector of street segments with vectors streetsegmentinfo objects
std::vector<StreetSegmentInfo> street_segment_info;

//Map of OSMid, and vector of pairs with strings key, and value
std::unordered_map< OSMID , std::vector <std::pair<std::string, std::string>>>OSMNodesandTags;

//Map of OSMid, and vector of pairs with strings key, and value
std::unordered_map< OSMID , std::vector <std::pair<std::string, std::string>>>OSMWaysandTags;

//Map of combination of three letter and streetsegments
std::unordered_map < std::string , std::vector<StreetIdx> > allStreetsKeys;

//Vector of intersecion_data (latlon position and intersection names)
std::vector<Intersection_data> intersections_xyposname;

//Vector of featurepoint IDs with all feature points in point2d x and y

std::vector <featureStruct> Features;

//Vector of streetSegmentIDs with all curve points associated with street segment in point2dxy
std::vector <std::vector <ezgl::point2d>> streetSegmentIdx_point2dxyCurvepoints;

// Vector of POI_data (latlon position, names and type etc)
std::vector<POI_data> poi_information;

//Vectors of City Names and Locations (String, Point2D)

std::vector <int> cityIndexes;
//Vector of Nodes
std::vector <Node> nodes;

//Vector of flags that indicate which street segments are part of the path
std::vector <bool> pathGlobalBool;

//GLOBAL VARIABLES
double max_lat = 0;
double min_lat = 0;
double max_lon = 0;
double min_lon = 0;
double avg_lat = 0;
float max_speed_limit;

bool loadMap(std::string map_streets_database_filename) {
    
    auto startTime = std::chrono::high_resolution_clock::now();

    //load the streets database and pass result into boolean flag (load_successful)
    bool load_successful = loadStreetsDatabaseBIN(map_streets_database_filename);

    
    std::string OSMFileName = map_streets_database_filename;
    
    if(map_streets_database_filename.find(".streets.bin", 0) < map_streets_database_filename.size()){
        for(int popbacknum = 0; popbacknum < 11; popbacknum++){
            OSMFileName.pop_back();
        }
        OSMFileName.append("osm.bin");
    }

    

    bool load_osm_successful = loadOSMDatabaseBIN(OSMFileName);

    if (load_successful && load_osm_successful) {

        //resizing the vectors to the appropriate size
        intersection_street_segments.resize(getNumIntersections());
        street_street_segments.resize(getNumStreets());
        street_intersections.resize(getNumStreets());
        street_segment_info.resize(getNumStreetSegments());
        street_segment_length.resize(getNumStreetSegments());
        street_segment_info.resize(getNumStreetSegments());
        street_segment_length.resize(getNumStreetSegments());
        street_lengths.resize(getNumStreets());
        intersections_xyposname.resize(getNumIntersections());
        streetSegmentIdx_point2dxyCurvepoints.resize(getNumStreetSegments());  
        Features.resize(getNumFeatures());
        pathGlobalBool.resize(getNumStreetSegments());
        nodes.resize(getNumIntersections());

        std::thread t1 {[](){
            //Vector of intersections with accompanying street segments (intersection_street_segments)
            //populating the intersection_street_segments nested vector with all intersections(+their connected intersections)
            for (int intersection = 0; intersection < getNumIntersections(); ++intersection) {  //iterating over all the intersections in the map
                for (int numIntersection = 0; numIntersection < getNumIntersectionStreetSegment(intersection); ++numIntersection) {       //iterating over all the street segments connected to the current intersection
                    int ss_id = getIntersectionStreetSegment(intersection, numIntersection); 
                    intersection_street_segments[intersection].push_back(ss_id);
                }
            }
        }};
        std::thread t2 {[](){
            //Vector of streets with accompanying street segments (street_street_segments)
            //populating the street_street_segments nested vector with all streets(+ their associated street segments)
            for (int streetSegment = 0; streetSegment < getNumStreetSegments(); ++streetSegment) {      //iterating over all street segments in the map
                StreetSegmentInfo currentStreetSegment = getStreetSegmentInfo(streetSegment);           
                street_street_segments[currentStreetSegment.streetID].push_back(streetSegment);
            }

            //Vector of streets with accompanying intersections (street_intersections)
            //populating the street_intersections nested vector with all streets(+ their associated intersections)
            for (int street = 0; street < getNumStreets(); ++street) {                                                          //iterating over all streets in the map
                for (int streetSegments = 0; streetSegments < street_street_segments[street].size(); ++streetSegments) {        //iterating over all the street segments in the map
                    StreetSegmentInfo currentSegment = getStreetSegmentInfo(street_street_segments[street][streetSegments]);    //storing the getStreetInfo function return in currentSegment object                                              

                            street_intersections[street].insert(currentSegment.from);                                        //store in vector if searchfrom == false

                            street_intersections[street].insert(currentSegment.to); 
                    
                }
            }
            //Vector of streets with accompanying street lengths (street_lengths)
            //populating the street_lengths nested vector with all streets(+ their accompanying lengths)
            for (int street = 0; street < getNumStreets(); street++) {
                double streetLength = 0;
                for (int streetSegments = 0; streetSegments < street_street_segments[street].size(); streetSegments++) {
                    streetLength += findStreetSegmentLength(street_street_segments[street][streetSegments]);            //iterate through the street segments of a given street and add the lengths
                }
                street_lengths[street].push_back(streetLength);                                                         //store street length in the vector
            }

        }};

        std::thread t4 {[](){
             max_speed_limit = 0;
            //Vector of streets segments with accompanying street segment info (street_segment_info)
            //populating the street_segment_info nested vector with all street segments(+ their associated segment info)
            for (int StreetSegment = 0; StreetSegment < getNumStreetSegments(); StreetSegment++) {
                street_segment_info[StreetSegment] = getStreetSegmentInfo(StreetSegment);           //store street segment info in vector
                street_segment_length[StreetSegment] = findStreetSegmentLength(StreetSegment);      //store street segment length in vector
                
                max_speed_limit = std::max(max_speed_limit, getStreetSegmentInfo(StreetSegment).speedLimit);
                
            }
        }};

        std::thread t6 {[](){
            //map of osmID to key-tagvalue pair
            for(int nodeNumber = 0; nodeNumber < getNumberOfNodes(); nodeNumber++){
                std::vector <std::pair <std::string, std:: string>> vectorPairs;
                for(int tagNumber = 0; tagNumber < getTagCount(getNodeByIndex(nodeNumber)); tagNumber++){
                    vectorPairs.push_back( getTagPair(getNodeByIndex(nodeNumber), tagNumber));
                    std::string key, tag;
                    std::tie(key, tag) = getTagPair(getNodeByIndex(nodeNumber), tagNumber);
                    std::pair <std::string, std::string> keyTagPair = getTagPair(getNodeByIndex(nodeNumber), tagNumber);
                    //if the given node is of key place and tag city
                    if (keyTagPair.first == "place" && keyTagPair.second == "city") {
                        cityIndexes.push_back(nodeNumber);
                    }

                }
                OSMNodesandTags.insert(std::make_pair(getNodeByIndex(nodeNumber)->id(), vectorPairs));
            }
        }};
        std::thread t7 {[](){
            //populating hashmap of alphabetically ordered streedIDs via name
            for(int street = 0; street < getNumStreets(); street++){
                std::string streetName = getStreetName(street);
                streetName.erase( std::remove( streetName.begin(), streetName.end(), ' ' ), streetName.end() );
                streetName = toLowerString(streetName);
                for(int letterIndex = 1; letterIndex <= streetName.size(); letterIndex++){
                    if(std::find(allStreetsKeys[shortVersion(streetName, letterIndex)].begin(), allStreetsKeys[shortVersion(streetName, letterIndex)].end(), street) == allStreetsKeys[shortVersion(streetName, letterIndex)].end())
                        allStreetsKeys[shortVersion(streetName, letterIndex)].push_back(street);
                }
                
            }
        }};
        std::thread t8 {[](){
            //M2 PREPROCESSING
            max_lat = getIntersectionPosition(0).latitude();
            min_lat = max_lat;
            max_lon = getIntersectionPosition(0).longitude();
            min_lon = max_lon;

            
            for (int intersectionID = 0; intersectionID < getNumIntersections(); ++intersectionID) {
                intersections_xyposname[intersectionID].position = getIntersectionPosition(intersectionID);
                intersections_xyposname[intersectionID].name = getIntersectionName(intersectionID);


                max_lat = std::max(max_lat, getIntersectionPosition(intersectionID).latitude());
                min_lat = std::min(min_lat, getIntersectionPosition(intersectionID).latitude());
                max_lon = std::max(max_lon, getIntersectionPosition(intersectionID).longitude());
                min_lon = std::min(min_lon, getIntersectionPosition(intersectionID).longitude());
            }

            avg_lat = (min_lat + max_lat)/2;

            for (int intersectionID = 0; intersectionID < getNumIntersections(); ++intersectionID) {
                double x = x_from_lon(getIntersectionPosition(intersectionID).longitude());
                double y = y_from_lat(getIntersectionPosition(intersectionID).latitude());

                intersections_xyposname[intersectionID].xy_loc = ezgl::point2d(x,y);
            }
        }};
        t8.join();
        std::thread t9 {[](){
            //Vector of featurepoint IDs with all feature points in point2d x and y    
            for(int featureID = 0; featureID < getNumFeatures(); ++featureID){
                Features[featureID].max_x = x_from_lon(getFeaturePoint(featureID, 0).longitude());
                Features[featureID].min_x = Features[featureID].max_x;
                Features[featureID].max_y = y_from_lat(getFeaturePoint(featureID, 0).latitude());
                Features[featureID].min_y = Features[featureID].max_y;
                Features[featureID].area = findFeatureArea(featureID);
                for(int featurePointNum = 0; featurePointNum < getNumFeaturePoints(featureID); ++featurePointNum){
                    double x = x_from_lon(getFeaturePoint(featureID, featurePointNum).longitude());
                    double y = y_from_lat(getFeaturePoint(featureID, featurePointNum).latitude());
                    Features[featureID].featurePoints.push_back(ezgl::point2d(x,y));
                    Features[featureID].numFeaturePoints = featurePointNum;
                    Features[featureID].type = asString(getFeatureType(featureID));
                    Features[featureID].max_x = std::max(Features[featureID].max_x, x);
                    Features[featureID].min_x = std::min(Features[featureID].min_x, x);
                    Features[featureID].max_y = std::max(Features[featureID].max_y, y);
                    Features[featureID].min_y = std::min(Features[featureID].min_y, y);
                }
            }
            std::sort(Features.begin(), Features.end(), areaCompare);
        }};
        std::thread t10 {[](){
            //Vector of street seg ids with curve points in point2dx
            for(int streetSegmentID = 0; streetSegmentID < getNumStreetSegments(); ++streetSegmentID){
                int num_curvepoints = street_segment_info[streetSegmentID].numCurvePoints;
                LatLon  start_point = getIntersectionPosition(street_segment_info[streetSegmentID].from),
                        end_point = getIntersectionPosition(street_segment_info[streetSegmentID].to);

                
                double x = x_from_lon(start_point.longitude());
                double y = y_from_lat(start_point.latitude());
                streetSegmentIdx_point2dxyCurvepoints[streetSegmentID].push_back(ezgl::point2d(x,y)); 
            
                
                for (int curvePointNum = 0; curvePointNum < num_curvepoints; curvePointNum++) {
                    x = x_from_lon(getStreetSegmentCurvePoint(streetSegmentID, curvePointNum).longitude());
                    y = y_from_lat(getStreetSegmentCurvePoint(streetSegmentID, curvePointNum).latitude());
                    streetSegmentIdx_point2dxyCurvepoints[streetSegmentID].push_back(ezgl::point2d(x,y)); 
                }

                x = x_from_lon(end_point.longitude());
                y = y_from_lat(end_point.latitude());
                streetSegmentIdx_point2dxyCurvepoints[streetSegmentID].push_back(ezgl::point2d(x,y));

            }
        }};
        std::thread t11 {[](){
            // initialize required poi data
            int numPOIS = getNumPointsOfInterest();
            poi_information.resize(numPOIS);

            for (int poiID = 0; poiID < numPOIS; poiID++) {
                poi_information[poiID].position = getPOIPosition(poiID);

                double x = x_from_lon(poi_information[poiID].position.longitude());
                double y = y_from_lat(poi_information[poiID].position.latitude());

                poi_information[poiID].xy_loc = ezgl::point2d(x,y);

                poi_information[poiID].name = getPOIName(poiID);
                
                //poi_information[poiID].type = getPOIType(poiID);
                sortPOITypes(getPOIType(poiID), poiID);
                    
            }
        }};
        std::thread t12 {[](){
            //map of osmID to key-tagvalue pair
            for(int wayNumber = 0; wayNumber < getNumberOfWays(); wayNumber++){
                std::vector <std::pair <std::string, std:: string>> vectorPairs;
                for(int tagNumber = 0; tagNumber < getTagCount(getWayByIndex(wayNumber)); tagNumber++){
                    vectorPairs.push_back( getTagPair(getWayByIndex(wayNumber), tagNumber));
                }
                OSMWaysandTags.insert(std::make_pair(getWayByIndex(wayNumber)->id(), vectorPairs));
            }
        }};
        t1.join();
        t2.join();
        t4.join();
        t6.join();
        t7.join();
        t9.join();
        t10.join();
        t11.join();
        t12.join();

    }
    auto currTime = std::chrono::high_resolution_clock::now();
    auto wallClock = std::chrono::duration_cast<std::chrono::duration<double>>(currTime - startTime);
    std::cout << "Load Map: " << wallClock.count() << std::endl;
    return load_successful;
}

void closeMap() {
    //Clean-up your map related data structures here
    closeStreetDatabase();
    closeOSMDatabase();
    
    intersection_street_segments.clear();           //clearing vectors used by loadMap and functions
    street_intersections.clear();
    street_street_segments.clear();
    street_lengths.clear();
    street_segment_length.clear();
    street_segment_info.clear();
    OSMNodesandTags.clear();
    allStreetsKeys.clear();
    intersections_xyposname.clear();
    streetSegmentIdx_point2dxyCurvepoints.clear();
    OSMWaysandTags.clear();
    Features.clear();
    cityIndexes.clear();
    poi_information.clear();
    pathGlobalBool.clear();
    nodes.clear();
}


// Returns the distance between two (lattitude,longitude) coordinates in meters
// Speed Requirement --> moderategit s
double findDistanceBetweenTwoPoints(LatLon point_1, LatLon point_2) {
 double  lat1 = point_1.latitude() * kDegreeToRadian,
            lat2 = point_2.latitude() * kDegreeToRadian,
            lon1 = point_1.longitude() * kDegreeToRadian,
            lon2 = point_2.longitude() * kDegreeToRadian;
    
    double lat_avg = (lat1 + lat2) * 0.5;

    double cosine = cos(lat_avg);

    double x1 = kEarthRadiusInMeters * lon1 * cosine;
    double y1 = kEarthRadiusInMeters * lat1;

    double x2 = kEarthRadiusInMeters * lon2 * cosine;
    double y2 = kEarthRadiusInMeters * lat2;

    double ret = (y2-y1) * (y2-y1) + (x2-x1) * (x2-x1);
    return sqrt(ret);

}

// Returns the length of the given street segment in meters
// Speed Requirement --> moderate
double findStreetSegmentLength(StreetSegmentIdx street_segment_id) {
   int num_curvepoints = street_segment_info[street_segment_id].numCurvePoints;
    LatLon  start_point = getIntersectionPosition(street_segment_info[street_segment_id].from),
            end_point = getIntersectionPosition(street_segment_info[street_segment_id].to);

    if (num_curvepoints == 0) {
        return findDistanceBetweenTwoPoints( start_point, end_point);       
    }

    double ret = 0;
    
    ret += findDistanceBetweenTwoPoints( start_point, getStreetSegmentCurvePoint(street_segment_id, 0));

    for (int curvePointNum = 0; curvePointNum < num_curvepoints-1; curvePointNum++) {
        ret += findDistanceBetweenTwoPoints( getStreetSegmentCurvePoint(street_segment_id, curvePointNum), getStreetSegmentCurvePoint(street_segment_id, curvePointNum+1)); 
    }

    ret += findDistanceBetweenTwoPoints( getStreetSegmentCurvePoint(street_segment_id, num_curvepoints-1), end_point);

    return ret;
   
}

// Returns the travel time to drive from one end of a street segment 
// to the other, in seconds, when driving at the speed limit
// Note: (time = distance/speed_limit)
// Speed Requirement --> high 
double findStreetSegmentTravelTime(StreetSegmentIdx street_segment_id) {
   double length = street_segment_length[street_segment_id];
    // StreetSegmentInfo street_segment = getStreetSegmentInfo(street_segment_id); // to access street speed limit
    float speed_limit = street_segment_info[street_segment_id].speedLimit; // have smth that saves street segment info when first called in streetsegment length
    double ret = length / speed_limit;
    return ret;
}

// Returns all intersections reachable by traveling down one street segment 
// from the given intersection (hint: you can't travel the wrong way on a 
// 1-way street)
// the returned vector should NOT contain duplicate intersections
// Corner case: cul-de-sacs can connect an intersection to itself 
// (from and to intersection on  street segment are the same). In that case
// include the intersection in the returned vector (no special handling needed).
// Speed Requirement --> high 
std::vector<IntersectionIdx> findAdjacentIntersections(IntersectionIdx intersection_id) {
    std::vector<IntersectionIdx> AdjacentIntersections;
    
    //iterating over all the street segments associated with a given intersection (extracted from intersection_street_segments vector)
    for(int streetSegmentNum = 0; streetSegmentNum < intersection_street_segments[intersection_id].size(); streetSegmentNum++){
        StreetSegmentInfo street_segment = getStreetSegmentInfo(intersection_street_segments[intersection_id][streetSegmentNum]);
        
        if(street_segment.oneWay != true){                                                                                                      
            //if the current street segment is not a one way, store either the "from" or "to" intersection (dependent on which is equal to the given intersection_id) in the return vector
            if(street_segment.from != intersection_id){
                if(std::find(AdjacentIntersections.begin(), AdjacentIntersections.end(), street_segment.from) == AdjacentIntersections.end()) 
                    AdjacentIntersections.push_back(street_segment.from);
            } else if (street_segment.to != intersection_id){
                if(std::find(AdjacentIntersections.begin(), AdjacentIntersections.end(), street_segment.to) == AdjacentIntersections.end())
                    AdjacentIntersections.push_back(street_segment.to);
            } else {
                if(std::find(AdjacentIntersections.begin(), AdjacentIntersections.end(), street_segment.from) == AdjacentIntersections.end())
                    AdjacentIntersections.push_back(street_segment.from);
            }

            //if the current street segment is a one way, store the "to" intersection 
        } else if(street_segment.to != intersection_id){
            if(std::find(AdjacentIntersections.begin(), AdjacentIntersections.end(), street_segment.to) == AdjacentIntersections.end())
                AdjacentIntersections.push_back(street_segment.to);
        }
    }
    return AdjacentIntersections;
}

// Returns tPOItypeuirement --> none
IntersectionIdx findClosestIntersection(LatLon my_position) {
    
    double minDistance = kEarthRadiusInMeters;
    IntersectionIdx minDistanceIntersection = 0;

    //iterate over all the intersections in the map - track the smallest distance using minDistance and minDistanceIntersection
    for(int intersection = 0; intersection < getNumIntersections(); intersection++){
        double distance = 0;
        LatLon intersectionPosition = getIntersectionPosition(intersection);
        distance = findDistanceBetweenTwoPoints(intersectionPosition, my_position);
        if(distance <= minDistance){
            minDistance = distance;
            minDistanceIntersection = intersection;
        }

    }

    return minDistanceIntersection;
}

// Returns the street segments that connect to the given intersection 
// Speed Requirement --> high
std::vector<StreetSegmentIdx> findStreetSegmentsOfIntersection(IntersectionIdx intersection_id) {
    return intersection_street_segments[intersection_id];
}

// Returns all intersections along the given street.
// There should be no duplicate intersections in the returned vector.
// Speed Requirement --> high
std::vector<IntersectionIdx> findIntersectionsOfStreet(StreetIdx street_id) {

    std::vector <IntersectionIdx> street_intersections_vector(street_intersections[street_id].begin(), street_intersections[street_id].end());
    
    return street_intersections_vector;
}

// Return all intersection ids at which the two given streets intersect
// This function will typically return one intersection id for streets
// that intersect and a length 0 vector for streets that do not. For unusual 
// curved streets it is possible to have more than one intersection at which 
// two streets cross.
// There should be no duplicate intersections in the returned vector.
// Speed Requirement --> high
std::vector<IntersectionIdx> findIntersectionsOfTwoStreets(StreetIdx street_id1, StreetIdx street_id2) {
    std::vector <IntersectionIdx> street1Vector(street_intersections[street_id1].begin(), street_intersections[street_id1].end());
    std::vector <IntersectionIdx> street2Vector(street_intersections[street_id2].begin(), street_intersections[street_id2].end());
    std::vector<IntersectionIdx> commonIntersections;
    
    //iterate through the street1 - for each intersection determine if the same intersection_id exists in street2
    for (int intersection = 0; intersection < street1Vector.size(); ++intersection) {
        int commonIntersection = street1Vector[intersection];
        if (std::find(street2Vector.begin(), street2Vector.end(), commonIntersection) != street2Vector.end()) {
            commonIntersections.push_back(commonIntersection);
        }
    }

    return commonIntersections;
}

// Returns all street ids corresponding to street names that start with the 
// given prefix 
// The function should be case-insensitive to the street prefix. 
// The function should ignore spaces.
//  For example, both "bloor " and "BloOrst" are prefixes to 
// "Bloor Street East".
// If no street names match the given prefix, this routine returns an empty 
// (length 0) vector.
// You can choose what to return if the street prefix passed in is an empty 
// (length 0) string, but your program must not crash if street_prefix is a 
// length 0 string.
// Speed Requirement --> high 
std::string toLowerString(std::string street_prefix){
    
    for(int letterIndex = 0; letterIndex < street_prefix.size(); letterIndex++){
        street_prefix[letterIndex] = tolower(street_prefix[letterIndex]);
    }
    return street_prefix;
}
std::string shortVersion(std::string input, int size){
    input.resize(size);
    return input;
}
std::vector<StreetIdx> findStreetIdsFromPartialStreetName(std::string street_prefix) {

    if(street_prefix.size() == 0){
        std::vector<StreetIdx> emptyVector= {};
        return emptyVector;
    }
    
    street_prefix.erase( std::remove( street_prefix.begin(), street_prefix.end(), ' ' ), street_prefix.end() );
    street_prefix = toLowerString(street_prefix);

    return allStreetsKeys[street_prefix];
  
}

// Returns the length of a given street in meters
// Speed Requirement --> high 
double findStreetLength(StreetIdx street_id) {
    return street_lengths[street_id][0];
}

// Returns the nearest point of interest of the given type (e.g. "restaurant") 
// to the given position
// Speed Requirement --> none 
POIIdx findClosestPOI(LatLon my_position, std::string POItype) {

    std::string type;
    double smallestDistance = kEarthRadiusInMeters;
    double currentDistance;
    POIIdx closestPOI = 0;

    //iterate through all points of interest - store smallest distance between current position and POI in smallest distance & closestPOI
    for (POIIdx currentPOI = 0; currentPOI < getNumPointsOfInterest(); ++currentPOI) {
        type = getPOIType(currentPOI);
        if (type == POItype) {
            currentDistance = findDistanceBetweenTwoPoints(getPOIPosition(currentPOI), my_position);
            if (currentDistance <= smallestDistance) {
                smallestDistance = currentDistance;
                closestPOI = currentPOI;
            }
        }
        
    }

    return closestPOI;
}

// Returns the area of the given closed feature in square meters
// Assume a non self-intersecting polygon (i.e. no holes)
// Return 0 if this feature is not a closed polygon.
// Speed Requirement --> moderate
double findFeatureArea(FeatureIdx feature_id) {
    
        LatLon point_1_edge =  getFeaturePoint(feature_id,0);
        LatLon point_2_edge =  getFeaturePoint(feature_id, getNumFeaturePoints(feature_id) - 1);

        double  lat1_edge = point_1_edge.latitude() * kDegreeToRadian,
                lat2_edge = point_2_edge.latitude() * kDegreeToRadian,
                lon1_edge = point_1_edge.longitude() * kDegreeToRadian,
                lon2_edge = point_2_edge.longitude() * kDegreeToRadian;
        
        double lat_avg_edge = (lat1_edge + lat2_edge) * 0.5;

        double cosine_edge = cos(lat_avg_edge);

        double x1_edge = kEarthRadiusInMeters * lon1_edge * cosine_edge;
        double y1_edge = kEarthRadiusInMeters * lat1_edge;

        double x2_edge = kEarthRadiusInMeters * lon2_edge * cosine_edge;
        double y2_edge = kEarthRadiusInMeters * lat2_edge;
        //Testing edge cases closed polynomial and one point feature id
        if(x1_edge != x2_edge && y1_edge != y2_edge){
            return 0;
        } else if((getNumFeaturePoints(feature_id) - 1) == 0){
            return 0;
        }

        double totalArea = 0;
    for(int currentFeature = 0; currentFeature < getNumFeaturePoints(feature_id) - 1; currentFeature++){
       LatLon point_1 =  getFeaturePoint(feature_id,currentFeature);
       LatLon point_2 =  getFeaturePoint(feature_id, currentFeature + 1);

        double  lat1 = point_1.latitude() * kDegreeToRadian,
                lat2 = point_2.latitude() * kDegreeToRadian,
                lon1 = point_1.longitude() * kDegreeToRadian,
                lon2 = point_2.longitude() * kDegreeToRadian;
         

        double lat_avg = (lat1 + lat2) * 0.5;

        double cosine = cos(lat_avg);

        double x1 = kEarthRadiusInMeters * lon1 * cosine;
        double y1 = kEarthRadiusInMeters * lat1;

        double x2 = kEarthRadiusInMeters * lon2 * cosine;
        double y2 = kEarthRadiusInMeters * lat2;

       totalArea += (y1 - y2) * ((x1 + x2)/2);
       
       
    }
    if(totalArea < 0){
        totalArea = totalArea * -1;
    }
    return totalArea;
}

// Return the value associated with this key on the specified OSMNode.
// If this OSMNode does not exist in the current map, or the specified key is 
// not set on the specified OSMNode, return an empty string.
// Speed Requirement --> high
std::string getOSMNodeTagValue (OSMID OSMid, std::string key) {

    std::vector <std::pair <std::string , std::string>>  vectorPairs =  OSMNodesandTags[OSMid];

    for(int pairs = 0; pairs < vectorPairs.size(); pairs++){
        if(vectorPairs[pairs].first == key){
            return vectorPairs[pairs].second;
        }
    }
    return "";
}

// Return the value associated with this key on the specified OSMWay.
// If this OSMWay does not exist in the current map, or the specified key is 
// not set on the specified OSMNode, return an empty string.
// Speed Requirement --> high
std::string getOSMWayTagValue (OSMID OSMid, std::string key) {

    std::vector <std::pair <std::string , std::string>>  vectorPairs =  OSMWaysandTags[OSMid];

    for(int pairs = 0; pairs < vectorPairs.size(); pairs++){
        if(vectorPairs[pairs].first == key){
            return vectorPairs[pairs].second;
        }
    }
    return "";
}

double x_from_lon(float lon) {
   
   return (lon * kDegreeToRadian * kEarthRadiusInMeters * std::cos(avg_lat*kDegreeToRadian));

}

double y_from_lat(float lat) {

   return (lat * kDegreeToRadian * kEarthRadiusInMeters);

}

double lon_from_x(float x) {
    return (x/(kDegreeToRadian * kEarthRadiusInMeters * std::cos(avg_lat*kDegreeToRadian)));
}

double lat_from_y(float y) {
    return (y/(kDegreeToRadian * kEarthRadiusInMeters));
}
bool areaCompare(featureStruct f1, featureStruct f2){
    return(f1.area > f2.area);
}

void sortPOITypes(std::string type, int poiID) {
    // Types: Food, Education, Transportation, Financial,
    //        Healthcare, Entertainment, Public, other

    if (type == "bar" || type == "biergarten" || type == "cafe" ||
        type == "fast_food" || type == "ice_cream" || type == "pub" ||
        type == "restaurant" || type == "food_court") {
            poi_information[poiID].type = "Food";
        }
    else if (type == "college" || type == "driving_school" ||  type == "kindergarten" ||
             type == "language_school" || type == "library" ||  type == "toy_library" ||
             type == "training" || type == "music_school" || type == "school" ||
             type == "traffic_park" || type == "university") {
        poi_information[poiID].type = "Education";
    }
    else if (type == "bicycle_parking" || type == "bicycle_repair_station" || type == "bicycle_rental" ||
            type == "boat_rental" || type == "boat_sharing" || type == "bus_station" ||
            type == "car_rental" || type == "car_sharing" ||type == "charging_station" ||
            type == "ferry_terminal" || type == "fuel" || type == "motorcycle_parking" ||
            type == "parking" || type == "parking_entrance" || type == "parking_space" ||
            type == "taxi" || type == "car_wash" || type == "compressed_air" ||
            type == "vehicle_inspection" || type == "driver_training" || type == "grit_bin") {
        poi_information[poiID].type = "Transportation";
    }
    else if (type == "atm" || type == "bank" || type == "bureau_de_change") {
        poi_information[poiID].type = "Financial";
    }
    else if (type == "clinic" || type == "dentist" || type == "doctors" ||
             type == "hospital" || type == "nursing_home" || type == "pharmacy" ||
             type == "social_facility" || type == "veterinary" || type == "baby_hatch") {
        poi_information[poiID].type = "Healthcare";
    }
    else if (type == "arts_centre" || type == "brothel" || type == "casino" ||
             type == "cinema" || type == "community_centre" || type == "conference_centre" ||
             type == "events_venue" || type == "exhibition_centre" || type == "fountain" ||
             type == "gambling" || type == "love_hotel" || type == "music_venue" ||
             type == "nightclub" || type == "planetarium" || type == "public_bookcase" ||
             type == "social_centre" || type == "stripclub" || type == "studio" ||
             type == "swingerclub" || type == "theatre") {
        poi_information[poiID].type = "Entertainment";
    }
    else if (type == "courthouse" || type == "fire_station" || type == "police" || 
             type == "post_box" || type == "post_depot" || type == "post_office" || 
             type == "prison" || type == "ranger_station" || type == "townhall") {
        poi_information[poiID].type = "Public";
    }
    else {
        poi_information[poiID].type = "Other";
    }
}
