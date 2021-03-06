#include "oauthwizard.h"
#include "ui_oauthwizard.h"
#include "ui_pinwidget.h"
#include "ui_allowwidget.h"
#include <QtOAuth>

#include <QDesktopServices>
#include <QUrl>
#include <QDateTime>
#include <QDebug>
#include <QRegExpValidator>
#include <QTimer>

const QByteArray OAuthWizard::TwitterRequestTokenURL = "https://twitter.com/oauth/request_token";
const QByteArray OAuthWizard::TwitterAccessTokenURL  = "https://twitter.com/oauth/access_token";
const QByteArray OAuthWizard::TwitterAuthorizeURL    = "https://twitter.com/oauth/authorize";

const QByteArray OAuthWizard::ConsumerKey    = "tIFvBZ10xbiOq5p60EcEdA";
const QByteArray OAuthWizard::ConsumerSecret = "5gXfCncW8qEilc3kAw05db2bbfw8RNsDhEAEl9iijdA";

const QByteArray OAuthWizard::ParamCallback      = "oauth_callback";
const QByteArray OAuthWizard::ParamCallbackValue = "oob";
const QByteArray OAuthWizard::ParamVerifier      = "oauth_verifier";
const QByteArray OAuthWizard::ParamScreenName    = "screen_name";


OAuthWizard::OAuthWizard(QWidget *parent) :
        QDialog(parent),
        state( false ),
        screenName( QByteArray() ),
        token( QByteArray() ),
        tokenSecret( QByteArray() ),
        qoauth( new QOAuth::Interface( this ) ),
        ui_o(new Ui::OAuthWizard),
        ui_a(new Ui::AllowWidget),
        ui_p(new Ui::PinWidget)
{
    qoauth->setConsumerKey( ConsumerKey );
    qoauth->setConsumerSecret( ConsumerSecret );
    qoauth->setRequestTimeout( 10000 );

    ui_o->setupUi(this);
    ui_a->setupUi(ui_o->widget);
    //  adjustSize();

    connect( ui_a->allowButton, SIGNAL(clicked()), this, SLOT(openUrl()) );
}

OAuthWizard::~OAuthWizard()
{
    delete ui_o;
    delete ui_a;
    delete ui_p;
}

QByteArray OAuthWizard::getScreenName() const
{
    if ( screenName.isEmpty() ) {
        qWarning() << __PRETTY_FUNCTION__ << "screenName is empty, did you finish the authorization procedure?";
    }
    return screenName;
}

QByteArray OAuthWizard::getOAuthKey() const
{
    if ( token.isEmpty() || tokenSecret.isEmpty() ) {
        qWarning() << __PRETTY_FUNCTION__ << "token and/or tokenSecret are empty, did you finish the authorization procedure?";
    }
    return token + "&" + tokenSecret;
}

bool OAuthWizard::authorized() const
{
    return state;
}

void OAuthWizard::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui_o->retranslateUi(this);
        ui_a->retranslateUi(this);
        ui_p->retranslateUi(this);
        break;
    default:
        break;
    }
}

void OAuthWizard::setOkButtonEnabled()
{
    if ( ui_p->pinEdit->text().length() < 6 ) {
        ui_p->okButton->setEnabled( false );
    } else {
        ui_p->okButton->setEnabled( true );
    }
}

void OAuthWizard::openUrl()
{
    QOAuth::ParamMap requestToken = qoauth->requestToken( TwitterRequestTokenURL, QOAuth::GET, QOAuth::HMAC_SHA1 );

    if ( qoauth->error() != QOAuth::NoError ) {
        if ( sender() == ui_p->okButton ) {
            delete ui_o->widget->layout();

            ui_p->okButton->deleteLater();
            ui_p->pinEdit->deleteLater();
            ui_p->pinLabel->deleteLater();
            ui_a->setupUi(ui_o->widget);
        }

        ui_a->allowLabel->setText( tr( "There was a network-related problem with completing the request. Please try again later." ) );
        ui_a->allowButton->setText( tr( "Retry" ) );
        adjustSize();
        QTimer::singleShot(0, this, SLOT(shrink()) );
        state = false;
        return;
    }

    token = requestToken.value( QOAuth::tokenParameterName() );
    tokenSecret = requestToken.value( QOAuth::tokenSecretParameterName() );

    QString url = TwitterAuthorizeURL;

    url.append( "?" );
    url.append( "&" + QOAuth::tokenParameterName() + "=" + token );
    url.append( "&" + ParamCallback + "=" + ParamCallbackValue );

    QDesktopServices::openUrl( QUrl( url ) );

    delete ui_o->widget->layout();

    if ( sender() == ui_p->okButton ) {
        delete ui_o->widget->layout();

        ui_p->okButton->deleteLater();
        ui_p->pinEdit->deleteLater();
        ui_p->pinLabel->deleteLater();
    } else {
        ui_a->allowLabel->deleteLater();
        ui_a->allowButton->deleteLater();
    }

    ui_p->setupUi(ui_o->widget);
    ui_p->pinEdit->setValidator( new QRegExpValidator( QRegExp( "\\d+" ), this ) );
    connect( ui_p->okButton, SIGNAL(clicked()), this, SLOT(authorize()) );
    connect( ui_p->pinEdit, SIGNAL(textChanged(QString)), this, SLOT(setOkButtonEnabled()) );
}

void OAuthWizard::authorize()
{
    ui_p->pinEdit->setEnabled( false );
    QOAuth::ParamMap otherArgs;
    otherArgs.insert( ParamVerifier, ui_p->pinEdit->text().toAscii() );
    QOAuth::ParamMap accessToken = qoauth->accessToken( TwitterAccessTokenURL, QOAuth::POST, token,
                                                        tokenSecret, QOAuth::HMAC_SHA1, otherArgs );

    if ( qoauth->error() != QOAuth::NoError ) {
        ui_p->pinEdit->hide();
        ui_p->pinLabel->setText( tr( "Either the PIN you entered is incorrect, or a network-related problem occured. Please try again later." ) );
        ui_p->okButton->setText( tr( "Retry" ) );
        adjustSize();
        QTimer::singleShot( 0, this, SLOT(shrink()) );
        disconnect( ui_p->okButton, SIGNAL(clicked()), this, SLOT(authorize()) );
        connect( ui_p->okButton, SIGNAL(clicked()), this, SLOT(openUrl()) );
        state = false;
        return;
    }

    screenName = accessToken.value( ParamScreenName );
    token = accessToken.value( QOAuth::tokenParameterName() );
    tokenSecret = accessToken.value( QOAuth::tokenSecretParameterName() );
    //  qDebug() << "token:" << token;
    //  qDebug() << "tokensecret:" << tokenSecret;
    state = true;
    accept();
}

void OAuthWizard::shrink()
{
    resize( width(), 0 );
}
