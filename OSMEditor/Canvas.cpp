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

Canvas::Canvas() {
	this->mainWin = mainWin;
	ctrlPressed = false;
	shiftPressed = false;

	origin = QPoint(0, 0);// width() * 0.5, height() * 0.5);
	scale = 0.1;

	vertex_selected = false;
	edge_selected = false;
}

Canvas::~Canvas() {
}

void Canvas::clear() {
	update();
}

void Canvas::open(const QString& filename) {
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
	RoadEdgeIter ei, eend;
	for (boost::tie(ei, eend) = boost::edges(roads.graph); ei != eend; ei++) {
		if (!roads.graph[*ei]->valid) continue;

		QPolygonF polygon;
		for (int i = 0; i < roads.graph[*ei]->polyline.size(); i++) {
			QVector2D pt = worldToScreenCoordinates(roads.graph[*ei]->polyline[i]);
			polygon.push_back(QPointF(pt.x(), pt.y()));
		}

		if (edge_selected && *ei == selected_edge_desc) {
			painter.setPen(QPen(QColor(0, 0, 255), 5));
		}
		else {
			painter.setPen(QPen(QColor(192, 192, 255), 5));
		}
		painter.drawPolyline(polygon);
	}

	// draw road vertices
	RoadVertexIter vi, vend;
	for (boost::tie(vi, vend) = boost::vertices(roads.graph); vi != vend; vi++) {
		if (!roads.graph[*vi]->valid) continue;

		QVector2D pt = worldToScreenCoordinates(roads.graph[*vi]->pt);
		if (vertex_selected && *vi == selected_vertex_desc) {
			painter.setPen(QPen(QColor(0, 0, 0), 3));
			painter.setBrush(QBrush(QColor(255, 255, 255)));
		}
		else {
			painter.setPen(QPen(QColor(192, 192, 192), 3));
			painter.setBrush(QBrush(QColor(255, 255, 255)));
		}
		painter.drawEllipse(pt.x() - 6, pt.y() - 6, 13, 13);
	}

	// draw axes
	painter.setPen(QPen(QColor(128, 128, 128), 1));
	painter.drawLine(-10000 * scale + origin.x(), origin.y(), 10000 * scale + origin.x(), origin.y());
	painter.drawLine(origin.x(), -10000 * scale + origin.y(), origin.x(), 10000 * scale + origin.y());
	painter.restore();
}

void Canvas::mousePressEvent(QMouseEvent* e) {
	// This is necessary to get key event occured even after the user selects a menu.
	setFocus();

	if (e->buttons() & Qt::LeftButton) {
		vertex_selected = false;
		edge_selected = false;

		float min_dist = 9;

		// hit test against the vertices
		RoadVertexIter vi, vend;
		for (boost::tie(vi, vend) = boost::vertices(roads.graph); vi != vend; vi++) {
			if (!roads.graph[*vi]->valid) continue;

			QVector2D pt = worldToScreenCoordinates(roads.graph[*vi]->pt);
			float dist = std::hypot(pt.x() - e->x(), pt.y() - e->y());
			if (dist < min_dist) {
				min_dist = dist;
				vertex_selected = true;
				selected_vertex_desc = *vi;
			}
		}

		// hit test against the edges
		if (!vertex_selected) {
			QVector2D mouse_pt(e->x(), e->y());

			RoadEdgeIter ei, eend;
			for (boost::tie(ei, eend) = boost::edges(roads.graph); ei != eend; ei++) {
				if (!roads.graph[*ei]->valid) continue;

				for (int i = 0; i < roads.graph[*ei]->polyline.size() - 1; i++) {
					QVector2D p0 = worldToScreenCoordinates(roads.graph[*ei]->polyline[i]);
					QVector2D p1 = worldToScreenCoordinates(roads.graph[*ei]->polyline[i + 1]);
					double dist = RoadGraph::pointSegmentDistance(p0, p1, mouse_pt);
					if (dist <= min_dist) {
						min_dist = dist;
						edge_selected = true;
						selected_edge_desc = *ei;
					}
				}
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
		update();
	}
	else {
	}
		
	prev_mouse_pt = e->pos();
}

void Canvas::mouseReleaseEvent(QMouseEvent* e) {
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
	default:
		break;
	}
}

