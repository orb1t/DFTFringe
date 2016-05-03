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
#include "metricsdisplay.h"
#include "ui_metricsdisplay.h"
#include <vector>
#include "mainwindow.h"
#include "zernikeprocess.h"
metricsDisplay *metricsDisplay::m_instance = 0;
metricsDisplay::metricsDisplay(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::metricsDisplay)
{
    ui->setupUi(this);
    mDiam = ui->metDiam;
    mROC = ui->metROC;
    mRMS = ui->metRMS;
    mStrehl = ui->metStrehl;
    mvalues = ui->tableView;
    mCC = ui->metConic;
    tableModel = new ZernTableModel(parent);
    std::vector<double> val(Z_TERMS,0.);
    ui->tableView->setModel(tableModel);
}

QTableView * metricsDisplay::getZernView(){
    return ui->tableView;
}

metricsDisplay *metricsDisplay::get_instance(QWidget *parent){
    if (m_instance == 0){
        m_instance = new metricsDisplay(parent);
    }
    return m_instance;
}
void metricsDisplay::setWavePerFringe(double val){
    ui->wavesPerFringe->setText(QString().sprintf("Waves Per Fringe: %2.1lf",val));
}

void metricsDisplay::setName(QString name){
    QStringList list = name.split("/");
    int size = list.size();
    if (list.size() > 2){
        name = list[size -2] + "/ " + list[size-1];
    }
    ui->wftName->setText(name);
}

metricsDisplay::~metricsDisplay()
{
    delete ui;

}

void metricsDisplay::on_recomputePB_clicked()
{
    emit recomputeZerns();
}

void metricsDisplay::on_DisableAll_clicked()
{

    for (unsigned int i = 0; i < zernEnables.size(); ++i)
        zernEnables[i] = false;

    emit recomputeZerns();
}
