#include "settings.h"


Settings::Settings( QWidget *parent ) : QDialog( parent ) {

  QCoreApplication::setOrganizationDomain( "ayoy.net" );
  qApp->installTranslator( &translator );

  ui.setupUi( this );
  ui.languageCombo->setItemData( 0, "en" );
  createLanguageMenu();
  QIntValidator *portValidator= new QIntValidator( 1, 65535, this );
  ui.portEdit->setValidator( portValidator );
  proxy.setType( QNetworkProxy::NoProxy );
  if ( proxy.type() != QNetworkProxy::NoProxy ) {
    ui.proxyBox->setChecked( true );
    ui.hostEdit->setEnabled( true );
    ui.portEdit->setEnabled( true );
  }
  ui.hostEdit->setText( proxy.hostName() );
  if ( proxy.port() ) {
    ui.portEdit->setText( QString::number( proxy.port() ) );
  }
  connect( ui.languageCombo, SIGNAL( currentIndexChanged( int )), this, SLOT( switchLanguage( int ) ) );
}

Settings::~Settings() {}

void Settings::accept() {
  setProxy();
  emit settingsOK();
  QDialog::accept();
}

void Settings::setProxy() {
  if ( ui.proxyBox->isChecked() ) {
    proxy.setType( QNetworkProxy::HttpProxy );
    proxy.setHostName( ui.hostEdit->text() );
    proxy.setPort( ui.portEdit->text().toInt() );
  } else {
    proxy.setType( QNetworkProxy::NoProxy );
  }
  QNetworkProxy::setApplicationProxy( proxy );
}

QDir Settings::directoryOf(const QString &subdir)
{
  QDir dir(QApplication::applicationDirPath());

#if defined(Q_OS_WIN)
  if (dir.dirName().toLower() == "debug" || dir.dirName().toLower() == "release")
    dir.cdUp();
#elif defined(Q_OS_MAC)
  if (dir.dirName() == "MacOS") {
    dir.cdUp();
    dir.cdUp();
    dir.cdUp();
  }
#endif
  dir.cd(subdir);
  return dir;
}

void Settings::switchLanguage( int index )
{
  QString locale = ui.languageCombo->itemData( index ).toString();
  QString qmPath = directoryOf("loc").absolutePath();
  qDebug() << "switching locale to" << locale << "from" << qmPath;
  translator.load( "qtwitter_" + locale, qmPath);
  retranslateUi();
}

void Settings::createLanguageMenu()
{
  QDir qmDir = directoryOf("loc");
  QStringList fileNames = qmDir.entryList(QStringList("qtwitter_*.qm"));
  for (int i = 0; i < fileNames.size(); ++i) {
    QString locale = fileNames[i];
    locale.remove(0, locale.indexOf('_') + 1);
    locale.chop(3);

    QTranslator translator;
    translator.load(fileNames[i], qmDir.absolutePath());
    QString language = translator.translate("Settings", "English");

    ui.languageCombo->addItem( language, locale );
    //if (language == "English")
    //    action->setChecked(true);
  }
}

void Settings::retranslateUi() {
  ui.label->setText( tr("Refresh every") );
  ui.label_2->setText( tr("minutes") );
  ui.label_3->setText( tr("Language") );
  ui.tabs->setTabText( 0, tr( "General " ) );
  ui.tabs->setTabText( 1, tr( "Network " ) );
  ui.proxyBox->setText( tr( "Use HTTP proxy" ) );
}
