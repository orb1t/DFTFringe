/******************************************************************************
**
**  Copyright 2016 Dale Eason
**  This file is part of DFTFringe
**  is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, version 3 of the License

** DFTFringe is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with DFTFringe.  If not, see <http://www.gnu.org/licenses/>.

****************************************************************************/
#include <QtWidgets>
#ifndef QT_NO_PRINTER
#include <QPrinter>
#include <QPrintDialog>
#endif

#include "IgramArea.h"
#include "circleoutline.h"
#include <QtGlobal>
#include <math.h>

#include "opencv/cv.h"
#include "graphicsutilities.h"
#include <iostream>
#include <fstream>
#include <QMessageBox>
#include "zernikeprocess.h"
#include "zernikedlg.h"
#include "mainwindow.h"
#include <qsettings.h>
#include "imagehisto.h"
#include "simigramdlg.h"
#include "settings2.h"

void undoStack::clear() {
    m_stack.clear();
    m_redo.clear();
}

void undoStack::push(QImage img, CircleOutline outline){
    m_redo.clear();
    m_stack.push_back(outlinePair(img,outline));

}

outlinePair undoStack::undo() {
    if (m_stack.size() > 1) {
        m_redo.push_back(m_stack.back());
        m_stack.pop_back();
    }
    return m_stack.back();

}
outlinePair undoStack::redo(){
    if (m_redo.size()> 0) {
        m_stack.push_back(m_redo.back());
        m_redo.pop_back();
    }
    return m_stack.back();
}
double distance(QPointF p1, QPointF p2)
{
    return qSqrt((p1.x() - p2.x())*(p1.x() - p2.x()) + (p1.y() - p2.y())*(p1.y() - p2.y()));
}

IgramArea::IgramArea(QWidget *parent, void *mw)
    : QWidget(parent),m_mw(mw),m_hideOutlines(false),scale(1.),outterPcount(0), innerPcount(0), zoomFactor(0.),m_current_boundry(OutSideOutline),
      zoomIndex(0),dragMode(false),cropTotalDx(0), cropTotalDy(0), hasBeenCropped(false)
{



    m_innerP1 = m_innerP2 = m_OutterP1 = m_OutterP2 = QPointF(0.,0.);
    setAttribute(Qt::WA_StaticContents);
    modified = false;
    scribbling = false;
    QSettings set;
    centerPenColor = set.value("igramCenterLineColor", QColor("white").name()).toString();
    edgePenColor = set.value("igramEdgeLineColor", QColor("green").name()).toString();
    edgePenWidth = set.value("igramEdgeWidth", 3).toInt();
    centerPenWidth = set.value("igramCenterWidth", 3).toInt();
    opacity = set.value("igramLineOpacity", 65.).toDouble();
    lineStyle = set.value("igramLineStyle", 1).toInt();
    setMouseTracking(true);
    m_dftThumb = new dftThumb();
    m_dftThumb->setWindowFlags(    Qt::WindowStaysOnTopHint);
    m_outlineTimer = new QTimer(this);
    connect(m_outlineTimer, SIGNAL(timeout()),this, SLOT(outlineTimerTimeout()));
    m_doGamma = false;
    m_gammaValue = 2.2;
    m_lastGamma = 2.2;
    needToConvertBGR = false;
}
void IgramArea::outlineTimerTimeout(){
    m_outlineTimer->stop();

}

void IgramArea::generateSimIgram()
{

    DrawSimIgram();
}


