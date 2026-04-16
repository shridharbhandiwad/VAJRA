#pragma once
#include <QString>

struct AntennaConfig {
    QString name;
    int     quadrants            = 4;
    int     elementsPerQuadrant  = 256;
    int     elementsPerCluster   = 16;
    int     udpPort              = 5005;

    int clustersPerQuadrant() const { return elementsPerQuadrant / elementsPerCluster; }
    int totalElements()       const { return quadrants * elementsPerQuadrant; }
};
