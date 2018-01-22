#include "Canvas.h"
#include <QPainter>
#include <iostream>
#include <QFileInfoList>
#include <QDir>
#include <QMessageBox>
#include <QTextStream>
#include <QDomDocument>
#include <QResizeEvent>
#include <QtWidgets/QApplication>
#include <QDate>
#include <glm/gtx/string_cast.hpp>
#include "MainWindow.h"
#include "OSMRoadsParser.h"
#include "OSMRoadsExporter.h"

Canvas::Canvas() {
	this->mainWin = mainWin;
	ctrlPressed = false;
	shiftPressed = false;
	keyAPressed = false;

	origin = QPoint(0, 0);// width() * 0.5, height() * 0.5);
	scale = 0.1;

	vertex_selected = false;
	edge_selected = false;
	edge_point_selected = false;
	vertex_moved = false;
	adding_new_edge = false;
}

Canvas::~Canvas() {
}

void Canvas::clear() {
	update();
}

void Canvas::open(const QString& filename) {
	roads.clear();
	vertex_selected = false;
	edge_selected = false;
	edge_point_selected = false;

	OSMRoadsParser parser(&roads);
	QXmlSimpleReader reader;
	reader.setContentHandler(&parser);
	QFile file(filename);
	QXmlInputSource source(&file);
	reader.parse(source);

	roads.reduce();
	roads.planarify();

	update();
}

void Canvas::save(const QString& filename) {
	OSMRoadsExporter::save(filename, roads);
}

void Canvas::undo() {
	try {
		roads = history.undo();
	}
	catch (char* ex) {
	}

	// clear the selection
	vertex_selected = false;
	edge_selected = false;
	edge_point_selected = false;

	update();
}

void Canvas::deleteEdge() {
	if (edge_selected) {
		history.push(roads);
		roads.deleteEdge(selected_edge_desc);
		edge_selected = false;
		update();
	}
}

/**
 * Find the closest vertex.
 *
 * @param x		x coordinate in the world coordinate system
 * @param y		y coordinate in the world coordinate system
 */
bool Canvas::findClosestVertex(const QVector2D& pt, float threshold, RoadVertexDesc& closest_vertex_desc) {
	float min_dist = std::numeric_limits<float>::max();

	QVector2D screen_pt = worldToScreenCoordinates(pt);

	RoadVertexIter vi, vend;
	for (boost::tie(vi, vend) = boost::vertices(roads.graph); vi != vend; vi++) {
		if (!roads.graph[*vi]->valid) continue;

		QVector2D vert_pt = worldToScreenCoordinates(roads.graph[*vi]->pt);
		float dist = std::hypot(vert_pt.x() - screen_pt.x(), vert_pt.y() - screen_pt.y());
		if (dist < min_dist) {
			min_dist = dist;
			closest_vertex_desc = *vi;
		}
	}

	if (min_dist < threshold) {
		return true;
	}
	else {
		return false;
	}
}

/**
* Find the closest vertex.
*
* @param x		x coordinate in the world coordinate system
* @param y		y coordinate in the world coordinate system
*/
bool Canvas::findClosestVertexExcept(const QVector2D& pt, float threshold, RoadVertexDesc except_vertex, RoadVertexDesc& closest_vertex_desc) {
	float min_dist = std::numeric_limits<float>::max();

	QVector2D screen_pt = worldToScreenCoordinates(pt);

	RoadVertexIter vi, vend;
	for (boost::tie(vi, vend) = boost::vertices(roads.graph); vi != vend; vi++) {
		if (!roads.graph[*vi]->valid || *vi == except_vertex) continue;

		QVector2D vert_pt = worldToScreenCoordinates(roads.graph[*vi]->pt);
		float dist = std::hypot(vert_pt.x() - screen_pt.x(), vert_pt.y() - screen_pt.y());
		if (dist < min_dist) {
			min_dist = dist;
			closest_vertex_desc = *vi;
		}
	}

	if (min_dist < threshold) {
		return true;
	}
	else {
		return false;
	}
}