void IgramArea::DrawSimIgram(void){

    simIgramDlg dlg;
    if (!dlg.exec())
        return;
    //m_demo->hide();
    int wx = dlg.size;
    int wy = wx;
    double xcen = (double)(wx-1)/2.;
    double ycen = (double)(wx-1)/2.;
    double rad = xcen-2;
    cv::Mat simgram = cv::Mat::zeros(wx,wx,CV_8UC4);


    double rho;
    double spacing = 1.;

    mirrorDlg *md = ((MainWindow*)(m_mw))->m_mirrorDlg;
    for (int i = 0; i <  wx-0; ++i)
    {
        double x1 = (double)(i - xcen) / rad;
        for (int j = 0; j < wy-0; ++j)
        {
            double y1 = (double)(j - ycen ) /rad;
            rho = sqrt(x1 * x1 + y1 * y1);
            if (rho <= 1.)
            {
            double phi = atan2(y1,x1);

                double S1 = md->z8 * dlg.correction * .01d * Zernike(8,x1,y1) +
                        dlg.xtilt * Zernike(1,x1,y1) +
                        dlg.ytilt * Zernike(2,x1,y1) +
                        dlg.defocus * Zernike(3,x1,y1) +
                        dlg.xastig * Zernike(4, x1, y1)+
                        dlg.yastig * Zernike(5, x1,y1) +
                        dlg.star * cos(10.  *  phi) +
                        dlg.ring * cos(2 * 10.  * rho);
                if (dlg.zernNdx > 0)
                    S1 += (dlg.zernValue * Zernike(dlg.zernNdx, x1,y1));

                int iv = cos(spacing *2 * M_PI * S1) * 100 + 120;


                //simgram.at<Vec4f>(i,j)[0] = 0;
                simgram.at<Vec4b>(j,i)[2] = iv;
                simgram.at<Vec4b>(j,i)[3] = 255;
            }
            else
            {
                simgram.at<Vec4b>(j,i) = Vec4f(0.,00.,100.,0);
            }
        }
    }
    //cv::circle(simgram, cv::Point(xcen,ycen),rad, cv::Scalar(200,200,200,200),-1);
    //simgram.convertTo(simgram,CV_8UC4);
    //imwrite("tmp.png",simgram);
    //openImage("tmp.png");

    //cv::cvtColor(simgram,simgram, CV_BGRA2RGBA);
    igramImage = QImage((uchar*)simgram.data,
                        simgram.cols,
                        simgram.rows,
                        simgram.step,
                        QImage::QImage::Format_RGB32);

    qDebug() << "sim format " << igramImage.format();

    zoomIndex = 0;
    m_outsideHist.clear();
    m_centerHist.clear();
    modified = false;
    igramDisplay = igramImage.copy();
    igramImage = igramDisplay.copy();
    resizeImage();

    m_outside = CircleOutline(QPointF(xcen, ycen),rad);
    m_OutterP1 = m_outside.m_p1.m_p;
    m_OutterP2 = m_outside.m_p2.m_p;
    outterPcount = 2;
    m_outsideHist.push(igramImage,m_outside);
    drawBoundary();
    fitScale = 1;
    zoomFactor = 1;
    zoomIndex = 0;
    scale = fitScale;
    m_filename = "simulatedIgram";
    update();
    emit showTab(0);
    emit upateColorChannels(igramImage);
}
void IgramArea::doGamma(double gammaV){

        cv::Mat mm(igramImage.height(), igramImage.width(), CV_8UC4, igramImage.bits(), igramImage.bytesPerLine());
        mm.convertTo(mm,CV_32FC4);

        //cv::Mat bgr_planes[4];
        //split(mm,bgr_planes);
        //merge(bgr_planes,4,mm);

        mm /= 255;
        pow(mm,gammaV,mm);
        mm *= 255;
        //if (needToConvertBGR)
            //cvtColor(mm,mm,CV_BGRA2RGBA);
        needToConvertBGR = false;
        mm.convertTo(mm,CV_8UC4);

        QImage img((uchar*)mm.data, mm.cols, mm.rows,mm.step1() ,QImage::Format_RGB32);
        igramImage = img.copy();

}

bool IgramArea::openImage(const QString &fileName)

