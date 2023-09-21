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
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARcISING FROM,
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
#include <cmath>
#include "libcurlstuff.h"

#define _USE_MATH_DEFINES

//Data structures
std::vector<std::string> cities = {"Cities", "Toronto", "Beijing", "Cairo", "Cape Town", "Golden Horseshoe", "Hamilton", 
"Hong Kong", "Iceland", "Interlaken", "Kyiv", "London", "New Delhi", "New York", "Rio de Janeiro", "Saint Helena", "Singapore", "Sydney", "Tehran", "Tokyo"};
std::vector<std::string> POIs = {"POIs", "Food", "Education", "Transportation", "Financial", "Healthcare", "Entertainment", "Public", "All"};

ezgl::rectangle pathScreen;
double lineWidth;

//Function Calls
void draw_main_canvas (ezgl::renderer*);
void act_on_mouse_click(ezgl::application *app, GdkEventButton* event, double x, double y);
void initial_setup(ezgl::application* application, bool /*new_window*/);
void toggle_find (GtkWidget* /*widget*/, ezgl::application* application);
void toggle_find_path(GtkWidget* /*widget*/, ezgl::application* application);
void toggle_cities(GtkComboBoxText* self, ezgl::application* app);
void toggle_poi_filter(GtkComboBoxText* self, ezgl::application* application);
void changeMap();
void displayIntersectionRectangles(ezgl::renderer *g);
void displayFeatures(ezgl::renderer *g);
void toggle_clear(GtkWidget* /*widget*/, ezgl::application* application);
void drawFeature(ezgl::renderer *g, int featureID);
void displayStreets(ezgl::renderer *g);
void displayPOI(ezgl::renderer *g);
double findAngle(double x1, double x2, double y1, double y2);
void displayDistanceScale(ezgl::renderer *g);
void toggle_night (GtkWidget* /*widget*/, ezgl::application* application);
void toggle_directions (GtkWidget* /*widget*/, ezgl::application* application);
void toggle_help (GtkWidget* /*widget*/, ezgl::application* application);
void deactivatePOIS();
void displayCityNames(ezgl::renderer *g);
void autoComplete(ezgl::application* application);
void setDrawDetails(ezgl::renderer *g, std::string tag, double zoomFactorGlobal, int& red, int& green, int& blue);

void load_closure();
void compile_closure_info(ptree &ptRoot);
void display_closure(ezgl::renderer* g);

ezgl::renderer *globalRenderer;

using boost::property_tree::ptree;
using boost::property_tree::read_json;
std::vector<closure_data> closure_information;
//to display each type of poi
void displayIcon(ezgl::renderer *g, ezgl::point2d xy_loc, int sizeMult);
/*
void displayFood(ezgl::renderer *g, ezgl::point2d xy_loc);
void displayEducation(ezgl::renderer *g, ezgl::point2d xy_loc);
void displayTransportation(ezgl::renderer *g, ezgl::point2d xy_loc);
void displayFinancial(ezgl::renderer *g, ezgl::point2d xy_loc);
void displayHealthcare(ezgl::renderer *g, ezgl::point2d xy_loc);
void displayEntertainment(ezgl::renderer *g, ezgl::point2d xy_loc);
void displayPublic(ezgl::renderer *g, ezgl::point2d xy_loc);
void displayOther(ezgl::renderer *g, ezgl::point2d xy_loc);
*/

//Global Variable Initilization
ezgl::application* applicationPtr;
citiesBool allCitiesBool;
poiTypeFilterBool poiFilterBool;
double zoomFactor = 1;
bool nightMode = false;
float icon_size = 1;
std::vector <StreetSegmentIdx> pathGlobal = {};
int firstIntersectionID = -1; //used for the clicking part


void drawMap() {
   // Set up the ezgl graphics window and hand control to it, as shown in the 
   // ezgl example program. 
   // This function will be called by both the unit tests (ece297exercise) 
   // and your main() function in main/src/main.cpp.
   // The unit tests always call loadMap() before calling this function
   // and call closeMap() after this function returns.
   
   ezgl::application::settings settings;

   settings.main_ui_resource = "libstreetmap/resources/main.ui";
   settings.window_identifier = "MainWindow"; 
   settings.canvas_identifier = "MainCanvas";

   ezgl::application application(settings); 
   applicationPtr = &application; 

   
   ezgl::rectangle initial_world({x_from_lon(min_lon), y_from_lat(min_lat)}, {x_from_lon(max_lon), y_from_lat(max_lat)});
   
   
   application.add_canvas("MainCanvas", draw_main_canvas, initial_world);
   
   application.run(initial_setup, act_on_mouse_click, nullptr, nullptr);
   

}
void draw_main_canvas (ezgl::renderer *g) {

   auto startTime = std::chrono::high_resolution_clock::now();
   ezgl::rectangle initial_world({x_from_lon(min_lon), y_from_lat(min_lat)}, {x_from_lon(max_lon), y_from_lat(max_lat)});
   ezgl::rectangle visible_world(g->get_visible_world());
   
   //making the canvas grey or black (depending on mode)
   if (nightMode) {
      g->set_color(100, 100, 100);
   } else {
      g->set_color(230, 230, 230);
   }
   g->fill_rectangle(visible_world);
  
  //determining zoomFactor for later use in zoom based dynamic rendering 
   zoomFactor = (visible_world.top_right().x - visible_world.bottom_left().x) / (initial_world.top_right().x - initial_world.bottom_left().x);

   //function calls for several aspects of the map
   displayFeatures(g); 
   displayPath(pathGlobal, g);
   displayStreets(g);
   displayPath(pathGlobal, g);
   displayIntersectionRectangles(g);
   displayPOI(g); 
   displayDistanceScale(g);
   displayCityNames(g);
   changeMap();
   display_closure(g);


   //setting default printing methods
   g->set_color(0,0,0);
   g->set_line_width (2);
   g->set_text_rotation(0);
   
  
   auto currTime = std::chrono::high_resolution_clock::now();
   auto wallClock = std::chrono::duration_cast<std::chrono::duration<double>>(currTime - startTime);
   
   std::cout << "Draw Map: " << wallClock.count() << std::endl;
}

void act_on_mouse_click(ezgl::application *app, GdkEventButton*, double x, double y) {
   std::cout << "Mouse clicked at (" << x << ", " << y << ")\n";
   LatLon pos = LatLon(lat_from_y(y), lon_from_x(x));
   int id = findClosestIntersection(pos);
   std::cout << "IntersectionID: " << id << std::endl; 
   std::cout << "Closest Intersection: " << intersections_xyposname[id].name << "\n";

   intersections_xyposname[id].highlight = true;

   std::stringstream ss;
   ss << "Intersection Selected: " << intersections_xyposname[id].name;
   app->update_message(ss.str());
   

   if (firstIntersectionID == -1) {
      firstIntersectionID = id;
   }
   else {
      std::pair <int,int> pairOfIntersections;
      pairOfIntersections.first = firstIntersectionID;
      pairOfIntersections.second = id;
      
      pathGlobal = findPathBetweenIntersections(pairOfIntersections, 15);
      if (pathGlobal.size() == 0) {
         stringstream output;

         output << "No path exists between the given intersections" << endl;

         app->create_popup_message("Error", output.str().c_str());
      }      
      LatLon destinationLocationLL = getIntersectionPosition(firstIntersectionID);
      ezgl::point2d destinationLocation = ezgl::point2d(x_from_lon(destinationLocationLL.longitude()), y_from_lat(destinationLocationLL.latitude()));
      LatLon sourceLocationLL = getIntersectionPosition(id);
      ezgl::point2d sourceLocation = ezgl::point2d(x_from_lon(sourceLocationLL.longitude()), y_from_lat(sourceLocationLL.latitude()));
      if(destinationLocation.x > sourceLocation.x){
         destinationLocation.x = destinationLocation.x*1.0001;
         sourceLocation.x = sourceLocation.x*0.9999;
      }else{
         sourceLocation.x =sourceLocation.x*1.0001;
         destinationLocation.x = destinationLocation.x*0.9999;
      }
      if(destinationLocation.y > sourceLocation.y){
         destinationLocation.y = destinationLocation.y*1.0001;
         sourceLocation.y = sourceLocation.y*0.9999;
      }else{
         sourceLocation.y =sourceLocation.y*1.0001;
         destinationLocation.y = destinationLocation.y*0.9999;
      }
      pathScreen = ezgl::rectangle(destinationLocation, sourceLocation);
      ezgl::renderer* helperG = app->get_renderer();
      
      helperG->set_visible_world(pathScreen);

      firstIntersectionID = -1;
   }

   app->refresh_drawing();
}

void initial_setup(ezgl::application *application, bool /*new_window*/) {

   //ezgl implementation of interface elements
   application->create_button ("Find Intersection", 8, toggle_find);
   application->create_button("Find Path", 11, toggle_find_path);
   application->create_combo_box_text("City Drop Down", 12, toggle_cities, cities);
   application->create_button ("Night Mode", 13, toggle_night);
   application->create_combo_box_text("POI Filter", 14, toggle_poi_filter, POIs);
   application->create_button ("Get Directions", 15, toggle_directions);
   application->create_button ("Guide", 16, toggle_help);
   application->create_button("Clear Selections", 17, toggle_clear);
   autoComplete(application);
   load_closure(); //could change later
}