bool Canvas::findClosestEdgePoint(const QVector2D& pt, float threshold, RoadEdgeDesc& closest_edge_desc, int& closest_edge_point) {
	float min_dist = std::numeric_limits<float>::max();

	QVector2D screen_pt = worldToScreenCoordinates(pt);

	RoadEdgeIter ei, eend;
	for (boost::tie(ei, eend) = boost::edges(roads.graph); ei != eend; ei++) {
		if (!roads.graph[*ei]->valid) continue;

		for (int i = 0; i < roads.graph[*ei]->polyline.size() - 1; i++) {
			QVector2D p = worldToScreenCoordinates(roads.graph[*ei]->polyline[i]);
			double dist = (p - screen_pt).length();
			if (dist < min_dist) {
				min_dist = dist;
				closest_edge_desc = *ei;
				closest_edge_point = i;
			}
		}
	}

	if (min_dist < threshold) {
		return true;
	}
	else {
		return false;
	}

}

/**
* Find the closest edge.
*
* @param pt		coordinates in the world coordinate system
*/
bool Canvas::findClosestEdge(const QVector2D& pt, float threshold, RoadEdgeDesc& closest_edge_desc) {
	float min_dist = std::numeric_limits<float>::max();

	QVector2D screen_pt = worldToScreenCoordinates(pt);

	RoadEdgeIter ei, eend;
	for (boost::tie(ei, eend) = boost::edges(roads.graph); ei != eend; ei++) {
		if (!roads.graph[*ei]->valid) continue;

		for (int i = 0; i < roads.graph[*ei]->polyline.size() - 1; i++) {
			QVector2D p0 = worldToScreenCoordinates(roads.graph[*ei]->polyline[i]);
			QVector2D p1 = worldToScreenCoordinates(roads.graph[*ei]->polyline[i + 1]);
			double dist = RoadGraph::pointSegmentDistance(p0, p1, screen_pt);
			if (dist < min_dist) {
				min_dist = dist;
				closest_edge_desc = *ei;
			}
		}
	}

	if (min_dist < threshold) {
		return true;
	}
	else {
		return false;
	}
}

/**
* Find the closest edge.
*
* @param pt				coordinates of the given point in the world coordinate system
* @param closest_pt		coordinates of the closest point in the world coordinate system
*/
bool Canvas::findClosestEdge(const QVector2D& pt, float threshold, RoadEdgeDesc& closest_edge_desc, QVector2D& closest_pt) {
	float min_dist = std::numeric_limits<float>::max();

	QVector2D screen_pt = worldToScreenCoordinates(pt);

	RoadEdgeIter ei, eend;
	for (boost::tie(ei, eend) = boost::edges(roads.graph); ei != eend; ei++) {
		if (!roads.graph[*ei]->valid) continue;

		for (int i = 0; i < roads.graph[*ei]->polyline.size() - 1; i++) {
			QVector2D p0 = worldToScreenCoordinates(roads.graph[*ei]->polyline[i]);
			QVector2D p1 = worldToScreenCoordinates(roads.graph[*ei]->polyline[i + 1]);
			QVector2D p2;
			double dist = RoadGraph::pointSegmentDistance(p0, p1, screen_pt, p2);
			if (dist < min_dist) {
				min_dist = dist;
				closest_edge_desc = *ei;
				closest_pt = screenToWorldCoordinates(p2);
			}
		}
	}

	if (min_dist < threshold) {
		return true;
	}
	else {
		return false;
	}
}

bool Canvas::findClosestEdgeExcept(const QVector2D& pt, float threshold, RoadVertexDesc except_vertex, RoadEdgeDesc& closest_edge_desc, QVector2D& closest_pt) {
	float min_dist = std::numeric_limits<float>::max();

	QVector2D screen_pt = worldToScreenCoordinates(pt);

	RoadEdgeIter ei, eend;
	for (boost::tie(ei, eend) = boost::edges(roads.graph); ei != eend; ei++) {
		if (!roads.graph[*ei]->valid) continue;

		RoadVertexDesc src = boost::source(*ei, roads.graph);
		RoadVertexDesc tgt = boost::target(*ei, roads.graph);
		if (except_vertex == src || except_vertex == tgt) continue;

		for (int i = 0; i < roads.graph[*ei]->polyline.size() - 1; i++) {
			QVector2D p0 = worldToScreenCoordinates(roads.graph[*ei]->polyline[i]);
			QVector2D p1 = worldToScreenCoordinates(roads.graph[*ei]->polyline[i + 1]);
			QVector2D p2;
			double dist = RoadGraph::pointSegmentDistance(p0, p1, screen_pt, p2);
			if (dist < min_dist) {
				min_dist = dist;
				closest_edge_desc = *ei;
				closest_pt = screenToWorldCoordinates(p2);
			}
		}
	}

	if (min_dist < threshold) {
		return true;
	}
	else {
		return false;
	}
}