{
    QImage loadedImage;
    if (!loadedImage.load(fileName))
        return false;
        qDebug() << "loaded format " << loadedImage.format();


    if (Settings2::getInstance()->shouldHflipIgram())
        loadedImage = loadedImage.mirrored(true,false);
    hasBeenCropped = false;
    needToConvertBGR = true;
    //m_demo->hide();
    m_filename = fileName;
    zoomIndex = 0;
    igramImage = loadedImage;
    if (m_doGamma)
        doGamma(m_gammaValue);
    igramDisplay = igramImage.copy();
    m_outsideHist.clear();
    m_outsideHist.push(igramImage, m_outside);
    m_centerHist.clear();
    m_centerHist.push(igramImage,m_center);
    modified = false;
    QSettings set;
    double rad = set.value("lastOutsideRad", 0).toDouble();


    gscrollArea->setWidgetResizable(true);
    if (igramImage.height() > height())
            fitScale = (double)parentWidget()->height()/(double)igramImage.height();
    else
        fitScale = 1;
    scale = fitScale;
    QFileInfo info(fileName);
    lastPath = info.absolutePath();
    QSettings settings;
    settings.setValue("lastPath",lastPath);
    resizeImage();

    if (rad > 0) {
        double cx = set.value("lastOutsideCx", 0).toDouble();
        double cy = set.value("lastOutsideCy",0).toDouble();

        // check that outline fits inside current image.
        int width = loadedImage.size().width();
        int height = loadedImage.size().height();
        bool tooBig = false;
        if (cx + rad > width ||  (cx - rad) < 0 ||  cy + rad > height || (cy-rad) < 0)
            tooBig = true;


        if (tooBig)
            m_outside = CircleOutline(QPointF(0,0),0);
        else {
            m_outside = CircleOutline(QPointF(cx,cy),rad);
            m_OutterP1 = m_outside.m_p1.m_p;
            m_OutterP2 = m_outside.m_p2.m_p;
            outterPcount = 2;
            drawBoundary();
        }
    }
    rad = set.value("lastInsideRad", 0).toDouble();
    if (rad) {
        double cx = set.value("lastInsideCx", 0).toDouble();
        double cy = set.value("lastInsideCy",0).toDouble();
        m_center = CircleOutline(QPointF(cx,cy),rad);
        m_innerP1 = m_center.m_p1.m_p;
        m_innerP2 = m_center.m_p2.m_p;
        innerPcount = 2;
        drawBoundary();
    }
    cropTotalDx = cropTotalDy = 0;

    update();
    if (m_outside.m_radius > 0.)
        emit upateColorChannels(igramImage);
    emit showTab(0);

    return true;
}


bool IgramArea::saveImage(const QString &fileName, const char *fileFormat)

{
    QImage visibleImage = igramImage;


    if (visibleImage.save(fileName, fileFormat)) {
        modified = false;
        return true;
    } else {
        return false;
    }
}

void IgramArea::setPenColor(const QColor &newColor)

{
    edgePenColor = newColor;
}



void IgramArea::setPenWidth(int newWidth)

{
    edgePenWidth = newWidth;
}

void IgramArea::clearImage()
//! [9] //! [10]
{
    igramImage.fill(qRgb(255, 255, 255));
    modified = true;
    update();
}
void IgramArea::undo(){
    if (m_current_boundry == OutSideOutline){
        outlinePair p = m_outsideHist.undo();
        m_outside = p.m_outline;
        igramImage = p.m_image;
        m_OutterP1 = m_outside.m_p1.m_p;
        m_OutterP2 = m_outside.m_p2.m_p;
    } else {
        outlinePair p = m_centerHist.undo();
        m_center = p.m_outline;
        igramImage = p.m_image;
        m_innerP1 = m_center.m_p1.m_p;
        m_innerP2 = m_center.m_p2.m_p;
    }
    resizeImage();
    drawBoundary();
}
void IgramArea::redo(){
    if (m_current_boundry == OutSideOutline){
        outlinePair p = m_outsideHist.redo();

        m_outside = p.m_outline;
        igramImage = p.m_image;
        m_OutterP1 = m_outside.m_p1.m_p;
        m_OutterP2 = m_outside.m_p2.m_p;
    }
    else {
        outlinePair p = m_centerHist.redo();

        m_center = p.m_outline;
        igramImage = p.m_image;
        m_innerP1 = m_center.m_p1.m_p;
        m_innerP2 = m_center.m_p2.m_p;

    }
    resizeImage();
    drawBoundary();
}

