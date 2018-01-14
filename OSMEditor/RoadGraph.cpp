#include "RoadGraph.h"

float RoadGraph::EPS = 1e-6f;

RoadGraph::RoadGraph() {
	showHighways = true;
	showBoulevard = true;
	showAvenues = true;
	showLocalStreets = true;
}

RoadGraph::~RoadGraph() {
}

void RoadGraph::clear() {
	graph.clear();
}

/**
* Return the degree of the specified vertex.
*/
int RoadGraph::getDegree(RoadVertexDesc v) {
	int count = 0;
	RoadOutEdgeIter ei, eend;
	for (boost::tie(ei, eend) = boost::out_edges(v, graph); ei != eend; ++ei) {
		if (graph[*ei]->valid) count++;
	}
	return count;
}

/**
* Remove the vertices of degree of 2, and make it as a part of an edge.
*/
void RoadGraph::reduce() {
	RoadVertexIter vi, vend;
	bool deleted = false;
	do {
		deleted = false;

		for (boost::tie(vi, vend) = boost::vertices(graph); vi != vend; ++vi) {
			if (!graph[*vi]->valid) continue;

			RoadVertexPtr v = graph[*vi];

			if (getDegree(*vi) == 2) {
				if (reduce(*vi)) {
					deleted = true;
					break;
				}
			}
		}
	} while (deleted);
}

/**
* Remove the vertex of degree 2, and make it as a part of an edge.
*/
bool RoadGraph::reduce(RoadVertexDesc desc) {
	int count = 0;
	RoadVertexDesc vd[2];
	RoadEdgeDesc ed[2];
	RoadEdgePtr edges[2];

	RoadOutEdgeIter ei, ei_end;
	for (boost::tie(ei, ei_end) = out_edges(desc, graph); ei != ei_end; ++ei) {
		if (!graph[*ei]->valid) continue;

		vd[count] = boost::target(*ei, graph);
		ed[count] = *ei;
		edges[count] = graph[*ei];
		count++;
	}

	// Don't remove the vertex if it is a loop road.
	if (vd[0] == vd[1]) return false;

	if (edges[0]->type != edges[1]->type) return false;
	//if (edges[0]->lanes != edges[1]->lanes) return false;

	// If the vertices form a triangle, don't remove it.
	//if (hasEdge(roads, vd[0], vd[1])) return false;

	RoadEdgePtr new_edge = RoadEdgePtr(new RoadEdge(edges[0]->type, edges[0]->lanes, edges[0]->oneWay));
	orderPolyLine(ed[0], vd[0]);
	orderPolyLine(ed[1], desc);

	for (int i = 0; i < edges[0]->polyline.size(); i++) {
		new_edge->addPoint(edges[0]->polyline[i]);
	}
	for (int i = 1; i < edges[1]->polyline.size(); i++) {
		new_edge->addPoint(edges[1]->polyline[i]);
	}
	std::pair<RoadEdgeDesc, bool> edge_pair = boost::add_edge(vd[0], vd[1], graph);
	graph[edge_pair.first] = new_edge;

	// invalidate the old edge
	graph[ed[0]]->valid = false;
	graph[ed[1]]->valid = false;

	// invalidate the vertex
	graph[desc]->valid = false;

	return true;
}

/**
* Move the vertex to the specified location.
* The outing edges are also moved accordingly.
*/
void RoadGraph::moveVertex(RoadVertexDesc v, const QVector2D& pt) {
	// Move the outing edges
	RoadOutEdgeIter ei, eend;
	for (boost::tie(ei, eend) = boost::out_edges(v, graph); ei != eend; ++ei) {
		RoadVertexDesc tgt = boost::target(*ei, graph);

		std::vector<QVector2D>& polyline = graph[*ei]->polyline;
		if ((polyline[0] - graph[v]->pt).lengthSquared() < (polyline[0] - graph[tgt]->pt).lengthSquared()) {
			std::reverse(polyline.begin(), polyline.end());
		}

		movePolyline(polyline, graph[tgt]->pt, pt);

		graph[*ei]->polyline = polyline;
	}

	// Move the vertex
	graph[v]->pt = pt;
}

