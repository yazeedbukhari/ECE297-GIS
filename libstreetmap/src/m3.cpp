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
#include <chrono>
#include "m1.h"
#include "m2.h"
#include "m3.h"
#include "StreetsDatabaseAPI.h"
#include "OSMDatabaseAPI.h"
#include "samiristhegoat.h"
#include "ezgl/application.hpp"
#include "ezgl/graphics.hpp"
#include "ezgl/point.hpp"
#include <cmath>
#include "libcurlstuff.h"

#define _USE_MATH_DEFINES
bool bfsPath(int srcID, int destID, const double turn_penalty);
std::vector <StreetSegmentIdx> bfsTraceBack (int destID);
int crossProduct(ezgl::point2d preLinkPointPos, ezgl::point2d linkIntersectionPos, ezgl::point2d nextPointPos);
std::string determineTurnDirection(StreetSegmentInfo currentStreetSegment, StreetSegmentInfo prevStreetSegment, StreetSegmentIdx currentStreetSegmentID, StreetSegmentIdx prevStreetSegmentID);
void displayPathMarkers(IntersectionIdx destID, IntersectionIdx srcID, ezgl::renderer *g);
std::stringstream directionsText;
bool newPath;

IntersectionIdx globalsrcID = 0;
IntersectionIdx globaldestID = 0;
//Vector of Node structs for each intersection

// Returns the time required to travel along the path specified, in seconds.
// The path is given as a vector of street segment ids, and this function can
// assume the vector either forms a legal path or has size == 0.  The travel
// time is the sum of the length/speed-limit of each street segment, plus the
// given turn_penalty (in seconds) per turn implied by the path.  If there is
// no turn, then there is no penalty. Note that whenever the street id changes
// (e.g. going from Bloor Street West to Bloor Street East) we have a turn.
double computePathTravelTime(const std::vector<StreetSegmentIdx>& path, const double turn_penalty){
    double totalTravelTime = 0;
    
    for (int currentStrSeg = 0; currentStrSeg < path.size(); currentStrSeg++) {
        StreetSegmentInfo currentStrSegInfo = getStreetSegmentInfo(path[currentStrSeg]);
        if(currentStrSeg != 0){

            StreetSegmentInfo prevStrSegInfo = getStreetSegmentInfo(path[currentStrSeg-1]);

             //if current streetID is different than previous streetID: add 15s for turn penalty
            if (currentStrSegInfo.streetID != prevStrSegInfo.streetID) {
                totalTravelTime += turn_penalty;
            }
        }

        //add street segment travel time
        totalTravelTime += findStreetSegmentTravelTime(path[currentStrSeg]);
    }

    return totalTravelTime;
}


