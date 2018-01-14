#ifndef CANVAS_H
#define CANVAS_H

#include <QWidget>
#include <QKeyEvent>
#include <glm/glm.hpp>
#include <boost/shared_ptr.hpp>
#include "RoadGraph.h"

class MainWindow;

class Canvas : public QWidget {
	Q_OBJECT

public:
	MainWindow* mainWin;
	bool ctrlPressed;
	bool shiftPressed;
	RoadGraph roads;

	QPointF prev_mouse_pt;
	QPointF origin;
	double scale;

	bool vertex_selected;
	RoadVertexDesc selected_vertex_desc;
	bool edge_selected;
	RoadEdgeDesc selected_edge_desc;

public:
	Canvas();
	~Canvas();

	void clear();
	void open(const QString& filename);
	void save(const QString& filename);
	QVector2D screenToWorldCoordinates(const QVector2D& p);
	QVector2D screenToWorldCoordinates(double x, double y);
	QVector2D worldToScreenCoordinates(const QVector2D& p);

protected:
	void paintEvent(QPaintEvent* e);
	void mousePressEvent(QMouseEvent* e);
	void mouseMoveEvent(QMouseEvent* e);
	void mouseReleaseEvent(QMouseEvent* e);
	void wheelEvent(QWheelEvent* e);
	void resizeEvent(QResizeEvent *e);

public:
	void keyPressEvent(QKeyEvent* e);
	void keyReleaseEvent(QKeyEvent* e);
};

#endif // CANVAS_H
