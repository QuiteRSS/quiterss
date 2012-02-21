#ifndef FILTERRULES_H
#define FILTERRULES_H

#include <QtGui>

class ItemRules : public QWidget
{
  Q_OBJECT
public:
  ItemRules(QWidget * parent = 0) : QWidget(parent)
  {
    QStringList itemList;
    QComboBox *comboBox1 = new QComboBox();
    itemList << tr("Title") << tr("Status") << tr("Description")
             << tr("Author") << tr("Category");
    comboBox1->addItems(itemList);
    QComboBox *comboBox2 = new QComboBox();
    itemList.clear();
    itemList << tr("contains") << tr("doesn't contains")
             << tr("is") << tr("isn't");
    comboBox2->addItems(itemList);

    lineEdit = new QLineEdit();

    addButton = new QToolButton();
    addButton->setIcon(QIcon(":/images/addFeed"));
    addButton->setAutoRaise(true);
    deleteButton = new QToolButton();
    deleteButton->setIcon(QIcon(":/images/deleteFeed"));
    deleteButton->setAutoRaise(true);

    QHBoxLayout *buttonsLayout = new QHBoxLayout();
    buttonsLayout->setAlignment(Qt::AlignCenter);
    buttonsLayout->setMargin(0);
    buttonsLayout->setSpacing(5);
    buttonsLayout->addWidget(comboBox1);
    buttonsLayout->addWidget(comboBox2);
    buttonsLayout->addWidget(lineEdit, 1);
    buttonsLayout->addWidget(addButton);
    buttonsLayout->addWidget(deleteButton);

    setLayout(buttonsLayout);

    connect(deleteButton, SIGNAL(clicked()), this, SLOT(deleteFilterRules()));
  }

  QToolButton *addButton;
  QToolButton *deleteButton;
  QLineEdit *lineEdit;

private slots:
  void deleteFilterRules ()
  {
    emit siganlDeleteFilterRules(this);
  }

signals:
   void siganlDeleteFilterRules (ItemRules *item);
};

class FilterRulesDialog : public QDialog
{
  Q_OBJECT
private:
  QSettings *settings_;
  QDialogButtonBox *buttonBox;
  QScrollArea *infoScrollArea;
  QVBoxLayout *infoLayout;
  QWidget *infoWidget;

public:
  explicit FilterRulesDialog(QWidget *parent = 0, QSettings *settings = 0);
  QLineEdit *filterName;

signals:

public slots:

private slots:
  void closeDialog();
  void addFilterRules();
  void deleteFilterRules(ItemRules *item);

};

#endif // FILTERRULES_H