/**
* Polylineを指定した始点、終点になるよう変形する。
*/
void RoadGraph::movePolyline(std::vector<QVector2D>& polyline, const QVector2D& src_pos, const QVector2D& tgt_pos) {
	/*
	float scale = (tgt_pos - src_pos).length() / (polyline.back() - polyline[0]).length();
	float rotation_degree = Util::rad2deg(Util::diffAngle(polyline.back() - polyline[0], tgt_pos - src_pos, false));

	polyline.scale(scale);
	polyline.rotate(rotation_degree, QVector2D(0, 0));
	polyline.translate(src_pos - polyline[0]);
	*/
	polyline[0] = src_pos;
	polyline.back() = tgt_pos;
}

/**
* Check if there is an edge between two vertices.
*/
bool RoadGraph::hasEdge(RoadVertexDesc desc1, RoadVertexDesc desc2) {
	RoadOutEdgeIter ei, eend;
	for (boost::tie(ei, eend) = boost::out_edges(desc1, graph); ei != eend; ++ei) {
		if (!graph[*ei]->valid) continue;

		RoadVertexDesc tgt = boost::target(*ei, graph);
		if (tgt == desc2) return true;
	}

	for (boost::tie(ei, eend) = boost::out_edges(desc2, graph); ei != eend; ++ei) {
		if (!graph[*ei]->valid) continue;

		RoadVertexDesc tgt = boost::target(*ei, graph);
		if (tgt == desc1) return true;
	}

	return false;
}

/**
* Return the edge between src and tgt.
*/
RoadEdgeDesc RoadGraph::getEdge(RoadVertexDesc src, RoadVertexDesc tgt) {
	RoadOutEdgeIter ei, eend;
	for (boost::tie(ei, eend) = boost::out_edges(src, graph); ei != eend; ++ei) {
		if (!graph[*ei]->valid) continue;

		if (boost::target(*ei, graph) == tgt) return *ei;
	}

	for (boost::tie(ei, eend) = boost::out_edges(tgt, graph); ei != eend; ++ei) {
		if (!graph[*ei]->valid) continue;

		if (boost::target(*ei, graph) == src) return *ei;
	}

	throw "No edge found.";
}

/**
* Snap v1 to v2.
*/
void RoadGraph::snapVertex(RoadVertexDesc v1, RoadVertexDesc v2) {
	if (v1 == v2) return;

	moveVertex(v1, graph[v2]->pt);

	if (hasEdge(v1, v2)) {
		RoadEdgeDesc e = getEdge(v1, v2);

		// if the edge is too short, remove it. (This might be contraversial...)
		if (graph[e]->getLength() < 1.0f) {
			graph[e]->valid = false;
		}
	}

	// Snap all the outing edges from v1
	RoadOutEdgeIter ei, eend;
	for (boost::tie(ei, eend) = boost::out_edges(v1, graph); ei != eend; ++ei) {
		if (!graph[*ei]->valid) continue;

		RoadVertexDesc v1b = boost::target(*ei, graph);

		// invalidate the old edge
		graph[*ei]->valid = false;

		if (v1b == v2) continue;
		if (hasEdge(v2, v1b)) continue;
		//if (hasCloseEdge(roads, v2, v1b)) continue;	// <-- In this case, snap the edge to the other instead of discard it.

		// add a new edge
		RoadEdgePtr e = RoadEdgePtr(new RoadEdge(*graph[*ei]));
		e->valid = true;
		std::pair<RoadEdgeDesc, bool> edge_pair = boost::add_edge(v2, v1b, graph);
		graph[edge_pair.first] = e;
	}

	// invalidate v1
	graph[v1]->valid = false;
}

