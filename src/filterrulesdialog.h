#ifndef FILTERRULES_H
#define FILTERRULES_H

#include <QtGui>

class ItemRules : public QWidget
{
  Q_OBJECT
private:
  QToolButton *deleteButton;
  QComboBox *comboBox2;
  QComboBox *comboBox3;

public:
  ItemRules(QWidget * parent = 0) : QWidget(parent)
  {
    QComboBox *comboBox1 = new QComboBox();
    comboBox2 = new QComboBox();
    comboBox3 = new QComboBox();
    lineEdit = new QLineEdit();

    QStringList itemList;
    itemList << tr("Title")  << tr("Description")
             << tr("Author") << tr("Category") << tr("Status")
             /*<< tr("Published") << tr("Received")*/;
    comboBox1->addItems(itemList);

    itemList.clear();
    itemList << tr("New") << tr("Read") << tr("Stared");
    comboBox3->addItems(itemList);

    currentIndexChanged("Title");

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
    buttonsLayout->addWidget(comboBox3, 1);
    buttonsLayout->addWidget(lineEdit, 1);
    buttonsLayout->addWidget(addButton);
    buttonsLayout->addWidget(deleteButton);

    setLayout(buttonsLayout);

    connect(deleteButton, SIGNAL(clicked()), this, SLOT(deleteFilterRules()));
    connect(comboBox1, SIGNAL(currentIndexChanged(QString)),
            this, SLOT(currentIndexChanged(QString)));
  }

  QToolButton *addButton;
  QLineEdit *lineEdit;

private slots:
  void deleteFilterRules()
  {
    emit signalDeleteFilterRules(this);
  }

  void currentIndexChanged(QString str)
  {
    QStringList itemList;

    comboBox2->clear();
    comboBox3->setVisible(false);
    lineEdit->setVisible(true);
    if (str == "Title") {
      itemList << tr("contains") << tr("doesn't contains")
               << tr("is") << tr("isn't")
               << tr("begins with") << tr("ends with");
      comboBox2->addItems(itemList);
    } else if (str == "Description") {
      itemList << tr("contains") << tr("doesn't contains")
               << tr("is") << tr("isn't");
      comboBox2->addItems(itemList);
    } else if (str == "Author") {
      itemList << tr("contains") << tr("doesn't contains")
               << tr("is") << tr("isn't");
      comboBox2->addItems(itemList);
    } else if (str == "Category") {
      itemList << tr("is") << tr("isn't")
               << tr("begins with") << tr("ends with");
      comboBox2->addItems(itemList);
    } else if (str == "Status") {
      itemList << tr("is") << tr("isn't");
      comboBox2->addItems(itemList);
      comboBox3->setVisible(true);
      lineEdit->setVisible(false);
    } /*else if (str == "Published") {
      itemList << tr("is") << tr("isn't")
               << tr("is before") << tr("is after");
      comboBox2->addItems(itemList);
    } else if (str == "Received") {
      itemList << tr("is") << tr("isn't")
               << tr("is before") << tr("is after");
      comboBox2->addItems(itemList);
    }*/
  }

signals:
   void signalDeleteFilterRules(ItemRules *item);
};

class ItemAction : public QWidget
{
  Q_OBJECT
private:
  QToolButton *deleteButton;
  QComboBox *comboBox2;

public:
  ItemAction(QWidget * parent = 0) : QWidget(parent)
  {
    QStringList itemList;
    QComboBox *comboBox1 = new QComboBox();
    itemList << tr("Move news")  << tr("Copy news")
             << tr("Mark news as read") << tr("Add star")
             << tr("Delete");
    comboBox1->addItems(itemList);

    comboBox2 = new QComboBox();

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
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(addButton);
    buttonsLayout->addWidget(deleteButton);

    setLayout(buttonsLayout);

    connect(deleteButton, SIGNAL(clicked()), this, SLOT(deleteFilterAction()));
    connect(comboBox1, SIGNAL(currentIndexChanged(int)),
            this, SLOT(currentIndexChanged(int)));
  }

  QToolButton *addButton;

private slots:
  void deleteFilterAction()
  {
    emit signalDeleteFilterAction(this);
  }

  void currentIndexChanged(int index)
  {
    if (index < 2) comboBox2->setVisible(true);
    else comboBox2->setVisible(false);
  }

signals:
   void signalDeleteFilterAction(ItemAction *item);
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

  QScrollArea *actionsScrollArea;
  QVBoxLayout *actionsLayout;
  QWidget *actionsWidget;

  QTreeWidget *feedsTree;

public:
  explicit FilterRulesDialog(QWidget *parent = 0, QSettings *settings = 0,
                             QStringList *feedsList_ = 0);
  QLineEdit *filterName;
  QStringList feedsList_;

signals:

public slots:

private slots:
  void feedsTreeClicked(QTreeWidgetItem *item, int column);
  void closeDialog();

  void addFilterRules();
  void deleteFilterRules(ItemRules *item);

  void addFilterAction();
  void deleteFilterAction(ItemAction *item);

};

#endif // FILTERRULES_H