QVector2D Canvas::screenToWorldCoordinates(const QVector2D& p) {
	return screenToWorldCoordinates(p.x(), p.y());
}

QVector2D Canvas::screenToWorldCoordinates(double x, double y) {
	return QVector2D((x - origin.x()) / scale, -(y - origin.y()) / scale);
}

QVector2D Canvas::worldToScreenCoordinates(const QVector2D& p) {
	return QVector2D(origin.x() + p.x() * scale, origin.y() - p.y() * scale);
}

void Canvas::paintEvent(QPaintEvent *e) {
	QPainter painter(this);
	painter.fillRect(0, 0, width(), height(), QColor(255, 255, 255));

	// draw road edges
	painter.setPen(QPen(QColor(192, 192, 255), 5));
	RoadEdgeIter ei, eend;
	for (boost::tie(ei, eend) = boost::edges(roads.graph); ei != eend; ei++) {
		if (!roads.graph[*ei]->valid) continue;

		QPolygonF polygon;
		for (int i = 0; i < roads.graph[*ei]->polyline.size(); i++) {
			QVector2D pt = worldToScreenCoordinates(roads.graph[*ei]->polyline[i]);
			polygon.push_back(QPointF(pt.x(), pt.y()));
		}
		painter.drawPolyline(polygon);

		for (int i = 0; i < roads.graph[*ei]->polyline.size(); i++) {
			QVector2D pt = worldToScreenCoordinates(roads.graph[*ei]->polyline[i]);
			painter.drawEllipse(pt.x() - 3, pt.y() - 3, 7, 7);
		}
	}

	// draw selected edge
	if (edge_selected) {
		painter.setPen(QPen(QColor(0, 0, 255), 5));

		QPolygonF polygon;
		for (int i = 0; i < roads.graph[selected_edge_desc]->polyline.size(); i++) {
			QVector2D pt = worldToScreenCoordinates(roads.graph[selected_edge_desc]->polyline[i]);
			polygon.push_back(QPointF(pt.x(), pt.y()));
		}
		painter.drawPolyline(polygon);

		for (int i = 0; i < roads.graph[selected_edge_desc]->polyline.size(); i++) {
			QVector2D pt = worldToScreenCoordinates(roads.graph[selected_edge_desc]->polyline[i]);
			painter.drawEllipse(pt.x() - 3, pt.y() - 3, 7, 7);
		}
	}

	if (edge_point_selected) {
		painter.setPen(QPen(QColor(0, 0, 0), 3));
		painter.setBrush(QBrush(QColor(0, 0, 0)));

		QVector2D pt = worldToScreenCoordinates(roads.graph[selected_edge_desc]->polyline[selected_edge_point]);
		painter.drawEllipse(pt.x() - 3, pt.y() - 3, 7, 7);
	}

	// draw road vertices
	painter.setPen(QPen(QColor(192, 192, 192), 3));
	painter.setBrush(QBrush(QColor(255, 255, 255)));
	RoadVertexIter vi, vend;
	for (boost::tie(vi, vend) = boost::vertices(roads.graph); vi != vend; vi++) {
		if (!roads.graph[*vi]->valid) continue;

		QVector2D pt = worldToScreenCoordinates(roads.graph[*vi]->pt);
		painter.drawEllipse(pt.x() - 6, pt.y() - 6, 13, 13);
	}

	// draw selected vertex
	if (vertex_selected) {
		painter.setPen(QPen(QColor(0, 0, 0), 3));
		painter.setBrush(QBrush(QColor(255, 255, 255)));

		QVector2D pt = worldToScreenCoordinates(roads.graph[selected_vertex_desc]->pt);
		painter.drawEllipse(pt.x() - 6, pt.y() - 6, 13, 13);
	}

	// draw the adding edge
	if (adding_new_edge) {
		painter.setPen(QPen(QColor(0, 0, 255), 3));
		QPolygonF polygon;
		for (int i = 0; i < new_edge.size(); i++) {
			QVector2D pt = worldToScreenCoordinates(new_edge[i]);
			polygon.push_back(QPointF(pt.x(), pt.y()));
		}
		polygon.push_back(QPointF(prev_mouse_pt.x(), prev_mouse_pt.y()));
		painter.drawPolyline(polygon);
	}
	
	// draw axes
	painter.setPen(QPen(QColor(128, 128, 128), 1));
	painter.drawLine(-10000 * scale + origin.x(), origin.y(), 10000 * scale + origin.x(), origin.y());
	painter.drawLine(origin.x(), -10000 * scale + origin.y(), origin.x(), 10000 * scale + origin.y());
}