void toggle_clear(GtkWidget* /*widget*/, ezgl::application* application) {
   pathGlobal.clear();
   
   for (int streetSegID = 0; streetSegID < getNumStreetSegments(); streetSegID++) {
      pathGlobalBool[streetSegID] = false;
   }
   
   for (int intersectionID = 0; intersectionID < getNumIntersections(); intersectionID++) {
      if (intersections_xyposname[intersectionID].highlight) {
         intersections_xyposname[intersectionID].highlight = false;
      } else {
         continue;
      }
   }

   nightMode = false;

   poiFilterBool.Food = false;
   poiFilterBool.Education = false;
   poiFilterBool.Transportation = false;
   poiFilterBool.Financial = false;
   poiFilterBool.Healthcare = false;
   poiFilterBool.Entertainment = false;
   poiFilterBool.Public = false;
   poiFilterBool.All = false;

   application->refresh_drawing();

}

void toggle_find_path(GtkWidget* /*widget*/, ezgl::application* application){

   for (int streetSegment = 0; streetSegment < getNumStreetSegments(); streetSegment++) {
      pathGlobalBool[streetSegment] = false;
   }

   pathGlobal.clear();

   //getting input from the two entry boxes created in Glade
   GObject* gtk_object1 = application->get_object("SearchEntry1");
   GObject* gtk_object2 = application->get_object("SearchEntry2");
   GObject* gtk_object3 = application->get_object("SearchEntry3");
   GObject* gtk_object4 = application->get_object("SearchEntry4");

   GtkEntry* searchEntry1 = GTK_ENTRY(gtk_object1);
   GtkEntry* searchEntry2 = GTK_ENTRY(gtk_object2);
   GtkEntry* searchEntry3 = GTK_ENTRY(gtk_object3);
   GtkEntry* searchEntry4 = GTK_ENTRY(gtk_object4);

   const gchar* street1Name = gtk_entry_get_text(searchEntry1);
   const gchar* street2Name = gtk_entry_get_text(searchEntry2);
   const gchar* street3Name = gtk_entry_get_text(searchEntry3);
   const gchar* street4Name = gtk_entry_get_text(searchEntry4);


   std::vector < StreetIdx > partialMatchesVector1;
   
   std::vector < StreetIdx > partialMatchesVector2;

   std::vector < StreetIdx > partialMatchesVector3;
   
   std::vector < StreetIdx > partialMatchesVector4;

   std::vector < IntersectionIdx > intersectionsOfStreets;

   std::vector < IntersectionIdx> sourceIntersectionIDs;

   std::vector < IntersectionIdx> destinationIntersectionIDs;


   //passing input prefixes to function
   partialMatchesVector1 = findStreetIdsFromPartialStreetName(street1Name);
   partialMatchesVector2 = findStreetIdsFromPartialStreetName(street2Name);
   partialMatchesVector3 = findStreetIdsFromPartialStreetName(street3Name);
   partialMatchesVector4 = findStreetIdsFromPartialStreetName(street4Name);
   bool nonExistant = false;
   if (partialMatchesVector1.size() == 0 || partialMatchesVector2.size() == 0 || partialMatchesVector3.size() == 0 || partialMatchesVector4.size() == 0) {
      nonExistant = true;
      stringstream output;
      output << "The following streets do not exist: " << endl;
      if (partialMatchesVector1.size() == 0) {
         output << street1Name << endl;
      }
      if (partialMatchesVector2.size() == 0) {
         output << street2Name << endl;
      }
      if (partialMatchesVector3.size() == 0) {
         output << street3Name << endl;
      }
      if (partialMatchesVector4.size() == 0) {
         output << street4Name << endl;
      }
      application->create_popup_message("Error", output.str().c_str());
   }

   //if both unique results found, highlight intersection
   for(int street1 = 0; street1 < partialMatchesVector1.size(); street1++) {
      for(int street2 = 0; street2 < partialMatchesVector2.size(); street2++){
          intersectionsOfStreets = findIntersectionsOfTwoStreets(partialMatchesVector1[street1], partialMatchesVector2[street2]);

         for (int intersection = 0; intersection < intersectionsOfStreets.size(); intersection++){
           sourceIntersectionIDs.push_back(intersectionsOfStreets[intersection]);
         }
      }
   }
   for(int street3 = 0; street3 < partialMatchesVector3.size(); street3++) {
      for(int street4 = 0; street4 < partialMatchesVector4.size(); street4++){
          intersectionsOfStreets = findIntersectionsOfTwoStreets(partialMatchesVector3[street3], partialMatchesVector4[street4]);

         for (int intersection = 0; intersection < intersectionsOfStreets.size(); intersection++){
            destinationIntersectionIDs.push_back(intersectionsOfStreets[intersection]);
         }
      }
   }
   if ((sourceIntersectionIDs.size() == 0 || destinationIntersectionIDs.size() == 0) && !nonExistant) {
      stringstream output;
      if(sourceIntersectionIDs.size() == 0){
         output << "There are no intersections between: " << endl;
         output << street1Name << " & ";
         output << street2Name << "." << endl;
         
      }
      if(destinationIntersectionIDs.size() == 0){
         output << "There are no intersections between: " << endl;
         output << street3Name << " & ";
         output << street4Name << "." << endl;
      }
         application->create_popup_message("Error", output.str().c_str());
   }  
   if (sourceIntersectionIDs.size() != 0 && destinationIntersectionIDs.size()!=0) {
      int srcID = sourceIntersectionIDs[0];
      
      int destID = destinationIntersectionIDs[0];

      std::pair <int,int> pairOfIntersections;
      pairOfIntersections.first = srcID;
      pairOfIntersections.second = destID;

      pathGlobal = findPathBetweenIntersections(pairOfIntersections, 15);
      if (pathGlobal.size() == 0) {
         stringstream output;
         output << "No path exists between the given intersections: " << endl;
            output << street1Name << " & ";
            output << street2Name << ", ";
            output << street3Name << " & ";
            output << street4Name << "." << endl;
         application->create_popup_message("Error", output.str().c_str());
      }      
      LatLon destinationLocationLL = getIntersectionPosition(destID);
      ezgl::point2d destinationLocation = ezgl::point2d(x_from_lon(destinationLocationLL.longitude()), y_from_lat(destinationLocationLL.latitude()));
      LatLon sourceLocationLL = getIntersectionPosition(srcID);
      ezgl::point2d sourceLocation = ezgl::point2d(x_from_lon(sourceLocationLL.longitude()), y_from_lat(sourceLocationLL.latitude()));
      if(destinationLocation.x > sourceLocation.x){
         destinationLocation.x = destinationLocation.x*1.0001;
         sourceLocation.x = sourceLocation.x*0.9999;
      }else{
         sourceLocation.x =sourceLocation.x*1.0001;
         destinationLocation.x = destinationLocation.x*0.9999;
      }
      if(destinationLocation.y > sourceLocation.y){
         destinationLocation.y = destinationLocation.y*1.0001;
         sourceLocation.y = sourceLocation.y*0.9999;
      }else{
         sourceLocation.y =sourceLocation.y*1.0001;
         destinationLocation.y = destinationLocation.y*0.9999;
      }
      pathScreen = ezgl::rectangle(destinationLocation, sourceLocation);
      ezgl::renderer* helperG = application->get_renderer();

      helperG->set_visible_world(pathScreen);
   }

   application->refresh_drawing();


   //force redraw
   
}


void autoComplete(ezgl::application* application){
   GtkListStore* search1AutoFill = GTK_LIST_STORE(application->get_object("liststore1"));

   GtkTreeIter iterator;

   gtk_list_store_clear(search1AutoFill);

   for(int street = 0; street < getNumStreets(); street++){
      std::string stringAdd = getStreetName(street);
      gtk_list_store_append(search1AutoFill, &iterator);
      gtk_list_store_set(search1AutoFill, &iterator, 0,stringAdd.c_str(), 1, street, 2, FALSE, -1);
   }

}
void toggle_night (GtkWidget* /*widget*/, ezgl::application* application) {
   //toggle bool type nightMode flag
   nightMode = !nightMode;
   
   //display message in message box
   if (nightMode) {
      std::stringstream ss;
      ss << "Night Mode On";
      application->update_message(ss.str());
   } else {
      std::stringstream ss;
      ss << "Night Mode Off";
      application->update_message(ss.str());
   }

   //force redraw
   application->refresh_drawing();
}

