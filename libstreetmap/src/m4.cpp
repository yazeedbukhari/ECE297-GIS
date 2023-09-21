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
#include "m1.h"
#include "m3.h"
#include "samiristhegoat.h"

void multidestDijkstra(IntersectionIdx, float);
void loadM4(const float turn_penalty, const std::vector<DeliveryInf>& deliveries, const std::vector<IntersectionIdx>& depots);
void closeM4();
std::vector <StreetSegmentIdx> traceBack (int destID);
std::unordered_map <IntersectionIdx, std::vector <CourierPath>> pathsMatrix;
std::vector <IntersectionIdx> deliveryIntersections;

// std::unordered_map <IntersectionIdx, bool> completedCheck;
// std::unordered_map <IntersectionIdx, IntersectionIdx> dropOffpickUp;

// This routine takes in a vector of D deliveries (pickUp, dropOff
// intersection pairs), another vector of N intersections that
// are legal start and end points for the path (depots), and a turn 
// penalty in seconds (see m3.h for details on turn penalties).
//
// The first vector 'deliveries' gives the delivery information.  Each delivery
// in this vector has pickUp and dropOff intersection ids.
// A delivery can only be dropped-off after the associated item has been picked-up. 
// 
// The second vector 'depots' gives the intersection ids of courier company
// depots containing trucks; you start at any one of these depots and end at
// any one of the depots.
//
// This routine returns a vector of CourierSubPath objects that form a delivery route.
// The CourierSubPath is as defined above. The first street segment id in the
// first subpath is connected to a depot intersection, and the last street
// segment id of the last subpath also connects to a depot intersection.
// A package will not be dropped off if you haven't picked it up yet.
//
// The start_intersection of each subpath in the returned vector should be 
// at least one of the following (a pick-up and/or drop-off can only happen at 
// the start_intersection of a CourierSubPath object):
//      1- A start depot.
//      2- A pick-up location
//      3- A drop-off location. 
//
// You can assume that D is always at least one and N is always at least one
// (i.e. both input vectors are non-empty).
//
// It is legal for the same intersection to appear multiple times in the pickUp
// or dropOff list (e.g. you might have two deliveries with a pickUp
// intersection id of #50). The same intersection can also appear as both a
// pickUp location and a dropOff location.
//        
// If you have two pickUps to make at an intersection, traversing the
// intersection once is sufficient to pick up both packages. Additionally, 
// one traversal of an intersection is sufficient to drop off all the 
// (already picked up) packages that need to be dropped off at that intersection.
//
// Depots will never appear as pickUp or dropOff locations for deliveries.
//  
// If no valid route to make *all* the deliveries exists, this routine must
// return an empty (size == 0) vector.
std::vector<CourierSubPath> travelingCourier(const std::vector<DeliveryInf>& deliveries, const std::vector<IntersectionIdx>& depots, const float turn_penalty){
    
    std::vector<CourierSubPath> result = {};
    std::vector<CourierSubPath> finalResult;
    double finalResultTime = BIGNUMBER;

    loadM4(turn_penalty, deliveries, depots);

    //PASSES MODERATE TA
    //while loop for perturbations
    for(int multistart = 0; multistart < depots.size(); multistart++){

        bool isDone = false;

        //initializing vector of bool structs for drop off and pick up
        std::vector<DeliveryInfBool> deliveriesBool;
        for(int delivery = 0; delivery < deliveries.size(); delivery++){
            DeliveryInfBool currentDelivery;
            currentDelivery.pickUp = deliveries[delivery].pickUp;
            currentDelivery.dropOff = deliveries[delivery].dropOff;
            deliveriesBool.push_back(currentDelivery);
        }

        //start algorithm
        //go from depot to nearest pickup
        IntersectionIdx source; //store the current source intersection
        IntersectionIdx p;

        double bestTime = BIGNUMBER;

        // for (int currentDepot = 0; currentDepot < depots.size(); currentDepot++) {
        //     for (int currentDestination = 0; currentDestination < pathsMatrix[depots[currentDepot]].size(); currentDestination++) {
        //         CourierPath depotPath = pathsMatrix[depots[currentDepot]][currentDestination];
        //         CourierSubPath depotSubPath = pathsMatrix[depots[currentDepot]][currentDestination].courierSubPath;
        //         if (depotPath.subPathTime < bestTime) { //update the storage variables if new fastest time is found
        //             bestTime = depotPath.subPathTime;
        //             p = depotSubPath.start_intersection; //p holds the depot to start from
        //             continue;
        //         }
        //     }
        // }
        p = depots[multistart];

        CourierSubPath solutionSubPath; //stores the subpaths that will be put into result vector
        CourierPath solutionPath;

        while(!isDone){
            source = p; 
            bestTime = BIGNUMBER;
            
            for(int i = 0; i < pathsMatrix[source].size(); i++){
                CourierPath currentElement = pathsMatrix[source][i];
                CourierSubPath currentSubPath = pathsMatrix[source][i].courierSubPath;
                IntersectionIdx destination = currentSubPath.end_intersection;
                if(currentElement.destType == "depot"){
                    continue;
                } else if(currentElement.destType == "pickUp"){
                    for(int j = 0; j < deliveriesBool.size(); j++){
                        if(deliveriesBool[j].pickUp == destination && deliveriesBool[j].pickUpBool == false){
                            if (currentElement.subPathTime < bestTime){    
                                bestTime = currentElement.subPathTime;
                                p = currentSubPath.end_intersection;
                                solutionSubPath = currentSubPath;
                                solutionPath = currentElement;
                            }
                        }
                    }
                } else {
                    bool allPickedUp = true;
                    for(int k = 0; k < deliveriesBool.size(); k++){
                        if(deliveriesBool[k].dropOff == destination){
                            if(deliveriesBool[k].pickUpBool == false){
                                allPickedUp = false;
                            }
                        }
                    }
                    for(int j = 0; j < deliveriesBool.size(); j++){
                        if(deliveriesBool[j].pickUpBool == true && deliveriesBool[j].dropOff == destination && deliveriesBool[j].dropOffBool == false && allPickedUp){
                            if (currentElement.subPathTime < bestTime) {    
                                bestTime = currentElement.subPathTime;
                                p = currentSubPath.end_intersection;
                                solutionSubPath = currentSubPath;
                                solutionPath = currentElement;
                            }
                        }
                    }
                }
            }
            for(int j = 0; j < deliveriesBool.size(); j++){
                if(deliveriesBool[j].pickUp == p){
                    deliveriesBool[j].pickUpBool = true;
                }
                if(deliveriesBool[j].pickUpBool == true && deliveriesBool[j].dropOff == p){
                    deliveriesBool[j].dropOffBool = true;
                }
            }
            result.push_back(solutionSubPath); //add chosen sub path to the solution vector
            isDone = true;
            for(int j = 0; j < deliveriesBool.size(); j++){
                if(deliveriesBool[j].dropOffBool == false){
                    isDone = false;
                }
            }
        }
        //go to nearest depot
        bestTime = BIGNUMBER;
        for (int currentDestination = 0; currentDestination < pathsMatrix[p].size(); currentDestination++) {
            CourierPath currentElement = pathsMatrix[p][currentDestination];
            CourierSubPath currentSubPath = pathsMatrix[p][currentDestination].courierSubPath;
            if (!(currentElement.destType == "depot")){
                continue;
            } else {
                if (currentElement.subPathTime < bestTime) {    
                    bestTime = currentElement.subPathTime;
                    solutionSubPath = currentSubPath;
                }
            }
        }
        result.push_back(solutionSubPath); //add chosen sub path to the solution vector
        
        double resultTime = 0;
        for(int resultSubPath = 0; resultSubPath < result.size(); resultSubPath++){
            resultTime += computePathTravelTime(result[resultSubPath].subpath, turn_penalty);
        }
        if(resultTime < finalResultTime){
            finalResult = result;
        }

        bool oneBoost = false;
        int numSubPaths = result.size();
        int i = 1;
        while (!oneBoost) {
            if (numSubPaths <= 3) continue;

            //swapping 
            CourierSubPath temp = result[numSubPaths - i];
            result[numSubPaths - i] = result[numSubPaths - i - 1];
            result[numSubPaths - i - 1] = temp;
            
            resultTime = 0;
            for(int resultSubPath = 0; resultSubPath < result.size(); resultSubPath++){
                resultTime += computePathTravelTime(result[resultSubPath].subpath, turn_penalty);
            }

            if(resultTime < finalResultTime){
                finalResult = result;
                oneBoost = true;
            }


        }

        deliveriesBool.clear();
        result.clear();
    }
    
    closeM4();
   
    return finalResult;
}
void loadM4(const float turn_penalty, const std::vector<DeliveryInf>& deliveries, const std::vector<IntersectionIdx>& depots){

    for(int pickUp = 0; pickUp < deliveries.size(); pickUp++){
        deliveryIntersections.push_back(deliveries[pickUp].pickUp);
    }
    for(int dropOff = 0; dropOff < deliveries.size(); dropOff++){
        deliveryIntersections.push_back(deliveries[dropOff].dropOff);
    }
    for(int depot = 0; depot < depots.size(); depot++){
        deliveryIntersections.push_back(depots[depot]);
    }

    
    CourierPath currentElement;
    CourierSubPath currentSubPath;

    //drop offs as sources
    for(int dropOff = 0; dropOff < deliveries.size(); dropOff++){
         //Setting all nodes best times to a large number
        for(int nodeIntersection = 0; nodeIntersection < getNumIntersections(); nodeIntersection++){
            nodes[nodeIntersection].bestTime = BIGNUMBER;
        }
        multidestDijkstra(deliveries[dropOff].dropOff, turn_penalty);
        for(int destination = 0; destination < deliveryIntersections.size(); destination++){
            if(deliveryIntersections[destination] != deliveries[dropOff].dropOff){
                currentSubPath.start_intersection =  deliveries[dropOff].dropOff;
                currentSubPath.end_intersection = deliveryIntersections[destination];
                currentSubPath.subpath = traceBack(deliveryIntersections[destination]);
                currentElement.courierSubPath = currentSubPath;
                currentElement.subPathTime = nodes[deliveryIntersections[destination]].bestTime;
                if(destination < deliveries.size()){
                    currentElement.destType = "pickUp";
                } else if (destination < deliveries.size()*2){
                    currentElement.destType = "dropOff";
                } else {
                    currentElement.destType = "depot";
                }
                pathsMatrix[deliveries[dropOff].dropOff].push_back(currentElement);
            }
            
        }
    }
    //pick ups as sources
    for(int pickUp = 0; pickUp < deliveries.size(); pickUp++){
         //Setting all nodes best times to a large number
        for(int nodeIntersection = 0; nodeIntersection < getNumIntersections(); nodeIntersection++){
            nodes[nodeIntersection].bestTime = BIGNUMBER;
        }
        multidestDijkstra(deliveries[pickUp].pickUp, turn_penalty);
        for(int destination = 0; destination < (deliveryIntersections.size() - depots.size()); destination++){
            if(deliveryIntersections[destination] != deliveries[pickUp].pickUp){
                currentSubPath.start_intersection =  deliveries[pickUp].pickUp;
                currentSubPath.end_intersection = deliveryIntersections[destination];
                currentSubPath.subpath = traceBack(deliveryIntersections[destination]);//cannot be threaded
                currentElement.courierSubPath = currentSubPath;
                currentElement.subPathTime = nodes[deliveryIntersections[destination]].bestTime; //cannot be threaded
                if(destination < deliveries.size()){
                    currentElement.destType = "pickUp";
                } else {
                    currentElement.destType = "dropOff";
                }
                pathsMatrix[deliveries[pickUp].pickUp].push_back(currentElement);
            }
        }
    }
    //depots as sources
    for(int depot = 0; depot < depots.size(); depot++){
        //Setting all nodes best times to a large number
        for(int nodeIntersection = 0; nodeIntersection < getNumIntersections(); nodeIntersection++){
            nodes[nodeIntersection].bestTime = BIGNUMBER;
        }
        multidestDijkstra(depots[depot], turn_penalty);
        for(int destination = 0; destination < deliveries.size(); destination++){
            if(deliveryIntersections[destination] != depots[depot]){
                currentSubPath.start_intersection =  depots[depot];
                currentSubPath.end_intersection = deliveryIntersections[destination];
                currentSubPath.subpath = traceBack(deliveryIntersections[destination]);
                currentElement.courierSubPath = currentSubPath;
                currentElement.subPathTime = nodes[deliveryIntersections[destination]].bestTime;
                currentElement.destType = "pickUp";
                pathsMatrix[depots[depot]].push_back(currentElement);
            }
        }
    }
}
void closeM4(){
    pathsMatrix.clear();
    deliveryIntersections.clear();
}
///////----------------------------------------------------------------------------------last resort
void multidestDijkstra(IntersectionIdx srcID, float turn_penalty){
    //priority queue/ min heap
    std::priority_queue<WaveElem, std::vector<WaveElem>, timeWaveElemComparator> waveFrontMinHeap; 
    //Initializing source node
    waveFrontMinHeap.push(WaveElem(srcID, SOURCE_EDGE, 0,0));

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
                        
                        if(nodes[currNodeID].reachingEdge != SOURCE_EDGE && street_segment.streetID != currNodeReachingEdge.streetID){
                            waveFrontMinHeap.push(WaveElem(toNodeID, outEdge[edge], travelTimeNode + turn_penalty, travelTimeNode + turn_penalty));
                        } else{
                            waveFrontMinHeap.push(WaveElem(toNodeID, outEdge[edge], travelTimeNode, travelTimeNode));
                        }
                } else {
                    if(street_segment.from == currNodeID){
                        //calculating traveltime node and heuristic node
                        double travelTimeNode = nodes[currNodeID].bestTime + findStreetSegmentTravelTime(outEdge[edge]);
                        if(nodes[currNodeID].reachingEdge != SOURCE_EDGE && street_segment.streetID != currNodeReachingEdge.streetID){
                            waveFrontMinHeap.push(WaveElem(street_segment.to, outEdge[edge], travelTimeNode + turn_penalty, travelTimeNode + turn_penalty));
                        }else {
                            waveFrontMinHeap.push(WaveElem(street_segment.to, outEdge[edge], travelTimeNode, travelTimeNode));
                        }
                    }
                }
            }
            
        }

    }
}
std::vector <StreetSegmentIdx> traceBack (int destID) {

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