/**
* Split the edge at the specified point.
*/
RoadVertexDesc RoadGraph::splitEdge(RoadEdgeDesc edge_desc, const QVector2D& pt) {
	RoadEdgePtr edge = graph[edge_desc];

	// find which point along the polyline is the closest to the specified split point.
	int index;
	QVector2D pos;
	float min_dist = std::numeric_limits<float>::max();
	for (int i = 0; i < graph[edge_desc]->polyline.size() - 1; i++) {
		QVector2D vec = graph[edge_desc]->polyline[i + 1] - graph[edge_desc]->polyline[i];
		float length = vec.length();
		for (int j = 0; j < length; j += 1.0f) {
			QVector2D pt2 = graph[edge_desc]->polyline[i] + vec * (float)j / length;
			float dist = (pt2 - pt).lengthSquared();
			if (dist < min_dist) {
				min_dist = dist;
				index = i;
				pos = pt2;
			}
		}
	}

	RoadVertexDesc src = boost::source(edge_desc, graph);
	RoadVertexDesc tgt = boost::target(edge_desc, graph);

	// add a new vertex at the specified point on the edge
	RoadVertexPtr v = RoadVertexPtr(new RoadVertex(pos));
	RoadVertexDesc v_desc = boost::add_vertex(graph);
	graph[v_desc] = v;

	// add the first edge
	RoadEdgePtr e1 = RoadEdgePtr(new RoadEdge(edge->type, edge->lanes, edge->oneWay));
	if ((edge->polyline[0] - graph[src]->pt).lengthSquared() < (edge->polyline[0] - graph[tgt]->pt).lengthSquared()) {
		for (int i = 0; i <= index; i++) {
			e1->addPoint(edge->polyline[i]);
		}
		e1->addPoint(pos);
	}
	else {
		e1->addPoint(pos);
		for (int i = index + 1; i < edge->polyline.size(); i++) {
			e1->addPoint(edge->polyline[i]);
		}
	}
	std::pair<RoadEdgeDesc, bool> edge_pair1 = boost::add_edge(src, v_desc, graph);
	graph[edge_pair1.first] = e1;

	// add the second edge
	RoadEdgePtr e2 = RoadEdgePtr(new RoadEdge(edge->type, edge->lanes, edge->oneWay));
	if ((edge->polyline[0] - graph[src]->pt).lengthSquared() < (edge->polyline[0] - graph[tgt]->pt).lengthSquared()) {
		e2->addPoint(pos);
		for (int i = index + 1; i < edge->polyline.size(); i++) {
			e2->addPoint(edge->polyline[i]);
		}
	}
	else {
		for (int i = 0; i <= index; i++) {
			e2->addPoint(edge->polyline[i]);
		}
		e2->addPoint(pos);
	}
	std::pair<RoadEdgeDesc, bool> edge_pair2 = boost::add_edge(v_desc, tgt, graph);
	graph[edge_pair2.first] = e2;

	// remove the original edge
	graph[edge_desc]->valid = false;

	return v_desc;
}

/**
* Convert the road graph to a planar graph.
*/
void RoadGraph::planarify() {
	bool split = true;

	while (split) {
		split = planarifyOne();
	}
}

/**
* Convert one intersected road segments to a planar one by adding the intersection, and return true.
* If the road segments do not intersect, return false.
*/
bool RoadGraph::planarifyOne() {
	RoadEdgeIter ei, eend;
	for (boost::tie(ei, eend) = boost::edges(graph); ei != eend; ++ei) {
		RoadEdgePtr e = graph[*ei];
		if (!e->valid) continue;

		RoadVertexDesc src = boost::source(*ei, graph);
		RoadVertexDesc tgt = boost::target(*ei, graph);

		RoadEdgeIter ei2, eend2;
		for (boost::tie(ei2, eend2) = boost::edges(graph); ei2 != eend2; ++ei2) {
			RoadEdgePtr e2 = graph[*ei2];
			if (!e2->valid) continue;

			RoadVertexDesc src2 = boost::source(*ei2, graph);
			RoadVertexDesc tgt2 = boost::target(*ei2, graph);

			// skip if e and e2 are adjacent
			if (src == src2 || src == tgt2 || tgt == src2 || tgt == tgt2) continue;

			for (int i = 0; i < e->polyline.size() - 1; i++) {
				for (int j = 0; j < e2->polyline.size() - 1; j++) {
					float tab, tcd;
					QVector2D intPt;
					if (segmentSegmentIntersect(e->polyline[i], e->polyline[i + 1], e2->polyline[j], e2->polyline[j + 1], &tab, &tcd, intPt)) {
						// check if the intersection is too close to an end point
						//if ((roads.graph[src]->pt - intPt).length() < 0.0001f || (roads.graph[tgt]->pt - intPt).length() < 0.0001f || (roads.graph[src2]->pt - intPt).length() < 0.0001f || (roads.graph[tgt2]->pt - intPt).length() < 0.0001f) continue;

						// split the road segments
						RoadVertexDesc new_v_desc = splitEdge(*ei, intPt);
						RoadVertexDesc new_v_desc2 = splitEdge(*ei2, intPt);

						// invalidate the original road segments
						graph[*ei]->valid = false;
						graph[*ei2]->valid = false;

						// snap the intersection
						snapVertex(new_v_desc2, new_v_desc);

						return true;
					}
				}
			}
		}
	}

	return false;
}