void toggle_find (GtkWidget* /*widget*/, ezgl::application* application) {

   //getting input from the two entry boxes created in Glade
   GObject* gtk_object1 = application->get_object("SearchEntry1");
   GObject* gtk_object2 = application->get_object("SearchEntry2");

   GtkEntry* searchEntry1 = GTK_ENTRY(gtk_object1);
   GtkEntry* searchEntry2 = GTK_ENTRY(gtk_object2);

   const gchar* street1Name = gtk_entry_get_text(searchEntry1);
   const gchar* street2Name = gtk_entry_get_text(searchEntry2);

   std::vector < StreetIdx > partialMatchesVector1;
   
   std::vector < StreetIdx > partialMatchesVector2;

   std::vector < IntersectionIdx > intersectionsOfStreets;

   //passing input prefixes to function
   partialMatchesVector1 = findStreetIdsFromPartialStreetName(street1Name);
   partialMatchesVector2 = findStreetIdsFromPartialStreetName(street2Name);

   bool nonExistant = false;
   if (partialMatchesVector1.size() == 0 || partialMatchesVector2.size() == 0) {
      nonExistant = true;
      stringstream output;
      output << "The following streets do not exist: " << endl;
      if (partialMatchesVector1.size() == 0) {
         output << street1Name << endl;
      }
      if (partialMatchesVector2.size() == 0) {
         output << street2Name << endl;
      }
      application->create_popup_message("Error", output.str().c_str());
   }
   bool foundIntersection = false;
   //if both unique results found, highlight intersection
   for(int street1 = 0; street1 < partialMatchesVector1.size(); street1++) {
      for(int street2 = 0; street2 < partialMatchesVector2.size(); street2++){
          intersectionsOfStreets = findIntersectionsOfTwoStreets(partialMatchesVector1[street1], partialMatchesVector2[street2]);

         for (int intersection = 0; intersection < intersectionsOfStreets.size(); intersection++){
            foundIntersection = true;
            intersections_xyposname[intersectionsOfStreets[intersection]].highlight = true;
         }
      }
   }
   if(!foundIntersection && !nonExistant){
         stringstream output;
         output << "There are no intersections between: " << endl;
         output << street1Name << " & ";
         output << street2Name << "." << endl;
         application->create_popup_message("Error", output.str().c_str());
      }

   //force redraw
   application->refresh_drawing();
}

void toggle_directions (GtkWidget* /*widget*/, ezgl::application* application) {

   application->create_popup_message("Get Directions", directionsText.str().c_str());
}

void toggle_help (GtkWidget* /*widget*/, ezgl::application* application) {
   std::stringstream output;
   output << "Zoom In: self-explanatory" << endl
          << "Zoom Out: self-explanatory" << endl
          << "Zoom Fit: sets visible screen to initial screen" << endl
          << "Source Street Search Bars: highlights an intersection" << endl
          << "Destination Street Search Bars: finds a path to this intersection from the first intersection that was highlighted" << endl
          << "Cities: choose what city you want to look at" << endl
          << "Night Mode: changes color of screen to better fit night-time use" << endl
          << "POIs: choose what points of interest you'd like to see on the map" << endl
          << "Get Directions: gives you instructions step by step to get from the first intersection to the second one" << endl
          << "Guide: self-explanatory" << endl
          << "Proceed: ends the program" << endl;
   application->create_popup_message("Guide", output.str().c_str());
}


//Displaying intersections as rectangles
void displayIntersectionRectangles(ezgl::renderer *g){
   for (int intersectionID = 0; intersectionID < intersections_xyposname.size(); intersectionID++) {
      
      //only display intersections that have been selected or searched 
      if (intersections_xyposname[intersectionID].highlight) {
         g->set_color(ezgl::RED);
         float width = 5;
         float height = width;
         ezgl::point2d inter_loc = intersections_xyposname[intersectionID].xy_loc - ezgl::point2d(width/2, height/2);
         g->fill_rectangle(inter_loc, width, height);
      } 

   }


}

void displayDistanceScale(ezgl::renderer *g){

   //setting line dimensions in terms of visible world to ensure it does not change relative size
   ezgl::rectangle visible_world(g->get_visible_world());
   double lineStartX = visible_world.bottom_left().x + ((visible_world.bottom_right().x - visible_world.bottom_left().x) / 10);
   double lineStartY = visible_world.bottom_left().y + ((visible_world.top_left().y - visible_world.bottom_left().y) / 10);
   double lineEndX = visible_world.bottom_left().x + (2 * ((visible_world.bottom_right().x - visible_world.bottom_left().x) / 10));
   double lineEndY = visible_world.bottom_left().y + (( visible_world.top_left().y - visible_world.bottom_left().y) / 10);

   double notchY = lineStartY - ((lineStartY - visible_world.bottom_left().y) / 10);
   if(nightMode){
      g->set_color(255, 213, 128);
   } else {
      g->set_color(0,0,0);
   }
   
   g->draw_line(ezgl::point2d(lineStartX, lineStartY), ezgl::point2d(lineEndX, lineEndY));
   g->set_line_width(1);
   g->draw_line(ezgl::point2d(lineStartX, lineStartY), ezgl::point2d(lineStartX,notchY));
   g->draw_line(ezgl::point2d(lineEndX, lineStartY), ezgl::point2d(lineEndX,notchY));

   std::string distanceString = std::to_string(int(lineEndX - lineStartX));

   std::string metersString = "metres";
   std::string distanceZero = std::to_string(0);
  
   g->draw_text(ezgl::point2d(lineStartX,notchY - ((lineStartY - visible_world.bottom_left().y) / 10)), distanceZero, 5*(lineStartY - notchY), 5*(lineStartY - notchY));
   g->draw_text(ezgl::point2d(((lineEndX + lineStartX) / 2),notchY - ((lineStartY - visible_world.bottom_left().y) / 10)), metersString, 10*(lineStartY - notchY), 10*(lineStartY - notchY));
   g->draw_text(ezgl::point2d(lineEndX,notchY - ((lineStartY - visible_world.bottom_left().y) / 10)), distanceString, 5*(lineStartY - notchY), 5*(lineStartY - notchY));
}

double findAngle(double x1, double x2, double y1, double y2) {
   double xLength = (x1 - x2);
   double yLength = (y1 - y2);
   double angle = atan(yLength/xLength);
   return ((angle*180)/3.14159265359); 
}

void displayCityNames (ezgl::renderer *g) {
   g->set_color(0, 0, 0);
   g->set_text_rotation(0);

   //for each city index stored in the vector, determine location and name from OSM Node
   for (int city = 0; city < cityIndexes.size(); city++ ) {
      std::string cityName = getOSMNodeTagValue((getNodeByIndex(cityIndexes[city])->id()), "name");
      double xPos = x_from_lon(getNodeByIndex(cityIndexes[city])->coords().longitude());
      double yPos = y_from_lat(getNodeByIndex(cityIndexes[city])->coords().latitude());
      ezgl::point2d cityLocation = ezgl::point2d(xPos, yPos);
      
      ezgl::rectangle visible_world(g->get_visible_world());
      if (visible_world.contains(cityLocation)) {
         g->draw_text(cityLocation, cityName);
      }
   }
}

