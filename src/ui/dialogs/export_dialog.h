#ifndef EXPORT_DIALOG_H
#define EXPORT_DIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QSpinBox>

namespace SFinGe {

class ExportDialog : public QDialog {
    Q_OBJECT

public:
    explicit ExportDialog(QWidget *parent = nullptr);
    
    QString getFormat() const;
    int getQuality() const;

private:
    void setupUi();
    
    QComboBox* m_formatCombo;
    QSpinBox* m_qualitySpin;
};

}

#endif