void Canvas::mousePressEvent(QMouseEvent* e) {
	// This is necessary to get key event occured even after the user selects a menu.
	setFocus();

	vertex_moved = false;
	vertex_selected = false;
	edge_selected = false;
	edge_point_selected = false;

	if (e->buttons() & Qt::LeftButton) {
		QVector2D pt = screenToWorldCoordinates(e->x(), e->y());

		if (shiftPressed) {
			// add a vertex on an edge
			RoadEdgeDesc e_desc;
			
			if (findClosestEdge(pt, 9, e_desc)) {
				history.push(roads);
				roads.splitEdge(e_desc, pt);
			}
		}
		else if (keyAPressed) {
			// add a vertex
			if (!adding_new_edge) {
				adding_new_edge = true;
				new_edge.clear();
				setMouseTracking(true);
			}
			new_edge.push_back(pt);
		}
		else {
			// hit test against the vertices
			if (findClosestVertex(pt, 10, selected_vertex_desc)) {
				vertex_selected = true;
				history.push(roads);
			}
			else if (findClosestEdgePoint(pt, 9, selected_edge_desc, selected_edge_point)) {
				edge_point_selected = true;
			}
			// hit test against the edges
			else if (findClosestEdge(pt, 9, selected_edge_desc)) {
				edge_selected = true;
			}
		}
	}

	prev_mouse_pt = e->pos();
	update();
}

void Canvas::mouseMoveEvent(QMouseEvent* e) {
	if (e->buttons() & Qt::RightButton) {
		// move the camera
		origin += e->pos() - prev_mouse_pt;
	}
	else if (e->buttons() & Qt::LeftButton) {
		if (vertex_selected) {
			QVector2D pt = screenToWorldCoordinates(e->x(), e->y());

			if (ctrlPressed) {
				// try to snap the currently selected vertex to the closest one
				RoadVertexDesc target_vertex_desc;
				RoadEdgeDesc target_edge_desc;
				QVector2D closest_pt;
				if (findClosestVertexExcept(pt, 30, selected_vertex_desc, target_vertex_desc)) {
					roads.moveVertex(selected_vertex_desc, roads.graph[target_vertex_desc]->pt);
				}
				else if (findClosestEdgeExcept(pt, 30, selected_vertex_desc, target_edge_desc, closest_pt)) {
					roads.moveVertex(selected_vertex_desc, closest_pt);
				}
				else {
					// move the currently selected vertex
					roads.moveVertex(selected_vertex_desc, pt);
				}
			}
			else {
				// move the currently selected vertex
				roads.moveVertex(selected_vertex_desc, pt);
			}

			vertex_moved = true;
		}
	}
	
	prev_mouse_pt = e->pos();
	update();
}

void Canvas::mouseReleaseEvent(QMouseEvent* e) {
	if (vertex_selected && !vertex_moved) {
		// if the currently selected vertex was not moved at all, cancel backuping the current state of roads
		history.undo();
	}
	else if (ctrlPressed && vertex_selected) {
		QVector2D pt = screenToWorldCoordinates(e->x(), e->y());

		// merge the snapped vertex to the closest one
		RoadVertexDesc target_vertex_desc;
		RoadEdgeDesc target_edge_desc;
		QVector2D closest_pt;
		if (findClosestVertexExcept(pt, 30, selected_vertex_desc, target_vertex_desc)) {
			roads.snapVertex(selected_vertex_desc, target_vertex_desc);
		}
		else if (findClosestEdgeExcept(pt, 30, selected_vertex_desc, target_edge_desc, closest_pt)) {
			target_vertex_desc = roads.splitEdge(target_edge_desc, closest_pt);
			roads.snapVertex(selected_vertex_desc, target_vertex_desc);
		}

		update();
	}
}