bool IgramArea::eventFilter(QObject *object, QEvent *event)
{
    if (igramImage.isNull())
        return false;

    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_Z &&
                keyEvent->modifiers().testFlag(Qt::ControlModifier)) {
            undo();
            // cntrl-Z
            return true;
        }
        else if (keyEvent->key() == Qt::Key_Y &&
                keyEvent->modifiers().testFlag(Qt::ControlModifier)) {
            // cntrl-y
            redo();
            return true;
        } else
            return false;
    }

    return QObject::eventFilter( object, event );
}

void IgramArea::increase() {

    if (m_current_boundry == OutSideOutline) {
        m_outside.enlarge(1);
        m_OutterP1 = m_outside.m_p1.m_p;
        m_OutterP2 = m_outside.m_p2.m_p;
    }
    else {
        m_center.enlarge(1);
        m_innerP1 = m_center.m_p1.m_p;
        m_innerP2 = m_center.m_p2.m_p;
    }
    drawBoundary();
}
void IgramArea::decrease(){

    if (m_current_boundry == OutSideOutline) {
        m_outside.enlarge(-1);
        m_OutterP1 = m_outside.m_p1.m_p;
        m_OutterP2 = m_outside.m_p2.m_p;
    }
    else {
        m_center.enlarge(-1);
        m_innerP1 = m_center.m_p1.m_p;
        m_innerP2 = m_center.m_p2.m_p;
    }
    drawBoundary();
}

void IgramArea::wheelEvent (QWheelEvent *e)
{
    if (igramImage.isNull())
        return;
    qDebug() << "Scroll wheel " << e->delta();

    QString result;
    if (e->delta() == 0)
        return;

    int del = e->delta()/120;

    if( e->modifiers() & Qt::ControlModifier) {
        setCursor(Qt::SizeAllCursor);
        if (m_current_boundry == OutSideOutline) {
            m_outside.enlarge(del);
            m_OutterP1 = m_outside.m_p1.m_p;
            m_OutterP2 = m_outside.m_p2.m_p;
        }
        else {
            m_center.enlarge(del);
            m_innerP1 = m_center.m_p1.m_p;
            m_innerP2 = m_center.m_p1.m_p;
        }

        drawBoundary();
        return;
    }
    QPointF pos = e->pos();

    zoompt = pos/scale;

    zoomIndex += del;
    if (zoomIndex < 0) {
        zoomIndex = 0;
    }
    qDebug() << "Zoom Index "<< zoomIndex;

    if (zoomIndex > 0) {
        zoomFactor = fitScale + .5 * zoomIndex;
        scale = zoomFactor;
        qDebug() << QString().sprintf("scale %lf",scale);
        gscrollArea->setWidgetResizable(false);
        resize(igramImage.size() * scale);

        //gscrollArea->ensureVisible(width()/2,height()/2);

        QScrollBar *bar = gscrollArea->horizontalScrollBar();
        bar->setValue(zoompt.x() * scale - bar->pageStep()/2);

        bar = gscrollArea->verticalScrollBar();
        bar->setValue(zoompt.y() * scale - bar->pageStep()/2);


    }
    else {
        scale = fitScale;
        gscrollArea->setWidgetResizable(true);
    }

    drawBoundary();
}

