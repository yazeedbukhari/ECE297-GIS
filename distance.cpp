// Returns the distance between two (lattitude,longitude) coordinates in meters
// Speed Requirement --> moderate
double findDistanceBetweenTwoPoints(LatLon point_1, LatLon point_2) {
    double  lat1 = point1.latitude() * kDegreeToRadian,
            lat2 = point2.latitude() * kDegreeToRadian.
            lon1 = point1.longitude() * kDegreeToRadian,
            lon2 = point2.longitude() * kDegreeToRadian;
    
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
    StreetSegmentInfo street_segment = getStreetSegmentInfo(street_segment_id); // to access start and end point along with curvepoints
    
    int curvepoints = street_segment.numCurvePoints;
    if (curvepoints == 0) {
        return findDistanceBetweenTwoPoints( getIntersectionPosition(street_segment.from), getIntersectionPosition(street_segment.to));
    }
    double ret = 0;
    
    ret += findDistanceBetweenTwoPoints( getIntersectionPosition(street_segment.from), getStreetSegmentCurvePoint(street_segment_id, 0));
    for (int i = 1; i < curvepoints-1; i++) {
        ret += findDistanceBetweenTwoPoints( getStreetSegmentCurvePoint(street_segment_id, i), getStreetSegmentCurvePoint(street_segment_id, i+1));
    }
    ret += findDistanceBetweenTwoPoints( getIntersectionPosition(street_segment.to), getStreetSegmentCurvePoint(street_segment_id, curvepoints-1));

    return ret;
}

// Returns the travel time to drive from one end of a street segment 
// to the other, in seconds, when driving at the speed limit
// Note: (time = distance/speed_limit)
// Speed Requirement --> high 
double findStreetSegmentTravelTime(StreetSegmentIdx street_segment_id){

}