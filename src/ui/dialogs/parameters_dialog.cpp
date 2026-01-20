#include "parameters_dialog.h"
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QJsonDocument>

namespace SFinGe {

ParametersDialog::ParametersDialog(const FingerprintParameters& params, QWidget *parent)
    : QDialog(parent)
    , m_parameters(params) {
    setupUi();
}

void ParametersDialog::setupUi() {
    setWindowTitle(tr("Edit Parameters (JSON)"));
    resize(500, 400);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    m_textEdit = new QTextEdit(this);
    m_textEdit->setFont(QFont("Monospace", 10));
    
    QJsonDocument doc(m_parameters.toJson());
    m_textEdit->setPlainText(doc.toJson(QJsonDocument::Indented));
    
    mainLayout->addWidget(m_textEdit);
    
    QDialogButtonBox* buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);
}

FingerprintParameters ParametersDialog::getParameters() const {
    QJsonDocument doc = QJsonDocument::fromJson(m_textEdit->toPlainText().toUtf8());
    if (!doc.isNull() && doc.isObject()) {
        FingerprintParameters params;
        params.fromJson(doc.object());
        return params;
    }
    return m_parameters;
}

}
