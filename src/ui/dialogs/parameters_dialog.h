#ifndef PARAMETERS_DIALOG_H
#define PARAMETERS_DIALOG_H

#include <QDialog>
#include <QTextEdit>
#include "models/fingerprint_parameters.h"

namespace SFinGe {

class ParametersDialog : public QDialog {
    Q_OBJECT

public:
    explicit ParametersDialog(const FingerprintParameters& params, QWidget *parent = nullptr);
    
    FingerprintParameters getParameters() const;

private:
    void setupUi();
    
    FingerprintParameters m_parameters;
    QTextEdit* m_textEdit;
};

}

#endif