void IgramArea::mousePressEvent(QMouseEvent *event)
{
    if (igramImage.isNull())
        return;

    if (event->button() == Qt::LeftButton) {
        QPointF Raw = event->pos();

        QPointF pos = Raw;
        if (event->modifiers() & Qt::ShiftModifier){
            setCursor(Qt::OpenHandCursor);
            dragMode = true;
            lastPoint = Raw;
            drawBoundary();
            return;
        }

        scribbling = true;
        if (m_current_boundry == OutSideOutline) m_outside.m_radius = 0;
        else m_center.m_radius = 0;


        pos = pos/scale;
        pos.setX((int)pos.x());
        pos.setY((int)(pos.y()));
        int *Pcount;
        if (m_current_boundry == OutSideOutline){
            Pcount = &outterPcount;
        }
        else {
            Pcount = &innerPcount;
        }

        // if within 50 pixels of the old p1 then replace the old one with this one
        if (*Pcount > 0){    // if count is > 0 then there must be two points.
            int del = distance(pos,m_OutterP1);
            int del2 = distance(pos,m_OutterP2);

            if (del < 200) {
                if (*Pcount > 1){
                    if (m_current_boundry == OutSideOutline) {
                        m_OutterP1= m_OutterP2;
                        m_OutterP2= pos;
                    }
                    else {
                        m_innerP1 = m_innerP2;
                        m_innerP2 = pos;
                    }
                }
            }
            else if (del2< 300){
                if (m_current_boundry == OutSideOutline){
                    m_OutterP2 = pos;
                }
                else {
                    m_innerP2 = pos;
                }
            }
            else {
                if (m_current_boundry == OutSideOutline){
                    m_OutterP1 = pos;
                }
                else {
                    m_innerP1 = pos;
                }
                *Pcount = 1;
            }
        }
        else {
            if (m_current_boundry == OutSideOutline)
                m_OutterP1 = pos;
            else
                m_innerP1 = pos;
            *Pcount=1;
        }

        drawBoundary();
    }

}

void IgramArea::mouseMoveEvent(QMouseEvent *event)
{
    if (igramImage.isNull())
        return;

    QPointF pos = event->pos();
    QPointF scaledPos = pos/scale;
    scaledPos.setX((int)scaledPos.x());
    scaledPos.setY((int)scaledPos.y());

    if ((event->buttons() & Qt::LeftButton) && scribbling) {
        if (m_current_boundry == OutSideOutline){
            outterPcount = 2;
            m_OutterP2 = scaledPos;
            m_OutterP1.ry() = m_OutterP2.y();
            m_OutterP2.ry() = m_OutterP1.y();
        }
        else {
            innerPcount = 2;
            m_innerP2 = scaledPos;
            m_innerP1.ry() = m_innerP2.y();
            m_innerP2.ry() = m_innerP1.y();
        }
        drawBoundary();
    }
    else if ((event->buttons() & Qt::LeftButton) & dragMode){
        if( outterPcount == 2) {
            QPointF del = pos - lastPoint;
            if (m_current_boundry == OutSideOutline) {
                m_outside.translate(del);
                m_OutterP1 += del;
                m_OutterP2 += del;
            }
            else {
                m_center.translate((del));
                m_innerP1 += del;
                m_innerP2 += del;
            }

            drawBoundary();
            lastPoint = pos;

        }
    }
}

void IgramArea::mouseReleaseEvent(QMouseEvent *event)
{
    if (igramImage.isNull())
        return;
    setCursor(Qt::ArrowCursor);

    if (event->button() == Qt::LeftButton && (scribbling || dragMode)) {
        if (m_current_boundry == OutSideOutline){
            m_outside = CircleOutline(m_OutterP1,m_OutterP2);
            m_outsideHist.push(igramImage, m_outside);
        }
        else{
            m_center = CircleOutline(m_innerP1,m_innerP2);
            m_centerHist.push(igramImage, m_center);
        }
        emit enableShiftButtons(true);
    }

    drawBoundary();
    scribbling = false;
    dragMode = false;
}

void IgramArea::resizeEvent(QResizeEvent *event)

{
    if (igramImage.isNull())
        return;

    resizeImage();
    update();

    QWidget::resizeEvent(event);
}

void drawCircle(QPointF& p1, QPointF& p2, QPainter& painter)
{
    CircleOutline c(p1,p2);
    c.draw(painter,1.);
}



void IgramArea::drawBoundary()