// Returns a path (route) between the start intersection (1st object in intersect_ids pair)
// and the destination intersection(2nd object in intersect_ids pair),
// if one exists. This routine should return the shortest path
// between the given intersections, where the time penalty to turn right or
// left is given by turn_penalty (in seconds).  If no path exists, this routine
// returns an empty (size == 0) vector.  If more than one path exists, the path
// with the shortest travel time is returned. The path is returned as a vector
// of street segment ids; traversing these street segments, in the returned
// order, would take one from the start to the destination intersection.
std::vector<StreetSegmentIdx> findPathBetweenIntersections(const std::pair<IntersectionIdx, IntersectionIdx> intersect_ids, const double turn_penalty){

    newPath = true;
    //Setting all nodes best times to a large number
    for(int nodeIntersection = 0; nodeIntersection < getNumIntersections(); nodeIntersection++){
        nodes[nodeIntersection].bestTime = BIGNUMBER;
    }
    IntersectionIdx srcID = intersect_ids.first;
    IntersectionIdx destID = intersect_ids.second;
    globalsrcID = srcID;
    globaldestID = destID;
    //Initializing Path as an Empty Vector
    std::vector <StreetSegmentIdx> Path = {};
    if(bfsPath(srcID, destID, turn_penalty)){
        Path = bfsTraceBack(destID);
    }
    
    return Path;
}
bool bfsPath(int srcID, int destID, const double turn_penalty){
    //priority queue/ min heap
    std::priority_queue<WaveElem, std::vector<WaveElem>, timeWaveElemComparator> waveFrontMinHeap; 
    //Initializing source node
    waveFrontMinHeap.push(WaveElem(srcID, SOURCE_EDGE, 0,0));
    bool pathFound = false;
    //While heap is empty
    while(waveFrontMinHeap.size() != 0){
        //take top of heap and pop it
        WaveElem wave = waveFrontMinHeap.top();
        waveFrontMinHeap.pop();
        
        int currNodeID = wave.nodeID;
        //if the node im on has a faster time than its best time 
        if(wave.travelTime < nodes[currNodeID].bestTime){
            nodes[currNodeID].reachingEdge = wave.edgeID;
            nodes[currNodeID].bestTime = wave.travelTime;
               
            if(currNodeID == destID){
                pathFound = true;
                return pathFound;
            }
            //vector of all segments coming out of the current node
            std::vector<StreetSegmentIdx> outEdge = findStreetSegmentsOfIntersection(currNodeID);
            //for loop looping through all of segments
            for(int edge = 0; edge < outEdge.size(); edge++){
                StreetSegmentInfo street_segment = street_segment_info[outEdge[edge]];
                StreetSegmentInfo currNodeReachingEdge;
                if(nodes[currNodeID].reachingEdge != SOURCE_EDGE){
                    currNodeReachingEdge = street_segment_info[nodes[currNodeID].reachingEdge];
                }
                int toNodeID = 0;
                if(street_segment.oneWay != true){
                        if(currNodeID== street_segment.to){
                            toNodeID = street_segment.from;
                        } else {
                            toNodeID = street_segment.to;
                        }
                        //calculating traveltime node and heuristic node
                        double travelTimeNode = nodes[currNodeID].bestTime + findStreetSegmentTravelTime(outEdge[edge]);
                        double asTheCrowFlies = (findDistanceBetweenTwoPoints(getIntersectionPosition(destID), getIntersectionPosition(toNodeID)) / max_speed_limit);
                        if(nodes[currNodeID].reachingEdge != SOURCE_EDGE && street_segment.streetID != currNodeReachingEdge.streetID){
                            waveFrontMinHeap.push(WaveElem(toNodeID, outEdge[edge], travelTimeNode + turn_penalty, travelTimeNode + turn_penalty + asTheCrowFlies));
                        } else{
                            waveFrontMinHeap.push(WaveElem(toNodeID, outEdge[edge], travelTimeNode, travelTimeNode + asTheCrowFlies));
                        }
                } else {
                    if(street_segment.from == currNodeID){
                        //calculating traveltime node and heuristic node
                        double travelTimeNode = nodes[currNodeID].bestTime + findStreetSegmentTravelTime(outEdge[edge]);
                        double asTheCrowFlies = (findDistanceBetweenTwoPoints(getIntersectionPosition(destID), getIntersectionPosition(street_segment.to)) / max_speed_limit);
                        if(nodes[currNodeID].reachingEdge != SOURCE_EDGE && street_segment.streetID != currNodeReachingEdge.streetID){
                            waveFrontMinHeap.push(WaveElem(street_segment.to, outEdge[edge], travelTimeNode + turn_penalty, travelTimeNode + turn_penalty + asTheCrowFlies));
                        }else {
                            waveFrontMinHeap.push(WaveElem(street_segment.to, outEdge[edge], travelTimeNode, travelTimeNode + asTheCrowFlies));
                        }
                    }
                }
            }
            
        }

    }
    return pathFound;
}

std::vector <StreetSegmentIdx> bfsTraceBack (int destID) {

    std::list<StreetSegmentIdx> path;

    int currNodeID = destID;

    int prevEdge = nodes[currNodeID].reachingEdge;

    //this while loops goes into nodes vector at destID and back tracks all previous edges until the previous edge of the source then returns a vector
    while(prevEdge != SOURCE_EDGE){
        
        path.push_front(prevEdge);
        StreetSegmentInfo street_segment = getStreetSegmentInfo(prevEdge);
        if(currNodeID == street_segment.to){
            currNodeID = street_segment.from;
        }else{
            currNodeID = street_segment.to;
        }
        prevEdge = nodes[currNodeID].reachingEdge;
    }
    std::vector <StreetSegmentIdx> outputVector(path.begin(), path.end());

    return outputVector;
}


