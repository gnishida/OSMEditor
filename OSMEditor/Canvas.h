#ifndef CANVAS_H
#define CANVAS_H

#include <QWidget>
#include <QKeyEvent>
#include <boost/shared_ptr.hpp>
#include "RoadGraph.h"
#include "History.h"
#include "RoadGraphEditor.h"

class MainWindow;

class Canvas : public QWidget {
	Q_OBJECT

public:
	MainWindow* mainWin;
	bool ctrlPressed;
	bool shiftPressed;

	QPointF prev_mouse_pt;

	RoadGraphEditor editor;

public:
	Canvas(MainWindow* mainWin);
	~Canvas();

	void clear();
	void open(const QString& filename);
	void save(const QString& filename);
	void undo();
	void redo();
	void deleteEdge();
	void planarGraph();

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
