#include "Canvas.h"
#include <QPainter>
#include <iostream>
#include <QFileInfoList>
#include <QDir>
#include <QMessageBox>
#include <QResizeEvent>
#include <QtWidgets/QApplication>
#include <QDate>
#include "MainWindow.h"

Canvas::Canvas(MainWindow* mainWin) {
	this->mainWin = mainWin;
	ctrlPressed = false;
	shiftPressed = false;
}

Canvas::~Canvas() {
}

void Canvas::clear() {
	update();
}

void Canvas::open(const QString& filename) {
	editor.load(filename);
	update();
}

void Canvas::save(const QString& filename) {
	editor.save(filename);
}

void Canvas::undo() {
	editor.undo();
	update();
}

void Canvas::redo() {
	editor.redo();
	update();
}

void Canvas::deleteEdge() {
	editor.deleteEdge();
	update();
}

void Canvas::planarGraph() {
	editor.roads.planarify();
	update();
}

void Canvas::paintEvent(QPaintEvent *e) {
	QPainter painter(this);
	painter.fillRect(0, 0, width(), height(), QColor(255, 255, 255));

	editor.draw(painter, prev_mouse_pt);
}

void Canvas::mousePressEvent(QMouseEvent* e) {
	// This is necessary to get key event occured even after the user selects a menu.
	setFocus();

	if (e->buttons() & Qt::LeftButton) {
		QVector2D pt = editor.screenToWorldCoordinates(e->x(), e->y());

		if (QApplication::keyboardModifiers() & Qt::ControlModifier) {
			// add a vertex
			editor.addVertex(pt);
			setMouseTracking(true);
		}
		else {
			if (editor.selectVertex(pt)) {
				// do nothing
			}
			else if (editor.selectEdge(pt)) {
				mainWin->propertyWidget->setRoadEdge(editor.roads.graph[editor.selected_edge_desc]);
			}
		}
	}

	prev_mouse_pt = e->pos();
	update();
}

void Canvas::mouseMoveEvent(QMouseEvent* e) {
	if (e->buttons() & Qt::RightButton) {
		editor.moveCamera(e->pos() - prev_mouse_pt);
	}
	else if (e->buttons() & Qt::LeftButton) {
		if (editor.vertex_selected) {
			QVector2D pt = editor.screenToWorldCoordinates(e->x(), e->y());

			editor.moveVertex(pt);
		}
	}
	
	prev_mouse_pt = e->pos();
	update();
}

void Canvas::mouseReleaseEvent(QMouseEvent* e) {
	if (e->button() == Qt::LeftButton) {
		if (editor.vertex_selected) {
			QVector2D pt = editor.screenToWorldCoordinates(e->x(), e->y());
			editor.completeMovingVertex(pt);
			update();
		}
	}
}

void Canvas::mouseDoubleClickEvent(QMouseEvent* e) {
	if (editor.adding_new_edge) {
		setMouseTracking(false);
		editor.completeAddingVertex();
		update();
	}
}

void Canvas::wheelEvent(QWheelEvent* e) {
	editor.zoom(e->delta() * 0.001, width(), height());
	update();
}

void Canvas::resizeEvent(QResizeEvent *e) {
	editor.resize(e->oldSize(), e->size());
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
		editor.adding_new_edge = false;
		break;
	}

	update();
}

void Canvas::keyReleaseEvent(QKeyEvent* e) {
	ctrlPressed = false;
	shiftPressed = false;

	switch (e->key()) {
	default:
		break;
	}
}