void displayPath(std::vector<StreetSegmentIdx> streetSegmentPathVector, ezgl::renderer *g) {
    directionsText.str("");
    if (streetSegmentPathVector.empty()) {
        return;  
    }   

    if (newPath) {
        for(int strSegID = 0; strSegID < getNumStreetSegments(); strSegID++) {
            pathGlobalBool[strSegID] = false;
        }
    }
    newPath = false;
    
    //looping through the path vector: - toggling appropriate street segments to be displayed
    //                                 - recording directions to traverse the path 
    int distanceTraveled = 0;
    for (int currStrSeg = 0; currStrSeg < streetSegmentPathVector.size(); currStrSeg++) {
        int currStrSegID = streetSegmentPathVector[currStrSeg];
        pathGlobalBool[currStrSegID] = true;
        StreetSegmentInfo currentStreetSegment = getStreetSegmentInfo(currStrSegID);
        


        //if not the first segment, access previous street segment
        if (currStrSeg != 0) {
            StreetSegmentInfo prevStreetSegment = getStreetSegmentInfo(streetSegmentPathVector[currStrSeg-1]);
            
            //determining the turn direction onto new street 
            if (prevStreetSegment.streetID != currentStreetSegment.streetID) {
                directionsText << " for " << distanceTraveled << " metres." <<endl;
                std::string turnDirection = determineTurnDirection(currentStreetSegment, prevStreetSegment, currStrSegID, streetSegmentPathVector[currStrSeg-1]);
                directionsText << turnDirection << getStreetName(currentStreetSegment.streetID) << "." << std::endl;
                directionsText << "Continue down " << getStreetName(currentStreetSegment.streetID);
                distanceTraveled = findStreetSegmentLength(currStrSegID);
            } else {
                distanceTraveled += findStreetSegmentLength(currStrSegID);
            }

            if (currStrSeg == (streetSegmentPathVector.size()-1)) {
                directionsText << " for " << distanceTraveled << " metres to reach your destination." <<endl;
            }

        } else {
            distanceTraveled += findStreetSegmentLength(currStrSegID);
            //determining the initial intersection of path
            if (streetSegmentPathVector.size() > 1) {
                IntersectionIdx startIntersection;
                std::string otherStreetNameOfIntersection;
                bool otherStreetOfIntersectionFound = false;

                if (currentStreetSegment.from == getStreetSegmentInfo(streetSegmentPathVector[currStrSeg+1]).from 
                    || currentStreetSegment.from == getStreetSegmentInfo(streetSegmentPathVector[currStrSeg+1]).to) {
                    startIntersection = currentStreetSegment.to;
                } else {
    
                    startIntersection = currentStreetSegment.from;
                }

                for (int intersectionSegment = 0; intersectionSegment < getNumIntersectionStreetSegment(startIntersection); intersectionSegment++) {
                    if (getStreetSegmentInfo(getIntersectionStreetSegment (startIntersection, intersectionSegment)).streetID != currentStreetSegment.streetID) {
                        otherStreetNameOfIntersection = getStreetName(getStreetSegmentInfo(getIntersectionStreetSegment (startIntersection, intersectionSegment)).streetID);
                        otherStreetOfIntersectionFound = true;
                    }
                }

                if (otherStreetOfIntersectionFound) {
                    directionsText << "Begin the route on " << getStreetName(currentStreetSegment.streetID) << " and " << otherStreetNameOfIntersection << "." << std::endl;
                    directionsText << "Continue down " << getStreetName(currentStreetSegment.streetID);
                } else {
                    directionsText << "Begin the route on " << getStreetName(currentStreetSegment.streetID) << "." << std::endl;
                    directionsText << "Continue down " << getStreetName(currentStreetSegment.streetID);
                }
            
            }
        }

    }

    displayPathMarkers(globaldestID, globalsrcID, g);




    return;
}

