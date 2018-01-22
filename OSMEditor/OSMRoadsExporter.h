#pragma once

#include <QDomDocument>
#include "RoadGraph.h"

class OSMRoadsExporter {
public:
	OSMRoadsExporter() {}

public:
	static void save(const QString& filename, const RoadGraph& roads);
};