/**
* Sort the points of the polyline of the edge such that the first point is the location of the src vertex.
*/
void RoadGraph::orderPolyLine(RoadEdgeDesc e, RoadVertexDesc src) {
	RoadVertexDesc tgt;

	RoadVertexDesc s = boost::source(e, graph);
	RoadVertexDesc t = boost::target(e, graph);

	if (s == src) {
		tgt = t;
	}
	else {
		tgt = s;
	}

	// If the order is opposite, reverse the order.
	if ((graph[src]->pt - graph[e]->polyline[0]).lengthSquared() > (graph[tgt]->pt - graph[e]->polyline[0]).lengthSquared()) {
		std::reverse(graph[e]->polyline.begin(), graph[e]->polyline.end());
	}
}

/**
* Return the sistance from segment ab to point c.
* If the
*/
float RoadGraph::pointSegmentDistance(const QVector2D &a, const QVector2D &b, const QVector2D &c, bool segmentOnly) {
	float r_numerator = (c.x() - a.x())*(b.x() - a.x()) + (c.y() - a.y())*(b.y() - a.y());
	float r_denomenator = (b.x() - a.x())*(b.x() - a.x()) + (b.y() - a.y())*(b.y() - a.y());
	float r = r_numerator / r_denomenator;

	if (segmentOnly && (r < 0 || r > 1)) {
		float dist1 = std::hypot(c.x() - a.x(), c.y() - a.y());
		float dist2 = std::hypot(c.x() - b.x(), c.y() - b.y());
		if (dist1 < dist2) {
			return dist1;
		}
		else {
			return dist2;
		}
	}
	else {
		return abs((a.y() - c.y())*(b.x() - a.x()) - (a.x() - c.x())*(b.y() - a.y())) / sqrt(r_denomenator);
	}
}

/**
* Computes the intersection between two line segments on the XY plane.
* Segments must intersect within their extents for the intersection to be valid. z coordinate is ignored.
*
* @param a one end of the first line
* @param b another end of the first line
* @param c one end of the second line
* @param d another end of the second line
* @param tab
* @param tcd
* @param segmentOnly
* @param intPoint	the intersection
* @return true if two lines intersect / false otherwise
**/
bool RoadGraph::segmentSegmentIntersect(const QVector2D& a, const QVector2D& b, const QVector2D& c, const QVector2D& d, float *tab, float *tcd, QVector2D& intPoint) {
	QVector2D u = b - a;
	QVector2D v = d - c;

	if (u.lengthSquared() < EPS || v.lengthSquared() < EPS) {
		return false;
	}

	float numer = v.x()*(c.y() - a.y()) + v.y()*(a.x() - c.x());
	float denom = u.y()*v.x() - u.x()*v.y();

	if (denom == 0.0f)  {
		// they are parallel
		*tab = 0.0f;
		*tcd = 0.0f;
		return false;
	}

	float t0 = numer / denom;

	QVector2D ipt = a + t0*u;
	QVector2D tmp = ipt - c;
	float t1;
	if (QVector2D::dotProduct(tmp, v) > 0.0f){
		t1 = tmp.length() / v.length();
	}
	else {
		t1 = -1.0f * tmp.length() / v.length();
	}

	//Check if intersection is within segments
	if (!((t0 >= EPS) && (t0 <= 1.0f - EPS) && (t1 >= EPS) && (t1 <= 1.0f - EPS))){
		return false;
	}

	*tab = t0;
	*tcd = t1;
	QVector2D dirVec = b - a;

	intPoint = a + (*tab)*dirVec;

	return true;
}
