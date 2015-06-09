/** \file qdrawboxwidget.cpp
 * \brief Very crude graphic plot output
 *
 * \author Copyright (C) 2014 samplemaker
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 *
 * @{
 */

#include <QPainter>
#include <math.h>

#include "qdrawboxwidget.h"


QDrawBoxWidget::QDrawBoxWidget(QWidget *parent) : QWidget(parent)
{
    setMinimumSize(maxx, maxy);
    pixmap = new QPixmap(maxx, maxy);
    QPainter paintToMap(pixmap);
    paintToMap.setPen(QColor("#ffffff"));
    paintToMap.setBrush(QBrush("#ffffff"));
    paintToMap.drawRect(0, 0, maxx, maxy);
}


/* a paint event happens if repaint() or update() was invoked */
void QDrawBoxWidget::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);

    QPainter painter(this);
    painter.drawPixmap(QPoint(0,0), *pixmap);
}


void QDrawBoxWidget::drawLine(int x1, int y1, int x2, int y2)
{
    QPainter paintToMap(pixmap);
    paintToMap.setPen(QPen(Qt::red, 1));
    paintToMap.drawLine(x1, maxy - y1, x2, maxy - y2);
}


/** round up y-axis so that a good looking graticule can be drawn */
void QDrawBoxWidget::getMaxYticks(double value, double *maxY, double *increment){
    /* 1.) value > 1:
     * let a=2683=2.683*10^4 -> log(a)=log(2.683)+4=0.xyz+4=4.xyz
     * hence: a=10^0.xyz*10^4=10^(4.xyz-4)*10^4=
     *        10^(log(a)-trunc(log(a)))*10^trunc(log(a))
     *
     * 2.) value < 1:
     * let a=0.02683=2.683*10^-2 -> -log(a)=-log(2.683)+2=-0.xyz+2=a.bc
     * hence: a=10^0.xyz*10^-2= 10^(a.bc-ceil(a.bc))*10^(ceil(a.bc))
     */
    int exponent;
    double lg10 = log10(value);
    if (value < 1.0)
        exponent = -ceil(-lg10);
    else
        exponent = trunc(lg10);
    double mantissa = pow(10,lg10-exponent);
    double mul = pow(10,exponent);
    /* 1.) now we have mantissa = 2.683, exponent = 4 and mul = 10^4
     * 2.) and mantissa = 2.683, exponent = -2 and mul = 10^-2
     */

    /* gnuplot y-axis format depending on mantissa:
     * maxy={{1.0,1.2,1.4,1.6,1.8,2.0},
            {2.5,3.0,3.5,4.0,4.5,5.0},
            {6.0,7.0,8.0,9.0}}
     * steps={0.2,0.5,1.0}
     */
    if (value > 0.0){
      if (mantissa <= 2.0){
        *maxY = 2.0*0.1*ceil(0.5*10.0*mantissa)*mul;
        *increment = (0.2*mul);
      }
      else if(mantissa <= 5.0){
        *maxY = 0.5*ceil(2.0*mantissa)*mul;
        *increment = (0.5*mul);
      }
      else{
        *maxY = ceil(mantissa)*mul;
        *increment = (1.0*mul);
      }
    }
    else{
        *maxY = 10;
        *increment = 1;
    }
    //qWarning() << value << mantissa << mul << *maxY << exponent << lg10 << *increment;
}


/* create a graticule with max_xticks and display the data vector datapoints (length "len")
 * or the last max_xticks datapoints from the vector (whatever rules to fit max_xticks) */
void QDrawBoxWidget::drawCurve(double *data,
                               const unsigned int len,
                               const unsigned int max_xticks)
{
    QPainter paintToMap(pixmap);
    paintToMap.setBrush(Qt::darkBlue);
    paintToMap.drawRect(0, 0, maxx, maxy);

    unsigned int start = (len > max_xticks) ? (len - max_xticks) : 0;

    double peak = 0;
    for (unsigned int i = start; i < len; i ++){
      if (data[i] > peak) { peak = data[i];}
    }
    double inc;
    double data_maxy;
    getMaxYticks(peak, &data_maxy, &inc);

    paintToMap.setPen(QPen(Qt::red, 1));

    /* boundarys of the graticule with respect to (maxx,maxy) */
    const int x1_mn = 50;
    const int x2_mn = maxx - 20;
    const int y1_mn = 30;
    const int y2_mn = maxy - 20;
    /* draw the data */
    for (unsigned int i = start; i < len - 1; i ++){
       int y1 = (int)((double)(y2_mn-y1_mn)*(double)(data[i])/(double)(data_maxy) );
       int x1 = (int)((double)(x2_mn-x1_mn)*(double)(i-start)/(double)(max_xticks-1) );
       int y2 = (int)((double)(y2_mn-y1_mn)*(double)(data[i+1])/(double)(data_maxy) );
       int x2 = (int)((double)(x2_mn-x1_mn)*(double)(i+1-start)/(double)(max_xticks-1) );
       paintToMap.drawLine(x1_mn+x1, y2_mn-y1, x1_mn+x2, y2_mn-y2);
    }

    paintToMap.setPen(QPen(Qt::white, 1));

    /* boundary rectangle */
    paintToMap.drawLine(x1_mn, y1_mn, x1_mn, y2_mn);
    paintToMap.drawLine(x2_mn, y1_mn, x2_mn, y2_mn);
    paintToMap.drawLine(x1_mn, y1_mn, x2_mn, y1_mn);
    paintToMap.drawLine(x1_mn, y2_mn, x2_mn, y2_mn);

    /* x-ticks in increments of 5 */
    for (unsigned int i = 0; (i < max_xticks - 1); i += 5){
       int x1 = (int)((double)(x2_mn-x1_mn)*(double)(i)/(double)(max_xticks-1) );
       paintToMap.drawLine(x1_mn+x1, y2_mn-5, x1_mn+x1, y2_mn);
       paintToMap.drawLine(x1_mn+x1, y1_mn+5, x1_mn+x1, y1_mn);
    }
    /* y-ticks */
    for (double i = 0; i < data_maxy; i = i + inc){
      int y1 = (int)((double)(y2_mn-y1_mn)*(double)(i)/(double)(data_maxy) );
      paintToMap.drawLine(x1_mn, y2_mn-y1, x1_mn+5, y2_mn-y1);
      paintToMap.drawLine(x2_mn, y2_mn-y1, x2_mn-5, y2_mn-y1);
    }

    paintToMap.drawText(10, y1_mn, QString::number(data_maxy));
    paintToMap.drawText(10, y2_mn, QString("0"));

    repaint();
}