void displayStreets(ezgl::renderer *g){

   int redColor = 255;
   int greenColor = 255;
   int blueColor = 255;
   
   for (int StrSegID = 0; StrSegID < getNumStreetSegments(); StrSegID++) {
      if (pathGlobalBool[StrSegID]) {
         continue;
      }

      ezgl::rectangle visibleWorld = g->get_visible_world();
      ezgl::point2d fromIntersectionPos = streetSegmentIdx_point2dxyCurvepoints[StrSegID][0];
      ezgl::point2d toIntersectionPos = streetSegmentIdx_point2dxyCurvepoints[StrSegID][streetSegmentIdx_point2dxyCurvepoints[StrSegID].size() - 1];
      ezgl::point2d midPoint = ezgl::point2d((fromIntersectionPos.x+toIntersectionPos.x)/2.0, (fromIntersectionPos.y+toIntersectionPos.y)/2.0);

      if (visibleWorld.contains(fromIntersectionPos) || visibleWorld.contains(toIntersectionPos) || visibleWorld.contains(midPoint)) {
         OSMID streetOSMID = street_segment_info[StrSegID].wayOSMID;
         std::string key = "highway";
         std::string tag = getOSMWayTagValue(streetOSMID, key);
         setDrawDetails(g, tag, zoomFactor, redColor, greenColor, blueColor);
         std::string streetName = getStreetName(street_segment_info[StrSegID].streetID);

         //setting flags
         bool displayStreetName = true;

         //do not display unknown street names
         if (streetName == "<unknown>") { 
            displayStreetName = false;
         } 

         //do not display the given street types at the zoom factor value
         if (zoomFactor >= 0.36) {
            if (tag == "residential" || tag == "residential_link" || tag == "unclassified" || tag == "unclassified_link" || tag == "road" || tag == "tertiary" || tag == "tertiary_link" 
               || streetName == "<unknown>") {
               continue;
            }
         }

         for (int curvePoint = 0; curvePoint < streetSegmentIdx_point2dxyCurvepoints[StrSegID].size() - 1; curvePoint++) {


            g->draw_line(streetSegmentIdx_point2dxyCurvepoints[StrSegID][curvePoint],streetSegmentIdx_point2dxyCurvepoints[StrSegID][curvePoint + 1]);
            if (!pathGlobalBool[StrSegID] && (tag == "motorway" || tag == "motorway_link" || tag == "trunk" || tag == "trunk_link" || tag == "primary" || tag == "primary_link")) {
               g->set_line_width(lineWidth-2);
               g->set_color(253, 226, 147);
               g->draw_line(streetSegmentIdx_point2dxyCurvepoints[StrSegID][curvePoint],streetSegmentIdx_point2dxyCurvepoints[StrSegID][curvePoint + 1]);
               g->set_line_width(lineWidth);
               g->set_color(redColor, greenColor, blueColor);
            }

            ezgl::point2d startCoord = streetSegmentIdx_point2dxyCurvepoints[StrSegID][curvePoint];
            ezgl::point2d endCoord = streetSegmentIdx_point2dxyCurvepoints[StrSegID][curvePoint+1];

            ezgl::point2d midPoint2 = startCoord +  endCoord;
            midPoint2.x = (midPoint2.x)/2.0;
            midPoint2.y = (midPoint2.y)/2.0;
            double segmentLength = sqrt (pow(startCoord.x - endCoord.x, 2.0) + pow(startCoord.y - endCoord.y, 2.0));

            if (startCoord.x != endCoord.x) {
               double angle = atan((startCoord.y - endCoord.y)/(startCoord.x - endCoord.x));
               angle = (angle * 180)/M_PI;
               g->set_text_rotation(angle);
            } else {
               g->set_text_rotation(270.0);
            }

            std::string leftArrow = "<-";
            std::string rightArrow = "->";

            //displaying street names for non-one way streets
            if (displayStreetName && (curvePoint%3==0)) {
               g->set_color(0, 0, 0);
               g->draw_text(midPoint2, streetName, segmentLength, segmentLength);
               
            } else if (street_segment_info[StrSegID].oneWay && !(curvePoint%3==0) && (zoomFactor < 0.046656)) {
               if ((streetSegmentIdx_point2dxyCurvepoints[StrSegID][curvePoint].x < midPoint2.x)) {
                  g->set_color(0, 0, 0);
                  g->draw_text(midPoint2, rightArrow, segmentLength, segmentLength);
               } else if ((streetSegmentIdx_point2dxyCurvepoints[StrSegID][curvePoint].x > midPoint2.x)) {
                  g->set_color(0, 0, 0);
                  g->draw_text(midPoint2, leftArrow, segmentLength, segmentLength);
               }
            }
            g->set_color(redColor, greenColor, blueColor);

         }

      }
      

   }
   for (int streetSeg = 0; streetSeg < pathGlobal.size(); streetSeg++) {

      int streetSegID = pathGlobal[streetSeg];

      ezgl::rectangle visibleWorldForPath = g->get_visible_world();
      ezgl::point2d fromIntersectionPosPath = streetSegmentIdx_point2dxyCurvepoints[streetSegID][0];
      ezgl::point2d toIntersectionPosPath = streetSegmentIdx_point2dxyCurvepoints[streetSegID][streetSegmentIdx_point2dxyCurvepoints[streetSegID].size() - 1];
      ezgl::point2d midPointPath = ezgl::point2d((fromIntersectionPosPath.x+toIntersectionPosPath.x)/2.0, (fromIntersectionPosPath.y+toIntersectionPosPath.y)/2.0);

      if (visibleWorldForPath.contains(fromIntersectionPosPath) || visibleWorldForPath.contains(toIntersectionPosPath) || visibleWorldForPath.contains(midPointPath)) {
         OSMID streetOSMID = street_segment_info[streetSegID].wayOSMID;
         std::string key = "highway";
         std::string tag = getOSMWayTagValue(streetOSMID, key);
         setDrawDetails(g, tag, zoomFactor, redColor, greenColor, blueColor);
         std::string streetName = getStreetName(street_segment_info[streetSegID].streetID);

         //setting flags
         bool displayStreetName = true;

         //do not display unknown street names
         if (streetName == "<unknown>") { 
            displayStreetName = false;
         } 

         for (int curvePoint = 0; curvePoint < streetSegmentIdx_point2dxyCurvepoints[streetSegID].size() - 1; curvePoint++) {

            g->set_color(176, 87, 255);
            g->set_line_width(5);
            g->draw_line(streetSegmentIdx_point2dxyCurvepoints[streetSegID][curvePoint],streetSegmentIdx_point2dxyCurvepoints[streetSegID][curvePoint + 1]);
            
            ezgl::point2d startCoord = streetSegmentIdx_point2dxyCurvepoints[streetSegID][curvePoint];
            ezgl::point2d endCoord = streetSegmentIdx_point2dxyCurvepoints[streetSegID][curvePoint+1];

            ezgl::point2d midPoint2 = startCoord +  endCoord;
            midPoint2.x = (midPoint2.x)/2.0;
            midPoint2.y = (midPoint2.y)/2.0;
            double segmentLength = sqrt (pow(startCoord.x - endCoord.x, 2.0) + pow(startCoord.y - endCoord.y, 2.0));

            if (startCoord.x != endCoord.x) {
               double angle = atan((startCoord.y - endCoord.y)/(startCoord.x - endCoord.x));
               angle = (angle * 180)/M_PI;
               g->set_text_rotation(angle);
            } else {
               g->set_text_rotation(270.0);
            }

            std::string leftArrow = "<-";
            std::string rightArrow = "->";

            //displaying street names for non-one way streets
            if (displayStreetName && (curvePoint%3==0)) {
               g->set_color(0, 0, 0);
               g->draw_text(midPoint2, streetName, segmentLength, segmentLength);
               
            } else if (street_segment_info[streetSegID].oneWay && !(curvePoint%3==0) && (zoomFactor < 0.046656)) {
               if ((streetSegmentIdx_point2dxyCurvepoints[streetSegID][curvePoint].x < midPoint2.x)) {
                  g->set_color(0, 0, 0);
                  g->draw_text(midPoint2, rightArrow, segmentLength, segmentLength);
               } else if ((streetSegmentIdx_point2dxyCurvepoints[streetSegID][curvePoint].x > midPoint2.x)) {
                  g->set_color(0, 0, 0);
                  g->draw_text(midPoint2, leftArrow, segmentLength, segmentLength);
               }
            }


         }

      }
   }
}

void setDrawDetails(ezgl::renderer *g, std::string tag, double zoomFactorGlobal, int& red, int& green, int& blue) {
   g->set_color(255, 255, 255);
   red = 255;
   green = 255;
   blue = 255;
   
   if (tag == "motorway" || tag == "motorway_link") {
      g->set_color(255, 213, 128); //orange 
      if (zoomFactorGlobal >= 0.6) {
         g->set_line_width (4.5);
         lineWidth = 4.5;
      } else if (zoomFactorGlobal >= 0.36) {
         g->set_line_width (5);
         lineWidth = 5;
      } else if (zoomFactorGlobal >= 0.216) {
         g->set_line_width (5);
         lineWidth = 5;
      } else {
         g->set_line_width (5);
         lineWidth = 5;
      }
      red = 255;
      green = 213;
      blue = 128;


   } else if (tag == "trunk" || tag == "trunk_link") {
      g->set_color(255, 213, 128); //orange
      if (zoomFactorGlobal >= 0.6) {
         g->set_line_width (4.5);
         lineWidth = 4.5;
      } else if (zoomFactorGlobal >= 0.36) {
         g->set_line_width (5);
         lineWidth = 5;
      } else if (zoomFactorGlobal >= 0.216) {
         g->set_line_width (5);
         lineWidth = 5;
      } else {
         g->set_line_width (5);
         lineWidth = 5;
      }
      red = 255;
      green = 213;
      blue = 128;


   } else if (tag == "primary" || tag == "primary_link") {
      g->set_color(255, 213, 128); //orange
      if (zoomFactorGlobal >= 0.6) {
         g->set_line_width (3.5);
         lineWidth = 3.5;
      } else if (zoomFactorGlobal >= 0.36) {
         g->set_line_width (4);
         lineWidth = 4;
      } else if (zoomFactorGlobal >= 0.216) {
         g->set_line_width (4.5);
         lineWidth = 4.5;
      } else {
         g->set_line_width (4.5);
         lineWidth = 4.5;
      }
      red = 255;
      green = 213;
      blue = 128;


   } else if (tag == "secondary" || tag == "secondary_link") {
      g->set_color(255, 255, 255);
      if (zoomFactorGlobal >= 0.6) {
         g->set_line_width (2);
      } else if (zoomFactorGlobal >= 0.36) {
         g->set_line_width (3);
      } else if (zoomFactorGlobal >= 0.216) {
         g->set_line_width (4);
      } else {
         g->set_line_width (4.5);
      }
      red = 255;
      green = 255;
      blue = 255;


   } else if (tag == "tertiary" || tag == "tertiary_link") {
      g->set_color(255, 255, 255);
      if (zoomFactorGlobal >= 0.6) {
         g->set_line_width (0.5);
      } else if (zoomFactorGlobal >= 0.36) {
         g->set_line_width (0.75);
      } else if (zoomFactorGlobal >= 0.216) {
         g->set_line_width (1.25);
      } else if (zoomFactorGlobal >= 0.1296){
         g->set_line_width (3.5);
      } else {
         g->set_line_width (4);
      }
      red = 255;
      green = 255;
      blue = 255;


   } else if (tag == "road") {
      g->set_color(255, 255, 255);
      if (zoomFactorGlobal >= 0.6) {
         g->set_line_width (0.75);
      } else if (zoomFactorGlobal >= 0.36) {
         g->set_line_width (1.25);
      } else if (zoomFactorGlobal >= 0.216) {
         g->set_line_width (1.75);
      } else if (zoomFactorGlobal >= 0.1296) {
         g->set_line_width (2.5);
      } else {
         g->set_line_width (3);
      }
      red = 255;
      green = 255;
      blue = 255;


   } else if (tag == "unclassified" || tag == "unclassified_link") {
      g->set_color(255, 255, 255);
      if (zoomFactorGlobal >= 0.6) {
         g->set_line_width (0.5);
      } else if (zoomFactorGlobal >= 0.36) {
         g->set_line_width (1);
      } else if (zoomFactorGlobal >= 0.216) {
         g->set_line_width (1.5);
      } else if (zoomFactorGlobal >= 0.1296) {
         g->set_line_width (2.5);
      } else {
         g->set_line_width (3);
      }
      red = 255;
      green = 255;
      blue = 255;


   } else if (tag == "residential" || tag == "residential_link") {
      g->set_color(255, 255, 255);
      if (zoomFactorGlobal >= 0.6) {
         g->set_line_width (0.5);
      } else if (zoomFactorGlobal >= 0.36) {
         g->set_line_width (1);
      } else if (zoomFactorGlobal >= 0.216) {
         g->set_line_width (1.5);
      } else if (zoomFactorGlobal >= 0.1296) {
         g->set_line_width (2.5);
      } else {
         g->set_line_width (3);
      }
      red = 255;
      green = 255;
      blue = 255;

   }

   return;
}

