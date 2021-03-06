#include "campage.hpp"
#include "ui_campage.h"
#include "../modelingwizard.hpp"

/*!
 * \brief CamPage::CamPage
 * \param parent
 */
CamPage::CamPage(QWidget *parent) :
    QWizardPage(parent),
    ui(new Ui::CamPage)
{
    ui->setupUi(this);
    setTitle("Set camera angles");
    registerField("CamX", ui->camx);
}

/*!
 * \brief CamPage::~CamPage
 */
CamPage::~CamPage()
{
    delete ui;
}

/*!
 * \brief CamPage::nextId
 * \return 
 */
int CamPage::nextId() const
{
    ModelingWizard * mw = dynamic_cast<ModelingWizard*>(wizard());
    mw->getDataRef().setCamx(ui->camx->value() / 57.3);
    mw->getDataRef().setCamy(ui->camy->value() / 57.3);
    if(ui->groupBox->isChecked()){
        mw->getDataRef().setX_angle(ui->x->value() / 57.3);
        mw->getDataRef().setY_angle(ui->y->value() / 57.3);
    }
    return -1;
}
