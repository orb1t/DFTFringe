/******************************************************************************
**
**  Copyright 2016 Dale Eason
**  This file is part of DFTFringe
**  is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation version 3 of the License

** DFTFringe is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with DFTFringe.  If not, see <http://www.gnu.org/licenses/>.

****************************************************************************/
#ifndef ZERNIKEDLG_H
#define ZERNIKEDLG_H

#include <QDialog>
#include <QtCore>
#include <QtGui>
#include "zernikedlg.h"
#include <QAbstractTableModel>
#include <opencv/cv.h>
#include <vector>
#define Z_TERMS 34

namespace Ui {
class zernikeDlg;
}


class ZernTableModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    ZernTableModel(QObject *parent);
    int rowCount(const QModelIndex &parent = QModelIndex()) const ;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    void setValues(std::vector<double> vals);
    bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole);
    Qt::ItemFlags flags(const QModelIndex & index) const ;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    std::vector<double> values;

signals:
    void zernChanged();
};



#endif // ZERNIKEDLG_H
