/*
  PMFToolBox: A toolbox to analyze and post-process the output of
  potential of mean force calculations.
  Copyright (C) 2020  Haochuan Chen

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU Affero General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Affero General Public License for more details.

  You should have received a copy of the GNU Affero General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "aboutdialog.h"
#include "ui_aboutdialog.h"

#include <QFile>
#include <QTextStream>

AboutDialog::AboutDialog(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::AboutDialog)
{
  ui->setupUi(this);

  QList<AuthorInfo> authors = authorsInfo();
  for (int i = 0; i < authors.size(); ++i) {
    QLabel *nameLabel = new QLabel;
    QLabel *emailLabel = new QLabel;
    nameLabel->setText(authors[i].mName);
    emailLabel->setText(authors[i].mEmail);
    QHBoxLayout *vLayout = new QHBoxLayout;
    vLayout->addWidget(nameLabel);
    vLayout->addWidget(emailLabel);
    ui->vLayoutAuthor->addLayout(vLayout);
  }
  ui->vLayoutAuthor->addStretch(1);

  QList<LicenseInfo> submodule_licenses = licensesInfo();
  for (int i = 0; i < submodule_licenses.size(); ++i) {
    QWidget *widget = new QWidget;
    QHBoxLayout *hLayout = new QHBoxLayout;
    QTextBrowser *textBrowser = new QTextBrowser;
    textBrowser->setAutoFillBackground(false);
    textBrowser->setText(submodule_licenses[i].mLicense);
    hLayout->addWidget(textBrowser);
    widget->setLayout(hLayout);
    ui->tabLicenseSubmodule->addTab(widget, submodule_licenses[i].mProgram);
  }
}

AboutDialog::~AboutDialog()
{
  delete ui;
}

QString AboutDialog::getStringFromResource(const QString &res_file) const
{
  // partially swipe from QMMP's code
  QString ret_string;
  QFile file(res_file);
  if (file.open(QIODevice::ReadOnly)) {
    QTextStream ts(&file);
    ts.setCodec("UTF-8");
    ret_string = ts.readAll();
    file.close();
  }
  return ret_string;
}

QList<AuthorInfo> AboutDialog::authorsInfo() const
{
  QList<AuthorInfo> result;
  result.push_back(AuthorInfo{"Haochuan Chen", "summersnow9403@gmail.com"});
  return result;
}

QList<LicenseInfo> AboutDialog::licensesInfo() const
{
  QList<LicenseInfo> result;
  result.push_back(LicenseInfo{"PMFToolBox", getStringFromResource(":/licenses/LICENSE")});
  return result;
}