//Displaying all features
void displayFeatures(ezgl::renderer *g){
   for(int featureID = 0; featureID < getNumFeatures(); ++featureID){
         
      if(Features[featureID].numFeaturePoints > 1){

         if(Features[featureID].type == "park"){
            g->set_color(179, 222, 191);
            g->set_line_width(2);
            drawFeature(g, featureID);
         } else if(Features[featureID].type == "beach"){
            g->set_color(254, 239, 195);
            g->set_line_width(2);
            drawFeature(g, featureID);
         } else if(Features[featureID].type == "lake"){
            g->set_color(156, 192, 249);
            g->set_line_width(2);
            drawFeature(g, featureID);
         } else if(Features[featureID].type == "beach"){
            g->set_color(254, 239, 195);
            g->set_line_width(2);
            drawFeature(g, featureID);
         } else if(Features[featureID].type == "river"){
            g->set_color(156, 192, 249);
            g->set_line_width(2);
            drawFeature(g, featureID);
         } else if(Features[featureID].type == "island"){
            g->set_color(230, 230, 230);
            g->set_line_width(2);
            drawFeature(g, featureID);
         } else if(Features[featureID].type == "building"){
            if(zoomFactor <= 0.07776){
               g->set_color(200, 204, 208);
               g->set_line_width(2);
               drawFeature(g, featureID);
            }
         } else if(Features[featureID].type == "greenspace"){
            g->set_color(160, 214, 174);
            g->set_line_width(2);
            drawFeature(g, featureID);
         } else if(Features[featureID].type == "golfcourse"){
            g->set_color(160, 214, 174);
            g->set_line_width(2);
            drawFeature(g, featureID);
         } else if(Features[featureID].type == "stream"){
            if(zoomFactor <= 0.1296){
               g->set_color(156, 192, 249);
               g->set_line_width(2);
               drawFeature(g, featureID);
            }
         } else if(Features[featureID].type == "glacier"){
            g->set_color(255, 255, 255);
            g->set_line_width(2);
            drawFeature(g, featureID);
         }

      }
   }
}

void drawFeature(ezgl::renderer *g, int featureID){
   if (Features[featureID].min_x < (g->get_visible_world().right()) && Features[featureID].max_x > (g->get_visible_world().left()) &&
      Features[featureID].min_y < (g->get_visible_world().top()) && Features[featureID].max_y > (g->get_visible_world().bottom()))  {
      if(Features[featureID].featurePoints[0] == Features[featureID].featurePoints[Features[featureID].numFeaturePoints]){
            g->fill_poly(Features[featureID].featurePoints);
      } else {
            for(int featurePoint = 0; featurePoint < Features[featureID].numFeaturePoints; featurePoint++){
               g->draw_line(Features[featureID].featurePoints[featurePoint], Features[featureID].featurePoints[featurePoint+1]);
            }
         }
   }
}

//choosing city to load from the combo box by toggling the cititesBool data type
void toggle_cities(GtkComboBoxText* self, ezgl::application* application){
  
   if(gtk_combo_box_text_get_active_text(self) == cities[0]){
   }
   if(gtk_combo_box_text_get_active_text(self) == cities[1]){
      allCitiesBool.toronto = true;
      application->refresh_drawing();
   }
   if(gtk_combo_box_text_get_active_text(self) == cities[2]){
      allCitiesBool.beijing = true;
      application->refresh_drawing();
   }
   if(gtk_combo_box_text_get_active_text(self) == cities[3]){
      allCitiesBool.cairo = true;
      application->refresh_drawing();
   }
   if(gtk_combo_box_text_get_active_text(self) == cities[4]){
      allCitiesBool.capeTown = true;
      application->refresh_drawing();
   }
   if(gtk_combo_box_text_get_active_text(self) == cities[5]){
      allCitiesBool.goldenHorseShoe = true;
      application->refresh_drawing();
   }
   if(gtk_combo_box_text_get_active_text(self) == cities[6]){
      allCitiesBool.hamilton = true;
      application->refresh_drawing();
   }
   if(gtk_combo_box_text_get_active_text(self) == cities[7]){
      allCitiesBool.hongKong = true;
      application->refresh_drawing();
   }
   if(gtk_combo_box_text_get_active_text(self) == cities[8]){
      allCitiesBool.iceland = true;
      application->refresh_drawing();
   }
   if(gtk_combo_box_text_get_active_text(self) == cities[9]){
      allCitiesBool.interLaken = true;
      application->refresh_drawing();
   }
   if(gtk_combo_box_text_get_active_text(self) == cities[10]){
      allCitiesBool.kyiv = true;
      application->refresh_drawing();
   }
   if(gtk_combo_box_text_get_active_text(self) == cities[11]){
      allCitiesBool.london = true;
      application->refresh_drawing();
   }
   if(gtk_combo_box_text_get_active_text(self) == cities[12]){
      allCitiesBool.newDelhi = true;
      application->refresh_drawing();
   }
   if(gtk_combo_box_text_get_active_text(self) == cities[13]){
      allCitiesBool.newYork = true;
      application->refresh_drawing();
   }
   if(gtk_combo_box_text_get_active_text(self) == cities[14]){
      allCitiesBool.rio = true;
      application->refresh_drawing();
   }
   if(gtk_combo_box_text_get_active_text(self) == cities[15]){
      allCitiesBool.saintHelena = true;
      application->refresh_drawing();
   }
   if(gtk_combo_box_text_get_active_text(self) == cities[16]){
      allCitiesBool.singapore = true;
      application->refresh_drawing();
   }
   if(gtk_combo_box_text_get_active_text(self) == cities[17]){
      allCitiesBool.sydney = true;
      application->refresh_drawing();
   }
   if(gtk_combo_box_text_get_active_text(self) == cities[18]){
      allCitiesBool.tehran = true;
      application->refresh_drawing();
   }
   if(gtk_combo_box_text_get_active_text(self) == cities[19]){
      allCitiesBool.tokyo = true;
      application->refresh_drawing();
   }
}

