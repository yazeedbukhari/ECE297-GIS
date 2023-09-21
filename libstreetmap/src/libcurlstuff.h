#ifndef LIBCURL_H
#define LIBCURL_H
#include <iostream>
#include <string.h>
#include <curl/curl.h>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/foreach.hpp>
#include <string>
#include <ctime>
#include <chrono>

using namespace std;
using boost::property_tree::ptree;
using boost::property_tree::read_json;

struct closure_data{
    std::string id;
    std::string name;
    std::string description;
    ezgl::point2d xy_loc;
};

typedef struct MyCustomStruct {
    char *url = nullptr;
    unsigned int size = 0;
    char *response = nullptr;
} MyCustomStruct;

static size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp);
extern std::vector<closure_data> closure_information;

#endif /* LIBCURL_H */