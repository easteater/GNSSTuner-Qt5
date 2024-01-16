#ifndef GPSMODULECONFIG_H
#define GPSMODULECONFIG_H

#include <QDialog>

namespace Ui {
class GpsModuleConfig;
}

class GpsModuleConfig : public QDialog
{
    Q_OBJECT

public:
    explicit GpsModuleConfig(QWidget *parent = nullptr);
    ~GpsModuleConfig();

private:
    Ui::GpsModuleConfig *ui;
};

#endif // GPSMODULECONFIG_H