{
    QImage tmp = igramImage.copy();

    QPainter painter(&tmp);
    painter.drawImage(0,0,igramImage);

        CircleOutline outside(m_OutterP1,m_OutterP2);
        CircleOutline inside(m_innerP1, m_innerP2);
        if (!m_hideOutlines){
            painter.setOpacity(opacity * .01);
            if (outside.m_radius > 0){
                painter.setPen(QPen(edgePenColor, edgePenWidth, (Qt::PenStyle)lineStyle));
                outside.draw(painter,1.);
            }
            if (inside.m_radius > 0 && innerPcount > 1){
                painter.setPen(QPen(centerPenColor, centerPenWidth, (Qt::PenStyle)lineStyle));
                inside.draw(painter,1.);

            }
        }
        QString msg;
        QString msg2;
        if (outterPcount == 2)
            msg = QString().sprintf("Outside: x=%6.1lf y=%6.1lf, Radius= %6.1lf  ",
                                  outside.m_center.x(),outside.m_center.y(), outside.m_radius);
            m_outside = outside;
        if (innerPcount == 2){
            m_center = inside;


            msg2 = QString().sprintf("center= %6.1lf,%6.1lf radius = %6.2lf scale =%6.2lf",
                    inside.m_center.x(),inside.m_center.y(),inside.m_radius, scale);
        }
        emit statusBarUpdate(msg+msg2);

        m_outlineTimer->start(1000);



    //emit statusBarUpdate(QString().sprintf("%6.1lf,%6.1lf", m_OutterP1.x(),m_OutterP1.y()));
    QPainter p( &igramDisplay);
    p.drawImage(QPoint(0,0), tmp.scaled(tmp.width() * scale, tmp.height() * scale));

    modified = true;
    update();

}

void IgramArea::resizeImage()

{

    if (igramImage.isNull())
        return;

    QSize newSize;
    if (zoomIndex > 0){
        newSize = QSize(igramImage.width() * scale,igramImage.height()* scale);
        gscrollArea->setWidgetResizable(false);
    }
    else{
        newSize = gscrollArea->size();
        gscrollArea->setWidgetResizable(true);
        if (igramImage.height() > height())
                scale = (double)parentWidget()->height()/(double)igramImage.height();
    }
    try {
        QImage newImage(newSize, QImage::Format_RGB32);
        qDebug()<< QString().sprintf("%d",newImage.width());
        if (igramImage.isNull())
            return;
        QPainter painter(&newImage);
        igramDisplay = igramImage.scaled(igramImage.width() * scale, igramImage.height() * scale);
        painter.drawImage(QPoint(0, 0), igramDisplay);
    } catch (...) {
        QMessageBox::warning(NULL,"","Not enough memory to zoom this large");
    }
}


void IgramArea::paintEvent(QPaintEvent *event)

{
    QPainter painter(this);
    QRect dirtyRect = event->rect();
    painter.drawImage(dirtyRect, igramDisplay, dirtyRect);

}

void IgramArea::createActions()
{

    fitToWindowAct = new QAction(tr("&Fit to Window"), this);
    fitToWindowAct->setEnabled(false);
    fitToWindowAct->setCheckable(true);
    fitToWindowAct->setShortcut(tr("Ctrl+f"));
    connect(fitToWindowAct, SIGNAL(triggered()), this, SLOT(fitToWindow()));

}