//change map depending on the allCitiesBool flag
void changeMap(){
  
   if(allCitiesBool.toronto){
      allCitiesBool.toronto = false;
      pathGlobal.clear();
      pathGlobalBool.clear();
      closeMap();
      loadMap("/cad2/ece297s/public/maps/toronto_canada.streets.bin");
      autoComplete(applicationPtr);
      ezgl::rectangle initial_world({x_from_lon(min_lon), y_from_lat(min_lat)}, {x_from_lon(max_lon), y_from_lat(max_lat)});
      applicationPtr->change_canvas_world_coordinates(applicationPtr->get_main_canvas_id(), initial_world);
      applicationPtr->refresh_drawing();
   }
   if(allCitiesBool.london){
      allCitiesBool.london = false;
      pathGlobal.clear();
      pathGlobalBool.clear();
      closeMap();
      loadMap("/cad2/ece297s/public/maps/london_england.streets.bin");
      autoComplete(applicationPtr);
      ezgl::rectangle initial_world({x_from_lon(min_lon), y_from_lat(min_lat)}, {x_from_lon(max_lon), y_from_lat(max_lat)});
      applicationPtr->change_canvas_world_coordinates(applicationPtr->get_main_canvas_id(), initial_world);
      applicationPtr->refresh_drawing();
   }
   if(allCitiesBool.beijing){
      allCitiesBool.beijing = false;
      pathGlobal.clear();
      pathGlobalBool.clear();
      closeMap();
      loadMap("/cad2/ece297s/public/maps/beijing_china.streets.bin");
      autoComplete(applicationPtr);
      ezgl::rectangle initial_world({x_from_lon(min_lon), y_from_lat(min_lat)}, {x_from_lon(max_lon), y_from_lat(max_lat)});
      applicationPtr->change_canvas_world_coordinates(applicationPtr->get_main_canvas_id(), initial_world);
      applicationPtr->refresh_drawing();
   }
   if(allCitiesBool.cairo){
      allCitiesBool.cairo = false;
      pathGlobal.clear();
      pathGlobalBool.clear();
      closeMap();
      loadMap("/cad2/ece297s/public/maps/cairo_egypt.streets.bin");
      autoComplete(applicationPtr);
      ezgl::rectangle initial_world({x_from_lon(min_lon), y_from_lat(min_lat)}, {x_from_lon(max_lon), y_from_lat(max_lat)});
      applicationPtr->change_canvas_world_coordinates(applicationPtr->get_main_canvas_id(), initial_world);
      applicationPtr->refresh_drawing();
   }
   if(allCitiesBool.capeTown){
      allCitiesBool.capeTown = false;
      pathGlobal.clear();
      pathGlobalBool.clear();
      closeMap();
      loadMap("/cad2/ece297s/public/maps/cape-town_south-africa.streets.bin");
      autoComplete(applicationPtr);
      ezgl::rectangle initial_world({x_from_lon(min_lon), y_from_lat(min_lat)}, {x_from_lon(max_lon), y_from_lat(max_lat)});
      applicationPtr->change_canvas_world_coordinates(applicationPtr->get_main_canvas_id(), initial_world);
      applicationPtr->refresh_drawing();
   }
   if(allCitiesBool.hamilton){
      allCitiesBool.hamilton = false;
      pathGlobal.clear();
      pathGlobalBool.clear();
      closeMap();
      loadMap("/cad2/ece297s/public/maps/hamilton_canada.streets.bin");
      autoComplete(applicationPtr);
      ezgl::rectangle initial_world({x_from_lon(min_lon), y_from_lat(min_lat)}, {x_from_lon(max_lon), y_from_lat(max_lat)});
      applicationPtr->change_canvas_world_coordinates(applicationPtr->get_main_canvas_id(), initial_world);
      applicationPtr->refresh_drawing();
   }
   if(allCitiesBool.goldenHorseShoe){
      allCitiesBool.goldenHorseShoe = false;
      pathGlobal.clear();
      pathGlobalBool.clear();
      closeMap();
      loadMap("/cad2/ece297s/public/maps/golden-horseshoe_canada.streets.bin");
      autoComplete(applicationPtr);
      ezgl::rectangle initial_world({x_from_lon(min_lon), y_from_lat(min_lat)}, {x_from_lon(max_lon), y_from_lat(max_lat)});
      applicationPtr->change_canvas_world_coordinates(applicationPtr->get_main_canvas_id(), initial_world);
      applicationPtr->refresh_drawing();
   }
   if(allCitiesBool.hongKong){
      allCitiesBool.hongKong = false;
      pathGlobal.clear();
      pathGlobalBool.clear();
      closeMap();
      loadMap("/cad2/ece297s/public/maps/hong-kong_china.streets.bin");
      autoComplete(applicationPtr);
      ezgl::rectangle initial_world({x_from_lon(min_lon), y_from_lat(min_lat)}, {x_from_lon(max_lon), y_from_lat(max_lat)});
      applicationPtr->change_canvas_world_coordinates(applicationPtr->get_main_canvas_id(), initial_world);
      applicationPtr->refresh_drawing();
   }
   if(allCitiesBool.iceland){
      allCitiesBool.iceland = false;
      pathGlobal.clear();
      pathGlobalBool.clear();
      closeMap();
      loadMap("/cad2/ece297s/public/maps/iceland.streets.bin");
      autoComplete(applicationPtr);
      ezgl::rectangle initial_world({x_from_lon(min_lon), y_from_lat(min_lat)}, {x_from_lon(max_lon), y_from_lat(max_lat)});
      applicationPtr->change_canvas_world_coordinates(applicationPtr->get_main_canvas_id(), initial_world);
      applicationPtr->refresh_drawing();
   }
   if(allCitiesBool.interLaken){
      allCitiesBool.interLaken = false;
      pathGlobal.clear();
      pathGlobalBool.clear();
      closeMap();
      loadMap("/cad2/ece297s/public/maps/interlaken_switzerland.streets.bin");
      autoComplete(applicationPtr);
      ezgl::rectangle initial_world({x_from_lon(min_lon), y_from_lat(min_lat)}, {x_from_lon(max_lon), y_from_lat(max_lat)});
      applicationPtr->change_canvas_world_coordinates(applicationPtr->get_main_canvas_id(), initial_world);
      applicationPtr->refresh_drawing();
   }
   if(allCitiesBool.kyiv){
      allCitiesBool.kyiv = false;
      pathGlobal.clear();
      pathGlobalBool.clear();
      closeMap();
      loadMap("/cad2/ece297s/public/maps/kyiv_ukraine.streets.bin");
      autoComplete(applicationPtr);
      ezgl::rectangle initial_world({x_from_lon(min_lon), y_from_lat(min_lat)}, {x_from_lon(max_lon), y_from_lat(max_lat)});
      applicationPtr->change_canvas_world_coordinates(applicationPtr->get_main_canvas_id(), initial_world);
      applicationPtr->refresh_drawing();
   }
   if(allCitiesBool.newDelhi){
      allCitiesBool.newDelhi = false;
      pathGlobal.clear();
      pathGlobalBool.clear();
      closeMap();
      loadMap("/cad2/ece297s/public/maps/new-delhi_india.streets.bin");
      autoComplete(applicationPtr);
      ezgl::rectangle initial_world({x_from_lon(min_lon), y_from_lat(min_lat)}, {x_from_lon(max_lon), y_from_lat(max_lat)});
      applicationPtr->change_canvas_world_coordinates(applicationPtr->get_main_canvas_id(), initial_world);
      applicationPtr->refresh_drawing();
   }
   if(allCitiesBool.newYork){
      allCitiesBool.newYork = false;
      pathGlobal.clear();
      pathGlobalBool.clear();
      closeMap();
      loadMap("/cad2/ece297s/public/maps/new-york_usa.streets.bin");
      autoComplete(applicationPtr);
      ezgl::rectangle initial_world({x_from_lon(min_lon), y_from_lat(min_lat)}, {x_from_lon(max_lon), y_from_lat(max_lat)});
      applicationPtr->change_canvas_world_coordinates(applicationPtr->get_main_canvas_id(), initial_world);
      applicationPtr->refresh_drawing();
   }
   if(allCitiesBool.rio){
      allCitiesBool.rio = false;
      pathGlobal.clear();
      pathGlobalBool.clear();
      closeMap();
      loadMap("/cad2/ece297s/public/maps/rio-de-janeiro_brazil.streets.bin");
      autoComplete(applicationPtr);
      ezgl::rectangle initial_world({x_from_lon(min_lon), y_from_lat(min_lat)}, {x_from_lon(max_lon), y_from_lat(max_lat)});
      applicationPtr->change_canvas_world_coordinates(applicationPtr->get_main_canvas_id(), initial_world);
      applicationPtr->refresh_drawing();
   }
   if(allCitiesBool.saintHelena){
      allCitiesBool.saintHelena = false;
      pathGlobal.clear();
      pathGlobalBool.clear();
      closeMap();
      loadMap("/cad2/ece297s/public/maps/saint-helena.streets.bin");
      autoComplete(applicationPtr);
      ezgl::rectangle initial_world({x_from_lon(min_lon), y_from_lat(min_lat)}, {x_from_lon(max_lon), y_from_lat(max_lat)});
      applicationPtr->change_canvas_world_coordinates(applicationPtr->get_main_canvas_id(), initial_world);
      applicationPtr->refresh_drawing();
   }
   if(allCitiesBool.singapore){
      allCitiesBool.singapore = false;
      pathGlobal.clear();
      pathGlobalBool.clear();
      closeMap();
      loadMap("/cad2/ece297s/public/maps/singapore.streets.bin");
      autoComplete(applicationPtr);
      ezgl::rectangle initial_world({x_from_lon(min_lon), y_from_lat(min_lat)}, {x_from_lon(max_lon), y_from_lat(max_lat)});
      applicationPtr->change_canvas_world_coordinates(applicationPtr->get_main_canvas_id(), initial_world);
      applicationPtr->refresh_drawing();
   }
   if(allCitiesBool.sydney){
      allCitiesBool.sydney = false;
      pathGlobal.clear();
      pathGlobalBool.clear();
      closeMap();
      loadMap("/cad2/ece297s/public/maps/sydney_australia.streets.bin");
      autoComplete(applicationPtr);
      ezgl::rectangle initial_world({x_from_lon(min_lon), y_from_lat(min_lat)}, {x_from_lon(max_lon), y_from_lat(max_lat)});
      applicationPtr->change_canvas_world_coordinates(applicationPtr->get_main_canvas_id(), initial_world);
      applicationPtr->refresh_drawing();
   }
   if(allCitiesBool.tehran){
      allCitiesBool.tehran = false;
      pathGlobal.clear();
      pathGlobalBool.clear();
      closeMap();
      loadMap("/cad2/ece297s/public/maps/tehran_iran.streets.bin");
      autoComplete(applicationPtr);
      ezgl::rectangle initial_world({x_from_lon(min_lon), y_from_lat(min_lat)}, {x_from_lon(max_lon), y_from_lat(max_lat)});
      applicationPtr->change_canvas_world_coordinates(applicationPtr->get_main_canvas_id(), initial_world);
      applicationPtr->refresh_drawing();
   }
   if(allCitiesBool.tokyo){
      allCitiesBool.tokyo = false;
      pathGlobal.clear();
      pathGlobalBool.clear();
      closeMap();
      loadMap("/cad2/ece297s/public/maps/tokyo_japan.streets.bin");
      autoComplete(applicationPtr);
      ezgl::rectangle initial_world({x_from_lon(min_lon), y_from_lat(min_lat)}, {x_from_lon(max_lon), y_from_lat(max_lat)});
      applicationPtr->change_canvas_world_coordinates(applicationPtr->get_main_canvas_id(), initial_world);
      applicationPtr->refresh_drawing();
   }
}