void Canvas::mouseDoubleClickEvent(QMouseEvent* e) {
	if (adding_new_edge) {
		setMouseTracking(false);

		if (new_edge.size() >= 2) {
			history.push(roads);

			// add the new edge
			RoadVertexDesc src;
			RoadEdgeDesc closest_edge_desc;
			QVector2D closest_pt;
			if (findClosestVertex(new_edge[0], 10, src)) {
				new_edge[0] = roads.graph[src]->pt;
			}
			else if (findClosestEdge(new_edge[0], 10, closest_edge_desc, closest_pt)) {
				src = roads.splitEdge(closest_edge_desc, closest_pt);
				new_edge[0] = roads.graph[src]->pt;
			}
			else {
				RoadVertexPtr v = RoadVertexPtr(new RoadVertex(new_edge[0]));
				src = boost::add_vertex(roads.graph);
				roads.graph[src] = v;
			}
			RoadVertexDesc tgt;
			if (findClosestVertex(new_edge.back(), 10, tgt)) {
				new_edge.back() = roads.graph[tgt]->pt;
			}
			else if (findClosestEdge(new_edge.back(), 10, closest_edge_desc, closest_pt)) {
				tgt = roads.splitEdge(closest_edge_desc, closest_pt);
				new_edge.back() = roads.graph[tgt]->pt;
			}
			else {
				RoadVertexPtr v = RoadVertexPtr(new RoadVertex(new_edge.back()));
				tgt = boost::add_vertex(roads.graph);
				roads.graph[tgt] = v;
			}

			std::pair<RoadEdgeDesc, bool> edge_pair = boost::add_edge(src, tgt, roads.graph);
			roads.graph[edge_pair.first] = RoadEdgePtr(new RoadEdge(RoadEdge::TYPE_STREET, 1));
			roads.graph[edge_pair.first]->polyline = new_edge;
		}

		adding_new_edge = false;
		update();
	}
}

void Canvas::wheelEvent(QWheelEvent* e) {
	double new_scale = scale + e->delta() * 0.001;
	new_scale = std::min(std::max(0.1, new_scale), 1000.0);

	// adjust the origin in order to keep the screen center
	QVector2D p = screenToWorldCoordinates(width() * 0.5, height() * 0.5);
	origin.setX(width() * 0.5 - p.x() * new_scale);
	origin.setY(height() * 0.5 + p.y() * new_scale);
		
	scale = new_scale;

	update();
}

void Canvas::resizeEvent(QResizeEvent *e) {
	// adjust the origin in order to keep the screen center
	QVector2D p = screenToWorldCoordinates(e->oldSize().width() * 0.5, e->oldSize().height() * 0.5);
	origin.setX(e->size().width() * 0.5 - p.x() * scale);
	origin.setY(e->size().height() * 0.5 + p.y() * scale);
}

void Canvas::keyPressEvent(QKeyEvent* e) {
	ctrlPressed = false;
	shiftPressed = false;

	if (e->modifiers() & Qt::ControlModifier) {
		ctrlPressed = true;
	}
	if (e->modifiers() & Qt::ShiftModifier) {
		shiftPressed = true;
	}

	switch (e->key()) {
	case Qt::Key_Escape:
		adding_new_edge = false;
		break;
	case Qt::Key_A:
		keyAPressed = true;
		break;
	}

	update();
}

void Canvas::keyReleaseEvent(QKeyEvent* e) {
	switch (e->key()) {
	case Qt::Key_Control:
		ctrlPressed = false;
		break;
	case Qt::Key_Shift:
		shiftPressed = false;
		break;
	case Qt::Key_A:
		keyAPressed = false;
		break;
	default:
		break;
	}
}