void IgramArea::crop() {
    // add current bounds to crop history.
    double rad = m_outside.m_radius;
    double cx = m_outside.m_center.x();
    double cy = m_outside.m_center.y();
    QSettings set;
    set.setValue("lastOutsideRad", rad);

    set.setValue("lastinsideRad", m_center.m_radius);
    set.setValue("lastinsideCx", m_center.m_center.x());
    set.setValue("lastInsideCy", m_center.m_center.y());
    int width = igramImage.width();
    int right = width - (rad + cx);
    int left = fmax(0, cx - rad);
    int top = fmax(0, cy - rad);
    int bottom = igramImage.height() - (rad + cy);

    int border = fmin(left,fmin(right,fmin(bottom,fmin(top,20))));
    int x = cx - rad - border;
    int y = cy - rad - border;
    width = 2 * (rad + border);

    igramImage = igramImage.copy(QRect(x, y, width, width));

    crop_dx = x;
    crop_dy = y;
    cropTotalDx += x;
    cropTotalDy += y;

    x = igramImage.width()/2;
    y = igramImage.height()/2;


    m_outside.translate(QPointF(-crop_dx,-crop_dy));
    cx = m_outside.m_center.x() + crop_dx;
    cy = m_outside.m_center.y() + crop_dy;
    set.setValue("lastOutsideCx",cx);
    set.setValue("lastOutsideCy",cy);
    m_center.translate(QPointF(-crop_dx,-crop_dy));
    // need to rescale p1 and p2 because of the crop
    scale = (double)height()/y;
    m_OutterP1 = m_outside.m_p1.m_p;
    m_OutterP2 = m_outside.m_p2.m_p;
    m_innerP1 = m_center.m_p1.m_p;
    m_innerP2 = m_center.m_p2.m_p;
    resizeImage();
    drawBoundary();
    m_outsideHist.push(igramImage, m_outside);
    m_centerHist.push(igramImage, m_center);
    update();
    hasBeenCropped = true;
    emit upateColorChannels(igramImage);
}
void IgramArea::dftReady(QImage img){
    m_dftThumb->setImage(img);
    m_dftThumb->show();
    QRect r = m_dftThumb->geometry();
    QRect wr = geometry();
    QPoint p = mapToGlobal(QPoint(wr.left(), wr.top()));

    m_dftThumb->move(p.x() + wr.width() - r.width(),p.y());
}

void IgramArea::SideOutLineActive(bool checked){
    sideOutlineIsActive = checked;
    if (checked)
       m_current_boundry = OutSideOutline;
    else
       m_current_boundry = CenterOutline;
}

void IgramArea::loadOutlineFile(QString fileName){
    std::ifstream file((fileName.toStdString().c_str()));
    if (!file.is_open()) {
        QMessageBox::warning(this, tr("Read Outline"),
                             tr("Cannot read file %1: ")
                             .arg(fileName));
        return;
    }


    m_outside = readCircle(file);
    CircleOutline sideLobe = readCircle(file);
    if (file.gcount() < file.tellg()) {
        m_center = readCircle(file);
    }
    m_OutterP1 = m_outside.m_p1.m_p;
    m_OutterP2 = m_outside.m_p2.m_p;
    outterPcount = 2;
    drawBoundary();
    m_outsideHist.push(igramImage,m_outside);
    emit enableShiftButtons(true);
}

void IgramArea::readOutlines(){

    QString fileName = QFileDialog::getOpenFileName(this,
                        tr("Read outline file"), lastPath,
                        tr("oln (*.oln)"));
    if (fileName.isEmpty())
        return;
    loadOutlineFile(fileName);

}

void IgramArea::saveOutlines(){
    QSettings settings;
    lastPath = settings.value("lastPath").toString();
    QStringList list = m_filename.split("/");
    QString name = list[list.size()-1];
    int lastPoint = name.lastIndexOf(".");
    QString fileNameNoExt = name.left(lastPoint);
    QString fileName = QFileDialog::getSaveFileName(this,
                        tr("Save outline file"), lastPath + "/" + fileNameNoExt + ".oln",
                        tr("oln (*.oln)"));
    if (fileName.isEmpty())
        return;
    if (QFileInfo(fileName).suffix().isEmpty()) { fileName.append(".oln"); }
    std::ofstream file((fileName.toStdString().c_str()));
    if (!file.is_open()) {
        QMessageBox::warning(this, tr("Write Outline"),
                             tr("Cannot write file %1: ")
                             .arg(fileName));
        return;
    }
    // write oustide outline
    writeCircle(file,m_outside);
    // write sidelobe
    CircleOutline side(m_outside.m_center,0.);
    writeCircle(file, side);


    file.close();
}
void IgramArea::deleteOutline(){
    QSettings set;
    if (m_current_boundry == OutSideOutline){
        m_outsideHist.clear();
        m_OutterP1 = m_OutterP2 = QPointF(0,0);
        m_outside = CircleOutline(QPointF(0,0),0);
        outterPcount = 0;
        set.setValue("lastOutsideRad", 0);
        set.setValue("lastOutsideCx",0);
        set.setValue("lastOutsideCy",0);
    }
    else {
        m_centerHist.clear();
        m_center = CircleOutline(QPointF(0,0),0);
        innerPcount = 0;
        m_innerP1 = m_innerP2 = QPointF(0,0);
        set.setValue("lastinsideRad",0);
        set.setValue("lastinsideCx", 0);
        set.setValue("lastInsideCy", 0);
    }
;

    drawBoundary();
}
void IgramArea::shiftoutline(QPointF p) {
    if (m_current_boundry == OutSideOutline){
        m_outside.translate(p);
        m_OutterP1 = m_outside.m_p1.m_p;
        m_OutterP2 = m_outside.m_p2.m_p;
        m_outsideHist.push(igramImage, m_outside);
    }
    else {
        m_center.translate(p);
        m_innerP1 = m_center.m_p1.m_p;
        m_innerP2 = m_center.m_p2.m_p;
        m_centerHist.push(igramImage, m_center);
    }
    drawBoundary();

}