//Display POIs as icons
void displayPOI(ezgl::renderer *g) {
   int numPOIS = getNumPointsOfInterest();

   g->set_text_rotation(0);
   float rectangle_size = 10;
   float zoomNames = 0.01;

   for (int poiID = 0; poiID < numPOIS; poiID++) {
      ezgl::point2d poi_loc = poi_information[poiID].xy_loc;

      //code to print out shape(will be icon for each type)
      // g->set_color(64, 224, 208);
      // g->fill_rectangle(poi_loc - ezgl::point2d(rectangle_size/2, rectangle_size/2), rectangle_size, rectangle_size);
      if (poi_loc.x > g->get_visible_world().right() || poi_loc.x < g->get_visible_world().left() ||
          poi_loc.y > g->get_visible_world().top() || poi_loc.y < g->get_visible_world().bottom()) {
         continue;
      }

      if (poiFilterBool.Food && poi_information[poiID].type == "Food") {
         // g->set_color(64, 224, 208);
         // g->fill_rectangle(poi_loc - ezgl::point2d(rectangle_size/2, rectangle_size/2), rectangle_size, rectangle_size);
         if (zoomFactor*0.216 <= zoomNames) {
            displayIcon(g, poi_loc, 1);
         }
         if (zoomFactor <= zoomNames) {
            g->set_color(0, 0 ,0);
            poi_loc = ezgl::point2d(poi_loc.x, poi_loc.y + 5);
            g->draw_text(poi_loc, poi_information[poiID].name);
         }
      }
      else if (poiFilterBool.Education && poi_information[poiID].type == "Education") {
         // g->set_color(64, 224, 208);
         // g->fill_rectangle(poi_loc - ezgl::point2d(rectangle_size/2, rectangle_size/2), rectangle_size, rectangle_size);
         if (zoomFactor*0.216 <= zoomNames) {
            displayIcon(g, poi_loc, 1);
         }
         if (zoomFactor <= zoomNames) {
            g->set_color(0, 0 ,0);
            poi_loc = ezgl::point2d(poi_loc.x, poi_loc.y + 5);
            g->draw_text(poi_loc, poi_information[poiID].name);
         }
      }
      else if (poiFilterBool.Transportation && poi_information[poiID].type == "Transportation") {
         // g->set_color(64, 224, 208);
         // g->fill_rectangle(poi_loc - ezgl::point2d(rectangle_size/2, rectangle_size/2), rectangle_size, rectangle_size);
         if (zoomFactor*0.36 <= zoomNames) {
            displayIcon(g, poi_loc, 1);
         }
         if (zoomFactor <= zoomNames) {
            g->set_color(0, 0 ,0);
            poi_loc = ezgl::point2d(poi_loc.x, poi_loc.y + 5);
            g->draw_text(poi_loc, poi_information[poiID].name);
         }
      }
      else if (poiFilterBool.Financial && poi_information[poiID].type == "Financial") {
         // g->set_color(64, 224, 208);
         // g->fill_rectangle(poi_loc - ezgl::point2d(rectangle_size/2, rectangle_size/2), rectangle_size, rectangle_size);
         if (zoomFactor*0.216 <= zoomNames) {
            displayIcon(g, poi_loc, 1);
         }
         if (zoomFactor <= zoomNames) {
            g->set_color(0, 0 ,0);
            poi_loc = ezgl::point2d(poi_loc.x, poi_loc.y + 5);
            g->draw_text(poi_loc, poi_information[poiID].name);
         }
      }
      else if (poiFilterBool.Healthcare && poi_information[poiID].type == "Healthcare") {
         // g->set_color(64, 224, 208);
         // g->fill_rectangle(poi_loc - ezgl::point2d(rectangle_size/2, rectangle_size/2), rectangle_size, rectangle_size);
         if (zoomFactor*0.216 <= zoomNames) {
            displayIcon(g, poi_loc, 1);
         }
         if (zoomFactor <= zoomNames) {
            g->set_color(0, 0 ,0);
            poi_loc = ezgl::point2d(poi_loc.x, poi_loc.y + 5);
            g->draw_text(poi_loc, poi_information[poiID].name);
         }
      }
      else if (poiFilterBool.Entertainment && poi_information[poiID].type == "Entertainment") {
         // g->set_color(64, 224, 208);
         // g->fill_rectangle(poi_loc - ezgl::point2d(rectangle_size/2, rectangle_size/2), rectangle_size, rectangle_size);
         if (zoomFactor*0.216 <= zoomNames) {
            displayIcon(g, poi_loc, 1);
         }
         if (zoomFactor <= zoomNames) {
            g->set_color(0, 0 ,0);
            poi_loc = ezgl::point2d(poi_loc.x, poi_loc.y + 5);
            g->draw_text(poi_loc, poi_information[poiID].name);
         }
      }
      else if (poiFilterBool.Public && poi_information[poiID].type == "Public") {
         // g->set_color(64, 224, 208);
         // g->fill_rectangle(poi_loc - ezgl::point2d(rectangle_size/2, rectangle_size/2), rectangle_size, rectangle_size);
         if (zoomFactor*0.216 <= zoomNames) {
            displayIcon(g, poi_loc, 1);
         }
         if (zoomFactor <= zoomNames) {
            g->set_color(0, 0 ,0);
            poi_loc = ezgl::point2d(poi_loc.x, poi_loc.y + 5);
            g->draw_text(poi_loc, poi_information[poiID].name);
         }
      }
      else if (poiFilterBool.All) {
         g->set_color(64, 224, 208);
         if (poi_information[poiID].type == "Other") {
            g->fill_rectangle(poi_loc - ezgl::point2d(rectangle_size/4, rectangle_size/4), rectangle_size/2, rectangle_size/2);
         }
         else {
            g->fill_rectangle(poi_loc - ezgl::point2d(rectangle_size/2, rectangle_size/2), rectangle_size, rectangle_size);
         }
         if (zoomFactor <= 0.005) { // different zoom level for names when everything is displayed
            g->set_color(0, 0 ,0);
            poi_loc = ezgl::point2d(poi_loc.x, poi_loc.y + 5);
            g->draw_text(poi_loc, poi_information[poiID].name);
         }
      }
   }
   return;
}

void toggle_poi_filter(GtkComboBoxText* self, ezgl::application* application){
  
   if(gtk_combo_box_text_get_active_text(self) == POIs[0]){
      deactivatePOIS();
      application->refresh_drawing();
   }
   if(gtk_combo_box_text_get_active_text(self) == POIs[1]){
      deactivatePOIS();
      poiFilterBool.Food = true;
      application->refresh_drawing();
   }
   if(gtk_combo_box_text_get_active_text(self) == POIs[2]){
      deactivatePOIS();
      poiFilterBool.Education = true;
      application->refresh_drawing();
   }
   if(gtk_combo_box_text_get_active_text(self) == POIs[3]){
      deactivatePOIS();
      poiFilterBool.Transportation = true;
      application->refresh_drawing();
   }
   if(gtk_combo_box_text_get_active_text(self) == POIs[4]){
      deactivatePOIS();
      poiFilterBool.Financial = true;
      application->refresh_drawing();
   }
   if(gtk_combo_box_text_get_active_text(self) == POIs[5]){
      deactivatePOIS();
      poiFilterBool.Healthcare = true;
      application->refresh_drawing();
   }
   if(gtk_combo_box_text_get_active_text(self) == POIs[6]){
      deactivatePOIS();
      poiFilterBool.Entertainment = true;
      application->refresh_drawing();
   }
   if(gtk_combo_box_text_get_active_text(self) == POIs[7]){
      deactivatePOIS();
      poiFilterBool.Public = true;
      application->refresh_drawing();
   }
   if(gtk_combo_box_text_get_active_text(self) == POIs[8]){
      deactivatePOIS();
      poiFilterBool.All = true;
      application->refresh_drawing();
   }
}