void displayPathMarkers(IntersectionIdx destID, IntersectionIdx srcID, ezgl::renderer *g) {
   LatLon destinationLocationLL = getIntersectionPosition(destID);
   ezgl::point2d destinationLocation = ezgl::point2d(x_from_lon(destinationLocationLL.longitude()), y_from_lat(destinationLocationLL.latitude()));
   ezgl::surface *finalDestinationFlag = ezgl::renderer::load_png("libstreetmap/resources/finalDestination.png");
   g->draw_surface(finalDestinationFlag, destinationLocation, 0.5);

    LatLon sourceLocationLL = getIntersectionPosition(srcID);
   ezgl::point2d sourceLocation = ezgl::point2d(x_from_lon(sourceLocationLL.longitude()), y_from_lat(sourceLocationLL.latitude()));
   ezgl::surface *sourcePointer = ezgl::renderer::load_png("libstreetmap/resources/source.png");
   g->draw_surface(sourcePointer, sourceLocation, 0.05);
}

std::string determineTurnDirection(StreetSegmentInfo currentStreetSegment, StreetSegmentInfo prevStreetSegment, StreetSegmentIdx currentStreetSegmentID, StreetSegmentIdx prevStreetSegmentID) {
    ezgl::point2d preLinkPointPos; //one curve point prior to the link intersection
    ezgl::point2d linkIntersectionPos; //the intersection shared by the two street segments
    ezgl::point2d nextPointPos; //one curve point after the link intersection
    std::string turnDirection;

    IntersectionIdx linkIntersection;

    if (currentStreetSegment.from == prevStreetSegment.from || currentStreetSegment.from == prevStreetSegment.to) {
        linkIntersectionPos = streetSegmentIdx_point2dxyCurvepoints[currentStreetSegmentID][0]; //("from" intersection)
        nextPointPos = streetSegmentIdx_point2dxyCurvepoints[currentStreetSegmentID][1]; //first curvepoint
        linkIntersection = currentStreetSegment.from;
    } else {
        linkIntersectionPos = streetSegmentIdx_point2dxyCurvepoints[currentStreetSegmentID][streetSegmentIdx_point2dxyCurvepoints[currentStreetSegmentID].size()-1]; //("to" intersection)
        nextPointPos = streetSegmentIdx_point2dxyCurvepoints[currentStreetSegmentID][streetSegmentIdx_point2dxyCurvepoints[currentStreetSegmentID].size()-2]; //second last curvepoint
        linkIntersection = currentStreetSegment.to;
    }

    if (linkIntersection == prevStreetSegment.from) {
        preLinkPointPos = streetSegmentIdx_point2dxyCurvepoints[prevStreetSegmentID][1]; //first curvepoint
    } else {
        preLinkPointPos = streetSegmentIdx_point2dxyCurvepoints[prevStreetSegmentID][streetSegmentIdx_point2dxyCurvepoints[prevStreetSegmentID].size()-2]; //second last curvepoint
    }

    int turnDirectionNum = crossProduct(preLinkPointPos, linkIntersectionPos, nextPointPos);

    if (turnDirectionNum > 0) {
        turnDirection = "Turn right onto ";
    } else if (turnDirectionNum < 0) {
        turnDirection = "Turn left onto ";
    } else {
        turnDirection = "Continue straight onto ";
    }
    
    return turnDirection;
}

int crossProduct(ezgl::point2d preLinkPointPos, ezgl::point2d linkIntersectionPos, ezgl::point2d nextPointPos) {
    int crossProduct;

    ezgl::point2d vector1 = ezgl::point2d(linkIntersectionPos.x-preLinkPointPos.x, linkIntersectionPos.y-preLinkPointPos.y); //vector from preLinkPointPos to linkIntersectionPos
    ezgl::point2d vector2 = ezgl::point2d(nextPointPos.x-preLinkPointPos.x, nextPointPos.y-preLinkPointPos.y); //vector from preLinkPointPos to nextPointPos

    crossProduct = (vector2.x*vector1.y) - (vector1.x*vector2.y);

    return crossProduct;
}