void IgramArea::shiftUp(){
    shiftoutline(QPointF(0,-1));
}
void IgramArea::shiftDown(){
    shiftoutline(QPointF(0,1));
}
void IgramArea::shiftLeft(){
    shiftoutline(QPointF(-1,0));
}
void IgramArea::shiftRight(){
    shiftoutline(QPointF(1,0));
}
void IgramArea::nextStep(){
    if (m_outside.m_radius == 0){
       QMessageBox::warning(0,"Mirror Outside Boundary Error",
                            "You must first outline the mirror outside edge.");
        return;
    }
    if (!hasBeenCropped)
        crop();

    m_dftThumb->hide();
    emit showTab(1);
}
void IgramArea::save(){

    QStringList mimeTypeFilters;
    foreach (const QByteArray &mimeTypeName, QImageReader::supportedMimeTypes())
        mimeTypeFilters.append(mimeTypeName);
    mimeTypeFilters.sort();
    QSettings settings;
    QString lastPath = settings.value("projectPath",".").toString();
    QFileDialog dialog(this, tr("Save interferogram"),lastPath);
    //dialog.setAcceptMode(QFileDialog::AcceptOpen);
    dialog.setMimeTypeFilters(mimeTypeFilters);
    dialog.selectMimeTypeFilter("image/jpeg");
    QString fileName = dialog.getSaveFileName();
    if (fileName.isEmpty())
        return;
    QImage pm(igramImage.width(),igramImage.height(),igramImage.format());
    QPainter painter(&pm);

    painter.drawImage(0,0,igramImage);
    //painter.setOpacity(.65);
    //painter.setPen(QPen(Qt::white, 0.0));//, Qt::DotLine, Qt::RoundCap,Qt::RoundJoin));

    //m_outside.draw(painter,1.);
    pm.save(fileName);

}
void IgramArea::hideOutline(bool checked){
    m_hideOutlines = checked;
    drawBoundary();
}
void IgramArea::igramOutlineParmsChanged(int edgeW, int centerW, QColor edgeC, QColor centerC, double op, int style){
    edgePenWidth = edgeW;
    centerPenWidth = centerW;
    edgePenColor = edgeC;
    centerPenColor = centerC;
    opacity = op;
    lineStyle = style;
    drawBoundary();

}
void IgramArea::gammaChanged(bool checked, double value){
    m_gammaValue = value;
    m_doGamma = checked;
    if (igramImage.isNull())
        return;
    if (checked){
        doGamma(value);
        m_lastGamma = value;
    }
    else {
        doGamma(1./m_lastGamma);
        m_lastGamma = 0;
    }
    igramDisplay = igramImage.copy();
    resizeImage();
    if (m_outside.m_radius > 0.)
        emit upateColorChannels(igramImage);
    update();
}