void deactivatePOIS() {
   poiFilterBool.Food = false;
   poiFilterBool.Education = false;
   poiFilterBool.Transportation = false;
   poiFilterBool.Financial = false;
   poiFilterBool.Healthcare = false;
   poiFilterBool.Entertainment = false;
   poiFilterBool.Public = false;
   poiFilterBool.All = false;
}

void displayIcon(ezgl::renderer *g, ezgl::point2d xy_loc, int sizeMult) {
   ezgl::surface *icon = ezgl::renderer::load_png("libstreetmap/resources/icons8-visit-24.png");
   //pin from https://icons8.com/icon/ZjfbKqNCOojT/visit
   g->draw_surface(icon, xy_loc, icon_size*sizeMult);
}

/*

void displayFood(ezgl::renderer *g, ezgl::point2d xy_loc) {
   ezgl::surface *icon = ezgl::renderer::load_png("libstreetmap/resources/icons8-visit-24.png");
   //pin from https://icons8.com/icon/ZjfbKqNCOojT/visit
   g->draw_surface(icon, xy_loc, icon_size);
}


void displayEducation(ezgl::renderer *g, ezgl::point2d xy_loc) {
   ezgl::surface *icon = ezgl::renderer::load_png("libstreetmap/resources/noun-map-pin-university-1661313.png");
   //map pin university by b farias from https://thenounproject.com/browse/icons/term/map-pin-university/
   g->draw_surface(icon, xy_loc, icon_size);
}

void displayTransportation(ezgl::renderer *g, ezgl::point2d xy_loc) {
   ezgl::surface *icon = ezgl::renderer::load_png("libstreetmap/resources/noun-map-pin-bus-stop-1661272.png");
   //map pin bus stop by b farias from https://thenounproject.com/browse/icons/term/map-pin-bus-stop/
   g->draw_surface(icon, xy_loc, icon_size);
}

void displayFinancial(ezgl::renderer *g, ezgl::point2d xy_loc) {
   ezgl::surface *icon = ezgl::renderer::load_png("libstreetmap/resources/noun-bank-map-pin-1661310.png");
   //bank map pin by b farias from https://thenounproject.com/browse/icons/term/bank-map-pin/
   g->draw_surface(icon, xy_loc, icon_size);
}

void displayHealthcare(ezgl::renderer *g, ezgl::point2d xy_loc) {
   ezgl::surface *icon = ezgl::renderer::load_png("libstreetmap/resources/noun-map-pin-hospital-1661270.png");
   //map pin hospital by b farias from https://thenounproject.com/browse/icons/term/map-pin-hospital/
   g->draw_surface(icon, xy_loc, icon_size);
}

void displayEntertainment(ezgl::renderer *g, ezgl::point2d xy_loc) {
   ezgl::surface *icon = ezgl::renderer::load_png("libstreetmap/resources/noun-cinema-map-pin-1661309.png");
   //cinema map pin by b farias from https://thenounproject.com/browse/icons/term/cinema-map-pin/
   g->draw_surface(icon, xy_loc, icon_size);
}

void displayPublic(ezgl::renderer *g, ezgl::point2d xy_loc) {
   ezgl::surface *icon = ezgl::renderer::load_png("libstreetmap/resources/noun-map-pin-work-1661271.png");
   //map pin work by b farias from https://thenounproject.com/browse/icons/term/map-pin-work/
   g->draw_surface(icon, xy_loc, icon_size);
}

void displayOther(ezgl::renderer *g, ezgl::point2d xy_loc) {
   ezgl::surface *icon = ezgl::renderer::load_png("libstreetmap/resources/noun-map-pin-1661274.png");
   //map pin by b farias from https://thenounproject.com/browse/icons/term/map-pin/
   g->draw_surface(icon, xy_loc, icon_size*2/3); // other should be smaller to slightly reduce clutter
}
*/

static size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp) {
    if (buffer && nmemb && userp) {
        MyCustomStruct *pMyStruct = (MyCustomStruct *)userp;
        // Writes to struct passed in from main
        if (pMyStruct->response == nullptr) {
            // Case when first time write_data() is invoked
            pMyStruct->response = new char[nmemb + 1];
            strncpy(pMyStruct->response, (char *)buffer, nmemb);
        }
        else {
            // Case when second or subsequent time write_data() is invoked
            char *oldResp = pMyStruct->response;

            pMyStruct->response = new char[pMyStruct->size + nmemb + 1];

            // Copy old data
            strncpy(pMyStruct->response, oldResp, pMyStruct->size);

            // Append new data
            strncpy(pMyStruct->response + pMyStruct->size, (char *)buffer, nmemb);

            delete []oldResp;
        }
        pMyStruct->size += nmemb;
        pMyStruct->response[pMyStruct->size] = '\0';
    }
    return nmemb;
}

void compile_closure_info(ptree &ptRoot) {
   std::string road_name;
   std::string road_id;
   std::string description;
   
   BOOST_FOREACH(ptree::value_type &featVal, ptRoot.get_child("Closure")) {
      closure_data cur_closure;
      
      // "features" maps to a JSON array, so each child should have no name
      if ( !featVal.first.empty() )
         throw "\"features\" child node has a name";
      
      cur_closure.name = featVal.second.get<string>("name");
      cur_closure.id = featVal.second.get<string>("id");
      cur_closure.description = featVal.second.get<string>("description");

      LatLon loc(featVal.second.get<double>("latitude"), featVal.second.get<double>("longitude"));
      ezgl::point2d location(x_from_lon(loc.longitude()), y_from_lat(loc.latitude()));
      cur_closure.xy_loc = location;

      closure_information.push_back(cur_closure);
   }

   return;
}

void load_closure(){
   CURLcode res = curl_global_init(CURL_GLOBAL_ALL);
   if (res != CURLE_OK) {
      cout << "ERROR: Unable to initialize libcurl" << endl;
      cout << curl_easy_strerror(res) << endl;
      return;
   }

   CURL *curlHandle = curl_easy_init();
   if ( !curlHandle ) {
      cout << "ERROR: Unable to get easy handle" << endl;
      return;
   } else {
      char errbuf[CURL_ERROR_SIZE] = {0};
      MyCustomStruct myStruct;
      char targetURL[] = "https://secure.toronto.ca/opendata/cart/road_restrictions/v3?format=json";

      res = curl_easy_setopt(curlHandle, CURLOPT_URL, targetURL);
      if (res == CURLE_OK)
         res = curl_easy_setopt(curlHandle, CURLOPT_ERRORBUFFER, errbuf);
      if (res == CURLE_OK)
         res = curl_easy_setopt(curlHandle, CURLOPT_WRITEFUNCTION, write_data);
      if (res == CURLE_OK)
         res = curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, &myStruct);
      myStruct.url = targetURL;
      if (res != CURLE_OK) {
         cout << "ERROR: Unable to set libcurl option" << endl;
         cout << curl_easy_strerror(res) << endl;
      } else {
         res = curl_easy_perform(curlHandle);
      }
      //cout << endl << endl;
      if (res == CURLE_OK) {
         // Create an empty proper tree
         ptree ptRoot;
         /* Store JSON data into a Property Tree
         *
         * read_json() expects the first parameter to be an istream object,
         * or derived from istream (e.g. ifstream, istringstream, etc.).
         * The second parameter is an empty property tree.
         *
         * If your JSON data is in C-string or C++ string object, you can
         * pass it to the constructor of an istringstream object.
         */
         istringstream issJsonData(myStruct.response);
         read_json(issJsonData, ptRoot);
         // Parsing and printing the data
         //            cout << "Current road closures are as follows:" << endl;
         //            cout << "====================" << endl << endl;
         try {
            compile_closure_info(ptRoot);
         } catch (const char *errMsg) {
         //                cout << "ERROR: Unable to fully parse the road closures JSON data" << endl;
         //                cout << "Thrown message: " << errMsg << endl;
         }
         //            cout << endl << "====================" << endl;
         //            cout << "Done!" << endl;
      } 
      //            cout << "ERROR: res == " << res << endl;
      //            cout << errbuf << endl;

      if (myStruct.response)
      delete []myStruct.response;

      curl_easy_cleanup(curlHandle);
      curlHandle = nullptr;
      }
   curl_global_cleanup();
   return;
}

void display_closure(ezgl::renderer* g) {
   ezgl::surface *icon = ezgl::renderer::load_png("libstreetmap/resources/noun-closed-315800.png");
   // closed by Jonathan Li from https://thenounproject.com/browse/icons/term/closed/

   for (int cur_closure = 0; cur_closure < closure_information.size(); cur_closure++){
      ezgl::point2d xy_loc = closure_information[cur_closure].xy_loc;
      if (xy_loc.x > g->get_visible_world().right() || xy_loc.x < g->get_visible_world().left() ||
          xy_loc.y > g->get_visible_world().top() || xy_loc.y < g->get_visible_world().bottom() ||
          zoomFactor >= 0.022) {
         continue;
      }
      g->draw_surface(icon, xy_loc, icon_size*0.05);
   }
   return;
}
