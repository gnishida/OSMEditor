#ifndef CANVAS_H
#define CANVAS_H

#include <QWidget>
#include <QKeyEvent>
#include <glm/glm.hpp>
#include <boost/shared_ptr.hpp>
#include "RoadGraph.h"
#include "History.h"

class MainWindow;

class Canvas : public QWidget {
	Q_OBJECT

public:
	MainWindow* mainWin;
	bool ctrlPressed;
	bool shiftPressed;
	bool keyAPressed;
	RoadGraph roads;
	History history;

	QPointF prev_mouse_pt;
	QPointF origin;
	double scale;

	bool vertex_selected;
	RoadVertexDesc selected_vertex_desc;
	bool edge_selected;
	RoadEdgeDesc selected_edge_desc;
	bool edge_point_selected;
	int selected_edge_point;
	bool vertex_moved;
	bool adding_new_edge;
	std::vector<QVector2D> new_edge;

public:
	Canvas();
	~Canvas();

	void clear();
	void open(const QString& filename);
	void save(const QString& filename);
	void undo();
	void deleteEdge();
	bool findClosestVertex(const QVector2D& pt, float threshold, RoadVertexDesc& closest_vertex_desc);
	bool findClosestVertexExcept(const QVector2D& pt, float threshold, RoadVertexDesc except_vertex, RoadVertexDesc& closest_vertex_desc);
	bool findClosestEdgePoint(const QVector2D& pt, float threshold, RoadEdgeDesc& closest_edge_desc, int& closest_edge_point);
	bool findClosestEdge(const QVector2D& pt, float threshold, RoadEdgeDesc& closest_edge_desc);
	bool findClosestEdge(const QVector2D& pt, float threshold, RoadEdgeDesc& closest_edge_desc, QVector2D& closest_pt);
	bool findClosestEdgeExcept(const QVector2D& pt, float threshold, RoadVertexDesc except_vertex, RoadEdgeDesc& closest_edge_desc, QVector2D& closest_pt);
	QVector2D screenToWorldCoordinates(const QVector2D& p);
	QVector2D screenToWorldCoordinates(double x, double y);
	QVector2D worldToScreenCoordinates(const QVector2D& p);

protected:
	void paintEvent(QPaintEvent* e);
	void mousePressEvent(QMouseEvent* e);
	void mouseMoveEvent(QMouseEvent* e);
	void mouseReleaseEvent(QMouseEvent* e);
	void mouseDoubleClickEvent(QMouseEvent* e);
	void wheelEvent(QWheelEvent* e);
	void resizeEvent(QResizeEvent *e);

public:
	void keyPressEvent(QKeyEvent* e);
	void keyReleaseEvent(QKeyEvent* e);
};

#endif // CANVAS_H
