#include "gpsmoduleconfig.h"
#include "ui_gpsmoduleconfig.h"

GpsModuleConfig::GpsModuleConfig(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::GpsModuleConfig)
{
    ui->setupUi(this);
}

GpsModuleConfig::~GpsModuleConfig()
{
    delete ui;
}
